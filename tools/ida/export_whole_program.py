# Export the whole program as one mechanical C file (Hex-Rays "create C file") plus its type header.
# This is the raw material for draft/: a complete but unrefined version of every function.
# Run: idat -A -L<log> -S"tools/ida/export_whole_program.py <out.c> <report.txt>" ida/ILB.exe.i64
import traceback

import idc
import ida_auto
import ida_hexrays
import ida_pro

report = open(idc.ARGV[2], "w")
try:
    ida_auto.auto_wait()
    ida_hexrays.init_hexrays_plugin()
    # the database keeps pseudo-C from earlier sessions; it must not be reused after prototypes changed
    import idautils
    for ea in idautils.Functions():
        ida_hexrays.mark_cfunc_dirty(ea)
    ida_hexrays.clear_cached_cfuncs()
    ok = ida_hexrays.decompile_many(idc.ARGV[1], None, ida_hexrays.VDRUN_NEWFILE | ida_hexrays.VDRUN_SILENT | ida_hexrays.VDRUN_MAYSTOP)
    report.write("decompile_many: %s\n" % ok)
except Exception:
    report.write("EXCEPTION: " + traceback.format_exc())
report.close()
ida_pro.qexit(0)
