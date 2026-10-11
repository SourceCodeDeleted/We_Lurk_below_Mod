# Define functions IDA missed (code reached only through pointer tables, found by tools/draft_data.py)
# and write their pseudo-C in the whole-program export's format, so tools/draft_fix.py can append them.
# Changes the work database (new functions). Also writes bodies of functions the export skips (nullsub_*).
# Run: idat -A -L<log> -S"tools/ida/add_functions.py <out.c> <report.txt> <[!]hex ea | @file> ..." ida/ILB.exe.i64
import json
import os
import traceback

import idc
import ida_auto
import ida_funcs
import ida_hexrays
import ida_pro
import ida_typeinf

report = open(idc.ARGV[2], "w")
try:
    ida_auto.auto_wait()
    ida_hexrays.init_hexrays_plugin()
    out = open(idc.ARGV[1], "w")
    fixes_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "db", "prototype_fixes.json")
    for key, proto in json.load(open(fixes_path))["prototypes"].items():
        report.write("%s: SetType %s\n" % (key, idc.SetType(int(key, 16), proto)))
    wanted = []
    for arg in idc.ARGV[3:]:                # "@file" = one hex address per line
        wanted += open(arg[1:]).read().split() if arg.startswith("@") else [arg]
    for text in wanted:
        store = text.startswith("!")        # "!ea": also store the decompiled prototype, so that callers
        ea = int(text.lstrip("!"), 16)      # decompiled afterwards pass the arguments it really takes
        owner = ida_funcs.get_func(ea)
        if owner is not None and owner.start_ea != ea:
            # IDA attached the code to a function that jumps to it (a tail call); it is a function of its
            # own when something else points at it. Detach it and re-decompile the former owner too.
            removed = ida_funcs.remove_func_tail(owner, ea)
            ida_auto.auto_wait()
            report.write("%08X: detached from %s: %s\n" % (ea, idc.get_func_name(owner.start_ea), removed))
            if "%X" % owner.start_ea not in [a.upper() for a in idc.ARGV[3:]]:
                idc.ARGV.append("%X" % owner.start_ea)
        if ida_funcs.get_func(ea) is None:
            created = ida_funcs.add_func(ea)
            ida_auto.auto_wait()
            report.write("%08X: add_func %s\n" % (ea, created))
        func = ida_funcs.get_func(ea)
        if func is None or func.start_ea != ea:
            report.write("%08X: NOT a function start\n" % ea)
            continue
        try:
            ida_hexrays.mark_cfunc_dirty(ea)        # the database keeps old pseudo-C; it predates the type changes
            cfunc = ida_hexrays.decompile(ea, flags=ida_hexrays.DECOMP_NO_CACHE)
            if store:
                tif = ida_typeinf.tinfo_t()
                cfunc.get_func_type(tif)
                report.write("%08X: prototype stored: %s (%s)\n" % (ea, ida_typeinf.apply_tinfo(ea, tif, ida_typeinf.TINFO_DEFINITE), tif))
            out.write("//----- (%08X) --------------------------------------------------------\n%s\n" % (ea, str(cfunc)))
            report.write("%08X: %s, %d bytes, ends %08X\n" % (ea, idc.get_func_name(ea), func.end_ea - ea, func.end_ea))
        except Exception as e:
            report.write("%08X: decompile failed: %s\n" % (ea, e))
    out.close()
except Exception:
    report.write("EXCEPTION: " + traceback.format_exc())
report.close()
ida_pro.qexit(0)
