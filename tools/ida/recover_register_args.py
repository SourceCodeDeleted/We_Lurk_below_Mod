# Recover arguments that functions take in registers but that their prototype does not list.
# Symptom in the pseudo-C: "variable 'v2' is possibly undefined" for a register variable.
# For each function in the list: find the registers that are read before being written on some path from the
# entry (both sides of branches, each path up to its first call), add each as an argument (__usercall), and
# keep the change only if the decompiler then stops reporting undefined variables for that register.
# Changes the database it is run on. Callers must be re-decompiled afterwards (whole-program export).
# Run: idat -A -L<log> -S"tools/ida/recover_register_args.py <functions.txt> <report.txt>" <db>.i64
import re
import traceback

import idautils
import idc
import ida_auto
import ida_funcs
import ida_hexrays
import ida_idp
import ida_nalt
import ida_pro
import ida_typeinf
import ida_ua

FAMILY = {}
for names in (("eax", "ax", "al", "ah"), ("ebx", "bx", "bl", "bh"), ("ecx", "cx", "cl", "ch"), ("edx", "dx", "dl", "dh"),
              ("esi", "si"), ("edi", "di")):
    for n in names:
        FAMILY[n] = names[0]
CANDIDATES = ("ecx", "edx", "eax", "esi", "edi", "ebx")


def instruction_effects(at):
    """(size, mnemonic, registers read, registers written, jump target or None) of one instruction."""
    insn = ida_ua.insn_t()
    size = ida_ua.decode_insn(insn, at)
    if size <= 0:
        return 0, "", [], [], None
    mnem = insn.get_canon_mnem()
    feature = insn.get_canon_feature()
    reads, writes = [], []
    for i in range(4):
        op = insn.ops[i]
        if op.type == ida_ua.o_void:
            break
        if op.type == ida_ua.o_reg:
            name = FAMILY.get(ida_idp.get_reg_name(op.reg, 4 if op.dtype >= 2 else (2 if op.dtype == 1 else 1)))
            if name:
                if feature & (ida_idp.CF_USE1 << i):
                    reads.append(name)
                if feature & (ida_idp.CF_CHG1 << i):
                    writes.append(name)
        elif op.type in (ida_ua.o_phrase, ida_ua.o_displ):
            text = idc.print_operand(at, i)
            reads += [reg for reg in CANDIDATES if reg in text]
    if mnem == "push":
        reads = []                      # saves a register or reserves stack: does not use the value
    elif mnem in ("xor", "sub", "sbb") and idc.print_operand(at, 0) == idc.print_operand(at, 1):
        reads, writes = [], [FAMILY.get(idc.print_operand(at, 0))]      # clears the register
    target = None
    if mnem.startswith("j") and insn.ops[0].type in (ida_ua.o_near, ida_ua.o_far):
        target = insn.ops[0].addr
    return size, mnem, reads, [w for w in writes if w], target


def callee_register_arguments(call_ea):
    targets = [x for x in idautils.CodeRefsFrom(call_ea, 0)]
    regs = []
    for target in targets:
        tif = ida_typeinf.tinfo_t()
        ftd = ida_typeinf.func_type_data_t()
        if ida_nalt.get_tinfo(tif, target) and tif.get_func_details(ftd):
            for arg in ftd:
                if arg.argloc.is_reg1():
                    name = FAMILY.get(ida_idp.get_reg_name(arg.argloc.reg1(), 4))
                    if name:
                        regs.append(name)
    return regs


def live_in_registers(ea):
    """Registers read before being written on some path from the function entry.

    Both sides of every conditional jump are followed; a path ends at a call (what the callee reads or
    changes is not known here), at a return, or when it leaves the function."""
    func = ida_funcs.get_func(ea)
    live, seen, work, steps = [], set(), [(ea, frozenset())], 0
    while work and steps < 3000:
        at, written = work.pop()
        while func.start_ea <= at < func.end_ea and (at, written) not in seen and steps < 3000:
            seen.add((at, written))
            steps += 1
            size, mnem, reads, writes, target = instruction_effects(at)
            if mnem == "call":
                # the registers the callee takes as arguments are read here: a register this function only
                # hands on to the callee is still one of its own inputs
                for reg in callee_register_arguments(at):
                    if reg not in written and reg not in live:
                        live.append(reg)
                break
            if size == 0 or mnem in ("retn", "ret", "int3", "int"):
                break
            for reg in reads:
                if reg not in written and reg not in live:
                    live.append(reg)
            written = written | frozenset(writes)
            if mnem == "jmp":
                if target is None:
                    break               # a jump table
                at = target
                continue
            if target is not None:
                work.append((target, written))
            at += size
    return live


RETURN_REGISTER = {1: "al", 2: "ax", 4: "eax"}


