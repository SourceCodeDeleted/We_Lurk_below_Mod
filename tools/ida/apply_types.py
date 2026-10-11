# Wave 3: load db/types.h + db/definitions.h + db/custom_data.h (+ db/enums.h) into the work database, type the excel definition globals and
# give named functions their struct-pointer parameter types.
# Run: idat -A -L<log> -S"tools/ida/apply_types.py <repo root> <report.txt>" ida/ILB.exe.i64
#
# Parameter types come from the assert signature strings. Only the types are taken: the calling convention
# and argument locations stay as IDA detected them (link-time optimisation moved many arguments to registers),
# and a prototype is only touched when IDA sees the same number of arguments as the signature lists.
import json
import os
import re
import traceback

import idc
import idautils
import ida_auto
import ida_bytes
import ida_idaapi
import ida_segment
import ida_ua
import ida_loader
import ida_name
import ida_nalt
import ida_pro
import ida_typeinf

root = idc.ARGV[1]
report = open(idc.ARGV[2], "w")


def log(msg):
    report.write(msg + "\n")
    report.flush()


def split_params(sig):
    inner = sig[sig.index("(") + 1:sig.rindex(")")]
    parts, depth, cur = [], 0, ""
    for ch in inner:
        if ch == "," and depth == 0:
            parts.append(cur.strip())
            cur = ""
        else:
            depth += ch in "(<"
            depth -= ch in ")>"
            cur += ch
    if cur.strip() and cur.strip() != "void":
        parts.append(cur.strip())
    return parts


try:
    ida_auto.auto_wait()
    names = json.load(open(os.path.join(root, "db", "names.json")))
    tables = json.load(open(os.path.join(root, "db", "excel_tables.json")))

    # Every struct named in a signature gets at least a forward declaration, so pointers to it can be typed.
    known_text = "\n".join(open(os.path.join(root, "db", name)).read()
                           for name in ("types.h", "definitions.h", "custom_data.h"))
    defined = set(re.findall(r"^struct (\w+)", known_text, re.M))
    mentioned = set()
    for entry in names.values():
        for sig in [entry.get("sig")] + entry.get("inlined", []):
            if sig:
                mentioned.update(re.findall(r"struct (\w+)", sig))
    forward = "".join("struct %s;\n" % s for s in sorted(mentioned - defined))
    enums_path = os.path.join(root, "db", "enums.h")
    enums_text = open(enums_path).read() if os.path.exists(enums_path) else ""
    decls = forward + "\n".join(
        line for line in (known_text + "\n" + enums_text).splitlines() if not line.startswith("#pragma"))
    errors = ida_typeinf.parse_decls(None, decls, None, ida_typeinf.HTI_DCL)
    log("parse_decls errors: %s (forward-declared structs: %d)" % (errors, len(mentioned - defined)))

    til = ida_typeinf.get_idati()
    typed_globals = 0
    for t in tables:
        stem = t["struct"][:-len("_definition_data")]
        for ea, name, decl in ((t["definitions_global"], "%s_definitions" % stem, "%s *%s_definitions;" % (t["struct"], stem)),
                               (t["count_global"], "num_%s_definitions" % stem, "int num_%s_definitions;" % stem)):
            ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
            if idc.SetType(ea, decl):
                typed_globals += 1
            else:
                log("global type failed: %X %s" % (ea, decl))
    log("definition globals typed: %d of %d" % (typed_globals, 2 * len(tables)))

    # Remember every string literal: a wrongly typed pointer parameter makes IDA lay a struct over the
    # constants passed to it, which destroys the strings underneath.
    saved_strings = [(st.ea, st.length, st.strtype) for st in idautils.Strings()]
    our_structs = defined | (mentioned - defined)
    original = {}

    applied = skipped_count = skipped_none = failed = 0
    for key, entry in sorted(names.items()):
        if entry.get("confidence") not in ("high", "medium"):
            continue
        params = split_params(entry["sig"])
        if not any("struct " in p and p.endswith("*") for p in params):
            skipped_none += 1
            continue
        ea = int(key, 16)
        tif = ida_typeinf.tinfo_t()
        if not ida_nalt.get_tinfo(tif, ea):
            if not ida_typeinf.guess_tinfo(tif, ea):
                failed += 1
                continue
        ftd = ida_typeinf.func_type_data_t()
        if not tif.get_func_details(ftd) or len(ftd) != len(params):
            skipped_count += 1
            continue
        changed = False
        for i, p in enumerate(params):
            m = re.fullmatch(r"(?:const )?struct (\w+) \*", p)
            if not m or ftd[i].type.get_size() != 4 or ftd[i].type.is_floating():
                continue
            target = ida_typeinf.tinfo_t()
            if not target.get_named_type(til, m.group(1)):
                continue
            ptr = ida_typeinf.tinfo_t()
            ptr.create_ptr(target)
            ftd[i].type = ptr
            changed = True
        if not changed:
            continue
        new = ida_typeinf.tinfo_t()
        if new.create_func(ftd) and ida_typeinf.apply_tinfo(ea, new, ida_typeinf.TINFO_DEFINITE):
            original[ea] = tif.copy()
            applied += 1
        else:
            failed += 1
    log("prototypes: applied %d, argument count differs %d, no struct pointer params %d, failed %d" % (
        applied, skipped_count, skipped_none, failed))

    # Roll back prototypes that made IDA put one of our structs on read-only data or code, then restore
    # the strings. A struct there means a constant was passed where we claimed a struct pointer.
    for _ in range(4):
        ida_auto.auto_wait()
        bogus = []
        for segname in (".rdata", ".text"):
            seg = ida_segment.get_segm_by_name(segname)
            ea = seg.start_ea
            while ea != ida_idaapi.BADADDR and ea < seg.end_ea:
                if ida_bytes.is_struct(ida_bytes.get_flags(ea)):
                    t = ida_typeinf.tinfo_t()
                    if ida_nalt.get_tinfo(t, ea) and (t.get_type_name() or "") in our_structs:
                        bogus.append((ea, ida_bytes.get_item_size(ea)))
                ea = ida_bytes.next_head(ea, seg.end_ea)
        if not bogus:
            break
        culprits = set()
        for ea, size in bogus:
            for x in idautils.XrefsTo(ea):
                cur = x.frm
                for _step in range(12):
                    cur = ida_bytes.next_head(cur, cur + 64)
                    if cur == ida_idaapi.BADADDR:
                        break
                    if ida_ua.print_insn_mnem(cur) == "call":
                        for cx in idautils.CodeRefsFrom(cur, 0):
                            if cx in original:
                                culprits.add(cx)
                        break
        for ea, size in bogus:
            ida_bytes.del_items(ea, ida_bytes.DELIT_SIMPLE, size)
        for cx in culprits:
            ida_typeinf.apply_tinfo(cx, original.pop(cx), ida_typeinf.TINFO_DEFINITE)
            log("rolled back prototype: %X %s" % (cx, ida_name.get_name(cx)))
        restored = 0
        for sea, slen, stype in saved_strings:
            if not ida_bytes.is_strlit(ida_bytes.get_flags(sea)):
                if ida_bytes.create_strlit(sea, slen, stype):
                    restored += 1
        log("pass: %d bogus structs removed, %d prototypes rolled back, %d strings restored" % (
            len(bogus), len(culprits), restored))
    log("prototypes kept: %d; strings before %d, after %d" % (
        len(original), len(saved_strings), sum(1 for _ in idautils.Strings())))
    ida_loader.save_database(idc.get_idb_path(), 0)
except Exception:
    log("EXCEPTION: " + traceback.format_exc())
report.close()
ida_pro.qexit(0)
