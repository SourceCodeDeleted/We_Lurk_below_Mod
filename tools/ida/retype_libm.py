# The math imports _libm_sse2_*_precise take and return a double in xmm0. IDA does not know that, so the
# decompiler drops the argument at every call. Give them their real prototype and list the functions that
# call them (to be re-decompiled with tools/ida/add_functions.py). Changes the work database.
# Run: idat -A -L<log> -S"tools/ida/retype_libm.py <callers.txt> <report.txt>" ida/ILB.exe.i64
import traceback

import idc
import ida_auto
import ida_funcs
import ida_pro
import idautils

report = open(idc.ARGV[2], "w")
try:
    ida_auto.auto_wait()
    callers = set()
    for ea, name in idautils.Names():
        if "libm_sse2_" not in name:
            continue
        two = "pow" in name or "atan2" in name
        proto = "double __usercall f@<xmm0>(double x@<xmm0>%s);" % (", double y@<xmm1>" if two else "")
        ok = idc.SetType(ea, proto)
        refs = set()
        for ref in list(idautils.CodeRefsTo(ea, 0)) + list(idautils.DataRefsTo(ea)):
            func = ida_funcs.get_func(ref)
            if func is not None:
                refs.add(func.start_ea)
        callers |= refs
        report.write("%08X %s seg %s: SetType %s, %d calling functions, type now: %s\n" % (
            ea, name, idc.get_segm_name(ea), ok, len(refs), idc.get_type(ea)))
    open(idc.ARGV[1], "w").write("\n".join("%X" % ea for ea in sorted(callers)) + "\n")
    report.write("callers: %d\n" % len(callers))
except Exception:
    report.write("EXCEPTION: " + traceback.format_exc())
report.close()
ida_pro.qexit(0)
