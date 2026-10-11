#!/usr/bin/env python3
"""Name the addresses in an ILB_trace.txt written by a trace build (draft/glue/trace.c).

Usage: tools/draft_trace.py <ILB_trace.txt> [draft/build/ILB_draft.map]
For every assert: the function it fired in, and the functions entered before it (most recent first).
"""
import bisect
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
map_path = sys.argv[2] if len(sys.argv) > 2 else os.path.join(ROOT, "draft", "build", "ILB_draft.map")
symbols = []
for line in open(map_path, errors="replace"):
    m = re.match(r"\s*[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{16})", line)
    if m:
        symbols.append((int(m.group(2), 16), m.group(1)))
symbols.sort()


def name(address, offset=True):
    i = bisect.bisect_right(symbols, (address, "~")) - 1
    text = re.sub(r"^[_@]|@\d+$", "", symbols[i][1])
    return "%s+0x%X" % (text, address - symbols[i][0]) if offset else text


code_end = next(a for a, s in symbols if s == "_ILB_image_data")
lines = open(sys.argv[1]).read().splitlines()
for n, line in enumerate(lines):
    m = re.match(r"CRASH code=(\w+) at=(\w+) access=(\w+) address=(\w+) image=(\w+)", line)
    if m:
        code, at, access, address, image = (int(x, 16) for x in m.groups())
        shift = image - 0x400000            # the map is for the preferred load address
        where = name(at - shift) if 0x401000 <= at - shift < code_end else "outside the program (a DLL)"
        print("CRASH %08X in %s: %s address %08X" % (code, where, {0: "reading", 1: "writing", 8: "executing"}.get(access, "?"), address))
        for extra in lines[n + 1:n + 5]:
            if extra.startswith("  frames:"):
                print("    call chain (innermost first):")
                for word in extra.split()[1:]:
                    value = int(word, 16) - shift
                    print("        " + (name(value) if 0x401000 <= value < code_end else "%08X (not in the program)" % (value + shift)))
                continue
            if extra.startswith("  eax="):
                print("   " + extra.strip())
            if extra.startswith("  entered:"):
                seen = []
                for word in extra.split()[1:]:
                    f = name(int(word, 16) - shift, False)
                    if not seen or seen[-1] != f:
                        seen.append(f)
                print("    entered before it: " + " <- ".join(seen[:16]))
            if extra.startswith("  stack:") and not any(l.startswith("  frames:") for l in lines[n + 1:n + 5]):
                print("    code addresses on the stack (innermost first; mostly return addresses):")
                shown = 0
                for word in extra.split()[1:]:
                    value = int(word, 16) - shift
                    if 0x401000 <= value < code_end and shown < 16:
                        print("        " + name(value))
                        shown += 1
# samples: consecutive ones at the same place are counted
previous, repeat = None, 0
def flush_sample():
    if previous:
        print("SAMPLE x%d  %s" % (repeat, previous))
for line in lines:
    m = re.match(r"SAMPLE calls=(\w+) at=(\w+) frames:(.*) entered:(.*)", line)
    if m:
        at = int(m.group(2), 16)
        chain = [name(int(w, 16)) for w in m.group(3).split() if 0x401000 <= int(w, 16) < code_end]
        here = name(at) if 0x401000 <= at < code_end else "(in a DLL)"
        entered_now = []
        for w in m.group(4).split():
            f = name(int(w, 16), False)
            if f not in entered_now:
                entered_now.append(f)
        text = "at %s | called from: %s | %s | recently entered: %s" % (
            here, " <- ".join(chain[:7]) or "?", "busy" if int(m.group(1), 16) else "NO function calls since last sample", ", ".join(entered_now[:6]))
        if text == previous:
            repeat += 1
        else:
            flush_sample()
            previous, repeat = text, 1
        continue
flush_sample()
for line in lines:
    m = re.match(r"ENTER (\w+) from (\w+) args (.*)", line)
    if m:
        print("ENTER %-28s from %-36s args %s" % (name(int(m.group(1), 16), False), name(int(m.group(2), 16)), m.group(3)))
        continue
    m = re.match(r"ASSERT from (\w+) after (\w+) calls; entered before it:(.*)", line)
    if not m:
        continue
    before, last = [], None
    for word in m.group(3).split():
        n = name(int(word, 16), False)
        if n != last:
            before.append(n)
        last = n
    print("ASSERT in %s  (after %d calls)" % (name(int(m.group(1), 16)), int(m.group(2), 16)))
    print("    before: " + " <- ".join(before[:14]))
