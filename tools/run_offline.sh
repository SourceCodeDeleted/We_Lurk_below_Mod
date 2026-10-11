#!/bin/sh
# Rebuild everything in db/ and dump/by_file/ from the current dump/. Run from the repo root.
# The IDA steps (tools/ida/*.py, run with idat on ida/ILB.exe.i64) are separate; their order is:
#   export_all -> [assert_map] -> apply_names -> export_all force -> [excel_tables, custom_data]
#   -> apply_types -> export_all force -> retype_units -> export_all db/retype_units.json
set -e
python3 tools/assert_map.py
python3 tools/excel_tables.py > /dev/null
python3 tools/gbe_dump.py > /dev/null
python3 tools/extract_defines.py
python3 tools/assert_fields.py
python3 tools/custom_data.py
clang++ -target i686-pc-windows-msvc -fsyntax-only -Wno-everything -include db/enums.h db/types_check.cpp
python3 tools/by_file.py
python3 tools/progress.py
