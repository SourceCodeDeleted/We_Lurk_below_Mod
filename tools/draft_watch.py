#!/usr/bin/env python3
"""Choose functions for a trace build to log every time they are entered (see draft/glue/trace.c).

Usage: tools/draft_watch.py <function name> ...      writes draft/build/steam_run/ILB_watch.txt and run/ILB_watch.txt
The addresses come from the current link map, so run it again after every link.
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
addresses = {}
for line in open(os.path.join(ROOT, "draft", "build", "ILB_draft.map"), errors="replace"):
    m = re.match(r"\s*[0-9a-f]{4}:[0-9a-f]{8}\s+(\S+)\s+([0-9a-f]{16})", line)
    if m:
        addresses[re.sub(r"^[_@]|@\d+$", "", m.group(1))] = int(m.group(2), 16)
lines = []
for name in sys.argv[1:]:
    if name in addresses:
        lines.append("%08x" % addresses[name])
    else:
        print("not in the map:", name)
for folder in ("steam_run", "run"):
    path = os.path.join(ROOT, "draft", "build", folder)
    if os.path.isdir(path):
        open(os.path.join(path, "ILB_watch.txt"), "w").write("\n".join(lines) + "\n")
print("watching %d functions" % len(lines))
