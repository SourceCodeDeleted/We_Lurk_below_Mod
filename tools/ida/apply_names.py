# Wave 2: apply recovered names to the work database.
# Run: idat -A -L<log> -S"tools/ida/apply_names.py <db/names.json> <db/files.json> <report.txt>" ida/ILB.exe.i64
# high/medium names are applied; low ones only become a comment. The original signature and source location
# go into a function comment. Calling conventions are left as IDA detected them: the build used link-time
# optimisation, so many functions do not follow the __cdecl written in their signature string.
import json
import re
import traceback

import idc
import ida_auto
import ida_funcs
import ida_loader
import ida_name
import ida_pro
import ida_typeinf

names = json.load(open(idc.ARGV[1]))
files = json.load(open(idc.ARGV[2]))
report = open(idc.ARGV[3], "w")

ASSERT_EA = 0x401220
ASSERT_DECL = ("char __fastcall assert_failed(const char *expr, const char *file, "
               "const char *func, int line, char show_stack);")

try:
    ida_auto.auto_wait()
    ida_name.set_name(ASSERT_EA, "assert_failed", ida_name.SN_NOWARN)
    report.write("assert prototype applied: %s\n" % idc.SetType(ASSERT_EA, ASSERT_DECL))

    used = {}
    applied = renamed_dupes = failed = 0
    for key in sorted(names):
        entry = names[key]
        ea = int(key, 16)
        loc = files.get(key, {})
        comment = []
        if entry.get("sig"):
            comment.append("%s  [%s]" % (entry["sig"], entry["confidence"]))
        if loc.get("file"):
            comment.append("%s:%s" % (loc["file"], loc.get("line")))
        if entry.get("inlined"):
            comment.append("inlined: " + ", ".join(
                re.search(r"([\w:~]+)\s*\(", s).group(1) for s in entry["inlined"]))
        func = ida_funcs.get_func(ea)
        if func and comment:
            ida_funcs.set_func_cmt(func, "\n".join(comment), True)
        if entry.get("confidence") not in ("high", "medium", "manual"):
            ida_name.set_name(ea, "", ida_name.SN_NOWARN)  # drop a name left by an earlier, different run
            continue
        name = re.sub(r"[^\w]", "_", entry["name"])
        if name in used:  # statics with the same name in two files
            suffix = (loc.get("file") or "dup").rsplit(".", 1)[0]
            name = "%s__%s" % (name, suffix)
            renamed_dupes += 1
            if name in used:
                name = "%s_%X" % (name, ea)
        used[name] = ea
        if ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE):
            applied += 1
        else:
            failed += 1
            report.write("rename failed %s %s\n" % (key, name))
    report.write("applied %d, duplicate names suffixed %d, failed %d\n" % (applied, renamed_dupes, failed))
    ida_loader.save_database(idc.get_idb_path(), 0)
except Exception:
    report.write("EXCEPTION: " + traceback.format_exc())
report.close()
ida_pro.qexit(0)
