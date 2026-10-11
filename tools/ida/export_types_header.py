# Export every local type of the work database as one C header, so the whole-program C file can be
# compiled without the Windows / Direct3D / SDL headers (IDA already knows the types it used).
# Run: idat -A -L<log> -S"tools/ida/export_types_header.py <out.h> <report.txt>" ida/ILB.exe.i64
import traceback

import idc
import ida_auto
import ida_pro
import ida_typeinf

report = open(idc.ARGV[2], "w")
try:
    ida_auto.auto_wait()
    til = ida_typeinf.get_idati()
    limit = ida_typeinf.get_ordinal_limit(til)
    flags = ida_typeinf.PDF_INCL_DEPS | ida_typeinf.PDF_DEF_FWD | ida_typeinf.PDF_DEF_BASE
    text = idc.print_decls(",".join(str(i) for i in range(1, limit)), flags) or ""
    with open(idc.ARGV[1], "w") as f:
        f.write(text)
    rc, lines = bool(text), [text]
    report.write("ordinals: %d, print_decls: %s, bytes: %d\n" % (limit - 1, rc, sum(len(x) for x in lines)))
except Exception:
    report.write("EXCEPTION: " + traceback.format_exc())
report.close()
ida_pro.qexit(0)
