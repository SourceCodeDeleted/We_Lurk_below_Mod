# Wave 0 probe: headless auto-analysis, then decompile one function found via its assert signature string.
# Run: idat -A -c -L<log> -S"tools/ida/wave0_probe.py <out.txt>" ida/ILB.exe
import time
import idc
import idautils
import ida_auto
import ida_funcs
import ida_hexrays
import ida_loader
import ida_pro

out_path = idc.ARGV[1]
out = open(out_path, "w")


def log(msg):
    out.write(msg + "\n")
    out.flush()


try:
    t0 = time.time()
    ida_auto.auto_wait()
    log("auto-analysis: %.1fs" % (time.time() - t0))
    log("functions: %d" % ida_funcs.get_func_qty())

    target = None
    for s in idautils.Strings():
        if "upgrade_v8_to_v9" in str(s):
            for xref in idautils.XrefsTo(s.ea):
                f = ida_funcs.get_func(xref.frm)
                if f:
                    target = f.start_ea
                    break
            break
    log("target: %s" % (hex(target) if target else None))

    log("init_hexrays_plugin: %s" % ida_hexrays.init_hexrays_plugin())
    if target:
        for ea in (target, target):  # second run shows cached cost
            t1 = time.time()
            hf = ida_hexrays.hexrays_failure_t()
            cfunc = ida_hexrays.decompile(ea, hf)
            log("decompile %.2fs ok=%s err=%s" % (time.time() - t1, cfunc is not None, hf.desc()))
        if cfunc:
            log(str(cfunc))
    ida_loader.save_database(idc.get_idb_path(), 0)
    log("saved: %s" % idc.get_idb_path())
except Exception as e:
    import traceback
    log("EXCEPTION: " + traceback.format_exc())
out.close()
ida_pro.qexit(0)
