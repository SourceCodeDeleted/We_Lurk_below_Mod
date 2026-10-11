# Wave 1: dump every function (pseudo-C, disassembly, xrefs, strings) plus strings/imports/segments.
# Run: idat -A -L<log> -S"tools/ida/export_all.py <dump_dir> [force | <functions.json>]" ida/ILB.exe.i64
# Resumable: functions that already have a .c file are skipped unless "force" is given.
import json
import os
import time
import traceback

import idc
import idautils
import ida_auto
import ida_bytes
import ida_funcs
import ida_hexrays
import ida_lines
import ida_nalt
import ida_name
import ida_pro
import ida_segment

dump_dir = idc.ARGV[1]
force = len(idc.ARGV) > 2 and idc.ARGV[2] == "force"
# optional: a JSON file whose keys are function addresses (hex) to decompile again even if cached
only = set()
if len(idc.ARGV) > 2 and idc.ARGV[2].endswith(".json"):
    only = {int(k, 16) for k in json.load(open(idc.ARGV[2]))}
func_dir = os.path.join(dump_dir, "functions")
os.makedirs(func_dir, exist_ok=True)
progress = open(os.path.join(dump_dir, "progress.log"), "a")


def log(msg):
    progress.write("%s %s\n" % (time.strftime("%H:%M:%S"), msg))
    progress.flush()


def write_json(name, obj):
    with open(os.path.join(dump_dir, name), "w") as f:
        json.dump(obj, f, indent=1)


def disasm_text(func):
    lines = []
    for start, end in idautils.Chunks(func.start_ea):
        for head in idautils.Heads(start, end):
            lines.append("%08X  %s" % (head, ida_lines.tag_remove(idc.generate_disasm_line(head, 0) or "")))
    return "\n".join(lines)


def main():
    ida_auto.auto_wait()
    ida_hexrays.init_hexrays_plugin()

    segments = []
    for n in range(ida_segment.get_segm_qty()):
        s = ida_segment.getnseg(n)
        segments.append({"name": ida_segment.get_segm_name(s), "start": s.start_ea, "end": s.end_ea,
                         "class": ida_segment.get_segm_class(s)})
    write_json("segments.json", segments)

    string_at = {}
    strings = []
    for s in idautils.Strings():
        text = str(s)
        string_at[s.ea] = text
        strings.append({"ea": s.ea, "len": s.length, "text": text})
    write_json("strings.json", strings)

    imports = []
    for i in range(ida_nalt.get_import_module_qty()):
        module = ida_nalt.get_import_module_name(i)

        def cb(ea, name, ordinal, module=module):
            imports.append({"ea": ea, "module": module, "name": name, "ordinal": ordinal})
            return True
        ida_nalt.enum_import_names(i, cb)
    write_json("imports.json", imports)

    index = []
    fails = 0
    eas = list(idautils.Functions())
    log("functions: %d" % len(eas))
    for n, ea in enumerate(eas):
        func = ida_funcs.get_func(ea)
        entry = {
            "ea": ea, "name": ida_name.get_name(ea), "size": func.size(),
            "end": func.end_ea, "lib": bool(func.flags & ida_funcs.FUNC_LIB),
            "thunk": bool(func.flags & ida_funcs.FUNC_THUNK),
        }
        callers = set()
        for x in idautils.XrefsTo(ea):
            f = ida_funcs.get_func(x.frm)
            callers.add(f.start_ea if f else x.frm)
        callees, strs, data = [], [], set()
        for start, end in idautils.Chunks(ea):
            for head in idautils.Heads(start, end):
                for x in idautils.XrefsFrom(head, 0):
                    if x.type in (idc.fl_CN, idc.fl_CF, idc.fl_JN, idc.fl_JF):
                        f = ida_funcs.get_func(x.to)
                        if f and f.start_ea != ea and f.start_ea == x.to:
                            callees.append([head, x.to])
                    elif x.to in string_at:
                        strs.append([head, x.to])
                    elif x.type in (idc.dr_O, idc.dr_R, idc.dr_W) and ida_bytes.is_mapped(x.to):
                        data.add(x.to)
        entry["callers"] = sorted(callers)
        entry["callees"] = callees
        entry["strings"] = strs
        entry["data"] = sorted(data)

        base = os.path.join(func_dir, "%08X" % ea)
        with open(base + ".asm", "w") as f:
            f.write(disasm_text(func))
        if force or ea in only or not os.path.exists(base + ".c"):
            if entry["lib"] or entry["thunk"]:
                entry["decomp"] = "skipped"
            else:
                hf = ida_hexrays.hexrays_failure_t()
                try:
                    cfunc = ida_hexrays.decompile(ea, hf)
                except Exception as e:
                    cfunc = None
                    entry["decomp_err"] = "exception: %s" % e
                if cfunc:
                    with open(base + ".c", "w") as f:
                        f.write(str(cfunc))
                    entry["decomp"] = "ok"
                    fails = 0
                else:
                    entry["decomp"] = "fail"
                    entry.setdefault("decomp_err", hf.desc())
                    fails += 1
                    log("fail %08X %s" % (ea, entry["decomp_err"]))
                    if fails >= 5:  # likely throttling or network loss: back off, keep going
                        time.sleep(30)
        else:
            entry["decomp"] = "ok"
        index.append(entry)
        if n % 100 == 0:
            log("%d/%d" % (n, len(eas)))
    write_json("index.json", index)
    log("done: %d functions, %d decompiled, %d failed" % (
        len(index), sum(e["decomp"] == "ok" for e in index), sum(e["decomp"] == "fail" for e in index)))


try:
    main()
except Exception:
    log("EXCEPTION: " + traceback.format_exc())
progress.close()
ida_pro.qexit(0)
