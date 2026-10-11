#!/usr/bin/env python3
"""Name the functions in a "native: TRACE" line (draft/native/native_trace.c) of a game log.

Usage: tools/native_trace.py [log] [binary]     defaults: draft/build/native/game.log, build/native/ILB_native
Prints the functions entered before the crash, most recent first, with repeats folded.
"""
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
log = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "draft", "build", "native", "game.log")
binary = sys.argv[2] if len(sys.argv) > 2 else os.path.join(ROOT, "build", "native", "ILB_native")
for line in open(log, errors="replace"):
    m = re.match(r"native: TRACE signal (\w+) entered: (.*)", line)
    if not m:
        continue
    addresses = m.group(2).split()
    names = subprocess.run(["addr2line", "-f", "-e", binary] + ["0x" + a for a in addresses],
                           capture_output=True, text=True).stdout.splitlines()[0::2]
    folded = []
    for n in names:
        if folded and folded[-1][0] == n:
            folded[-1][1] += 1
        else:
            folded.append([n, 1])
    print("signal %d; functions entered before it (most recent first):" % int(m.group(1), 16))
    for n, count in folded[:int(os.environ.get("TRACE_DEPTH", "60"))]:
        print("   %s%s" % (n, " x%d" % count if count > 1 else ""))
