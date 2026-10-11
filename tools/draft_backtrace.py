#!/usr/bin/env python3
"""Print the crash from a wine log of ILB_draft.exe with function names from the linker map."""
import bisect
import re
import sys

stack_mode = sys.argv[1] == "--stack"       # --stack: the log is a winedbg memory dump of a thread's stack
log, map_path = sys.argv[-2:]
symbols = []
for line in open(map_path, errors="replace"):
    m = re.match(r"\s*[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{16})", line)
    if m:
        symbols.append((int(m.group(2), 16), m.group(1)))
symbols.sort()


def name(address):
    i = bisect.bisect_right(symbols, (address, "~")) - 1
    return "%s+0x%X" % (symbols[i][1].lstrip("_"), address - symbols[i][0]) if i >= 0 else "?"


text = open(log, errors="replace").read()
if stack_mode:
    # every stack word that points into the program's code, innermost first: mostly return addresses
    code_end = next(a for a, s in symbols if s == "_ILB_image_data")
    for line in text.splitlines():
        m = re.search(r"(0x[0-9a-f]+)[^:]*:\s+(.*)$", line)
        if not m:
            continue
        for k, word in enumerate(re.findall(r"\b[0-9a-f]{8}\b", m.group(2))):
            if 0x401000 <= int(word, 16) < code_end:
                print("  %08X  %s" % (int(m.group(1), 16) + 4 * k, name(int(word, 16))))
    sys.exit(0)
m = re.search(r"Unhandled exception: ([^\n]*)", text)
if not m:
    print("no unhandled exception in the log")
    sys.exit(0)
print("CRASH:", m.group(1))
for frame, address, module in re.findall(r"^(?:=>)?\s*(\d+) 0x([0-9a-f]{8}) in (\S+)", text, re.M):
    address = int(address, 16)
    print("  %2s  %08X  %s" % (frame, address, name(address) if module == "ilb_draft" else "(%s)" % module))
