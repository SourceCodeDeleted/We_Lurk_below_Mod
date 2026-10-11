# Define functions IDA's auto-analysis missed: code that is only reached through pointers stored in data
# (callback tables such as the module start/stop list and the app mode table).
# Every dword in .rdata/.data that points into .text at an address not inside a function is tried.
# Run: idat -A -L<log> -S"tools/ida/find_pointer_functions.py <report.txt>" ida/ILB.exe.i64
import traceback

import idc
import ida_auto
import ida_bytes
import ida_funcs
import ida_loader
import ida_pro
import ida_segment
import ida_ua

report = open(idc.ARGV[1], "w")
try:
    ida_auto.auto_wait()
    text = ida_segment.get_segm_by_name(".text")
    before = ida_funcs.get_func_qty()
    created = []
    for _ in range(3):  # new functions can reveal more code
        found = 0
        for segname in (".rdata", ".data"):
            seg = ida_segment.get_segm_by_name(segname)
            ea = seg.start_ea
            end = min(seg.end_ea, seg.start_ea + 0x400000)  # initialised part only matters
            while ea + 4 <= end:
                if ida_bytes.is_loaded(ea):
                    target = ida_bytes.get_wide_dword(ea)
                    if text.start_ea <= target < text.end_ea and ida_funcs.get_func(target) is None:
                        flags = ida_bytes.get_flags(target)
                        # a plausible function start: aligned, and either already code or decodable
                        if target % 16 == 0 and (ida_bytes.is_code(flags) or ida_ua.create_insn(target) > 0):
                            if ida_funcs.add_func(target):
                                created.append((target, ea))
                                found += 1
                ea += 4
        ida_auto.auto_wait()
        if not found:
            break
    for target, ref in created:
        f = ida_funcs.get_func(target)
        report.write("%08X size %d (pointer at %08X)\n" % (target, f.size() if f else 0, ref))
    report.write("functions: %d -> %d, created %d\n" % (before, ida_funcs.get_func_qty(), len(created)))
    ida_loader.save_database(idc.get_idb_path(), 0)
except Exception:
    report.write("EXCEPTION: " + traceback.format_exc())
report.close()
ida_pro.qexit(0)
