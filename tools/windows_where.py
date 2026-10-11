#!/usr/bin/env python3
"""Name the functions behind addresses of the Windows build (from a crash report), using its link map.

    tools/windows_where.py 0x5a8d29 0x5b0968 ...
"""
import bisect
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
symbols = []
for line in open(os.path.join(ROOT, "build", "windows", "ILB_windows.map")):
    m = re.match(r"\s*[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{16})\s+(\S+)", line)
    if m:
        symbols.append((int(m.group(2), 16), m.group(1), m.group(3)))
symbols.sort()
for text in sys.argv[1:]:
    address = int(text, 16)
    i = bisect.bisect(symbols, (address, "~", "")) - 1
    start, name, where = symbols[i]
    print(f"{address:#x}  {name} + {address - start:#x}   ({where})")