def usercall_prototype(cfunc, live):
    """The function's current prototype as __usercall text with the registers in `live` added as arguments."""
    header = next(l for l in str(cfunc).split("\n") if l and not l.startswith("//"))
    m = re.match(r"^(.*?)\b(__cdecl|__fastcall|__thiscall|__stdcall|__usercall|__userpurge)?\s*([\w$?@:~]+?)(@<\w+>)?\((.*)\)$", header)
    if not m:
        return None, []
    ret, cc, name, retloc, params = m.group(1).strip(), m.group(2) or "__cdecl", m.group(3), m.group(4), m.group(5).strip()
    if cc in ("__stdcall", "__userpurge") or "..." in params:
        return None, []                 # the callee removes its arguments: not expressible by adding a register
    params = [] if params in ("", "void") else [x.strip() for x in params.split(",")]
    have = set(re.findall(r"@<(\w+)>", header))
    if cc == "__fastcall":
        for i, reg in enumerate(("ecx", "edx")):
            if i < len(params):
                params[i] += "@<%s>" % reg
                have.add(reg)
    elif cc == "__thiscall" and params:
        params[0] += "@<ecx>"
        have.add("ecx")
    added = [r for r in live if r not in have]
    params = ["int reg_%s@<%s>" % (r, r) for r in added] + params
    if retloc is None and ret != "void":
        tif = ida_typeinf.tinfo_t()
        size = 4
        if cfunc.get_func_type(tif):
            rettype = tif.get_rettype()
            if rettype.is_floating():
                return None, []
            size = rettype.get_size()
        if size not in RETURN_REGISTER:
            return None, []
        retloc = "@<%s>" % RETURN_REGISTER[size]
    return "%s __usercall %s%s(%s);" % (ret, "f", retloc or "", ", ".join(params)), added


def undeclared_register_names(cfunc):
    text = str(cfunc)
    header = next(l for l in text.split("\n") if l and not l.startswith("//"))
    body = text[text.index(header) + len(header):]
    used = set(re.findall(r"(?<![\w.>])reg_(e[a-ds][xi])\b", body))
    declared = set(re.findall(r"\breg_(e[a-ds][xi])\b", header)) | set(re.findall(r"^  [^;\n]*\breg_(e[a-ds][xi]); //", body, re.M))
    return sorted(used - declared)


def undefined_registers(cfunc):
    """Processor registers of the variables the decompiler reports as possibly undefined."""
    names = set()
    for w in cfunc.warnings:
        if "possibly undefined" in w.text:
            names.add(w.text.split("'")[1])
    regs = set()
    for lv in cfunc.lvars:
        if lv.name in names and lv.is_reg_var():
            reg = ida_hexrays.get_mreg_name(lv.location.reg1(), 4)
            regs.add(FAMILY.get(reg, reg))
    return regs, len(names)


report = open(idc.ARGV[2], "w")
try:
    ida_auto.auto_wait()
    ida_hexrays.init_hexrays_plugin()
    fixed = partly = untouched = 0
    work = [int(text, 16) for text in open(idc.ARGV[1]).read().split()]
    done_fixed, position = set(), 0
    while position < len(work) and position < 4000:
        ea = work[position]
        position += 1
        if ea in done_fixed:
            continue
        try:
            cfunc = ida_hexrays.decompile(ea, flags=ida_hexrays.DECOMP_NO_CACHE)
            regs, before = undefined_registers(cfunc)
            # a register handed on to a callee shows up as a variable named after the callee's argument
            # (reg_ecx) that this function neither declares nor receives
            handed_on = set(undeclared_register_names(cfunc))
            before += len(handed_on)
            live = [r for r in live_in_registers(ea) if r in regs or r in handed_on]
            if not live:
                untouched += 1
                report.write("%08X untouched: undefined in %s, read at entry: %s\n" % (ea, sorted(regs), live_in_registers(ea)))
                continue
            old = ida_typeinf.tinfo_t()
            had_type = ida_nalt.get_tinfo(old, ea)
            proto, added = usercall_prototype(cfunc, live)
            if proto is None or not added or not idc.SetType(ea, proto):
                untouched += 1
                report.write("%08X untouched: could not build a prototype adding %s (%s)\n" % (ea, live, proto))
                continue
            ida_hexrays.mark_cfunc_dirty(ea)
            again = ida_hexrays.decompile(ea, flags=ida_hexrays.DECOMP_NO_CACHE)
            regs_after, after = undefined_registers(again)
            after += len(undeclared_register_names(again))
            if after < before:
                if after == 0:
                    fixed += 1
                else:
                    partly += 1
                report.write("%08X %s: added %s; undefined variables %d -> %d; now %s\n" % (
                    ea, "fixed" if after == 0 else "partly", added, before, after, idc.get_type(ea)))
                # its callers now have to supply the register: look at them (again)
                done_fixed.add(ea)
                for ref in idautils.CodeRefsTo(ea, 0):
                    caller = ida_funcs.get_func(ref)
                    if caller is not None and caller.start_ea not in done_fixed:
                        ida_hexrays.mark_cfunc_dirty(caller.start_ea)
                        work.append(caller.start_ea)
            else:
                if had_type:
                    ida_typeinf.apply_tinfo(ea, old, ida_typeinf.TINFO_DEFINITE)
                else:
                    ida_nalt.del_tinfo(ea)
                untouched += 1
                report.write("%08X reverted: adding %s did not help\n" % (ea, added))
        except Exception as e:
            untouched += 1
            report.write("%08X error: %s\n" % (ea, e))
    report.write("fixed %d, partly %d, untouched %d\n" % (fixed, partly, untouched))
except Exception:
    report.write("EXCEPTION: " + traceback.format_exc())
report.close()
ida_pro.qexit(0)
