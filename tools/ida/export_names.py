# List every named address in the work database (functions and data) with its size, for the draft's
# data layout. Run: idat -A -L<log> -S"tools/ida/export_names.py <out.json>" ida/ILB.exe.i64
import json
import traceback

import idc
import idautils
import ida_auto
import ida_bytes
import ida_pro
import ida_segment

try:
    ida_auto.auto_wait()
    out = []
    for ea, name in idautils.Names():
        seg = ida_segment.getseg(ea)
        out.append({"ea": ea, "name": name, "size": ida_bytes.get_item_size(ea),
                    "seg": ida_segment.get_segm_name(seg) if seg else None})
    json.dump(out, open(idc.ARGV[1], "w"))
except Exception:
    open(idc.ARGV[1] + ".err", "w").write(traceback.format_exc())
ida_pro.qexit(0)
