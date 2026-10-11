# Wave 3: give `unit *` to variables the decompiler left as integers although they are used as units
# (accessed at the unit_type / unit_class / data_version offsets). Input: db/retype_units.json
# { "<function ea hex>": ["a1", "v7", ...] }, produced from the dump.
# Run: idat -A -L<log> -S"tools/ida/retype_units.py <db/retype_units.json> <report.txt>" ida/ILB.exe.i64
import json
import traceback

import idc
import ida_auto
import ida_hexrays
import ida_loader
import ida_pro
import ida_typeinf

targets = json.load(open(idc.ARGV[1]))
report = open(idc.ARGV[2], "w")
try:
    ida_auto.auto_wait()
    ida_hexrays.init_hexrays_plugin()
    unit = ida_typeinf.tinfo_t()
    unit.get_named_type(ida_typeinf.get_idati(), "unit")
    unit_ptr = ida_typeinf.tinfo_t()
    unit_ptr.create_ptr(unit)
    done = missing = 0
    for key, variables in sorted(targets.items()):
        ea = int(key, 16)
        cfunc = ida_hexrays.decompile(ea)
        if not cfunc:
            report.write("no decompile %s\n" % key)
            continue
        wanted = set(variables)
        for lv in cfunc.get_lvars():
            if lv.name in wanted and lv.width == 4:
                info = ida_hexrays.lvar_saved_info_t()
                info.ll = lv
                info.type = unit_ptr
                if ida_hexrays.modify_user_lvar_info(ea, ida_hexrays.MLI_TYPE, info):
                    done += 1
                    wanted.discard(lv.name)
        missing += len(wanted)
    report.write("variables retyped: %d, not found: %d, functions: %d\n" % (done, missing, len(targets)))
    ida_loader.save_database(idc.get_idb_path(), 0)
except Exception:
    report.write("EXCEPTION: " + traceback.format_exc())
report.close()
ida_pro.qexit(0)
