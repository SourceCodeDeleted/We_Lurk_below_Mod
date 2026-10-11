# Export library types (Direct3D / DXGI interfaces and structures) that the decompiler output uses but that
# are not among the database's local types. Run on a scratch copy of the database: it imports the types.
# Run: idat -A -L<log> -S"tools/ida/export_named_types.py <names.txt> <out.h> <report.txt>" <copy>.i64
import traceback

import idc
import ida_auto
import ida_pro
import ida_typeinf

report = open(idc.ARGV[3], "w")
try:
    ida_auto.auto_wait()
    til = ida_typeinf.get_idati()
    before = ida_typeinf.get_ordinal_limit(til)
    wanted = open(idc.ARGV[1]).read().split()
    for name in wanted:
        if idc.import_type(-1, name) == idc.BADADDR:
            report.write("not found: %s\n" % name)
    after = ida_typeinf.get_ordinal_limit(til)
    flags = ida_typeinf.PDF_INCL_DEPS | ida_typeinf.PDF_DEF_FWD | ida_typeinf.PDF_DEF_BASE
    text = idc.print_decls(",".join(str(i) for i in range(before, after)), flags) or ""
    with open(idc.ARGV[2], "w") as f:
        f.write(text)
    report.write("new ordinals: %d, bytes: %d\n" % (after - before, len(text)))
except Exception:
    report.write("EXCEPTION: " + traceback.format_exc())
report.close()
ida_pro.qexit(0)
