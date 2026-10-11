#!/usr/bin/env python3
"""Read the symbol tables of the draft's object files (llvm-nm) and report symbols nobody defines.

Usage: tools/draft_symbols.py <object dir>      (run through tools/draft_symbols.sh)
"""
import collections
import glob
import json
import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
NM = next(n for n in ("llvm-nm-14", "llvm-nm") if subprocess.run(["which", n], capture_output=True).returncode == 0)


def plain(symbol):
    """_name, @name@8, _name@8 -> name (32-bit Windows decoration)."""
    symbol = symbol.lstrip("_@")
    return symbol.split("@")[0]


defined, needed = collections.defaultdict(list), collections.defaultdict(set)
objects = sorted(glob.glob(os.path.join(sys.argv[1], "*.o")))
for path in objects:
    name = os.path.basename(path)[:-2]
    for line in subprocess.run([NM, path], capture_output=True, text=True).stdout.splitlines():
        parts = line.split()
        kind, symbol = parts[-2], parts[-1]
        if symbol.startswith(("??_C@", "__real@", "__xmm@")):       # string and float constants, merged by the linker
            continue
        if kind == "U":
            needed[plain(symbol)].add(name)
        elif kind in "TDBRC":
            defined[plain(symbol)].append(name)

imports = set()
for entry in json.load(open(os.path.join(ROOT, "dump", "imports.json"))):
    name = entry["name"]
    if name.startswith("?"):                # C++ import: ?stop@ChannelControl@FMOD@@... -> FMOD__ChannelControl__stop
        name = "__".join(reversed(name[1:].split("@@")[0].split("@")))
    imports.add(plain(name))

# C runtime and compiler support routines: supplied by the runtime library when the program is linked,
# not part of the game's source. Hand-maintained in db/runtime_symbols.json.
runtime = set(json.load(open(os.path.join(ROOT, "db", "runtime_symbols.json")))["symbols"])

duplicates = {s: f for s, f in defined.items() if len(f) > 1}
missing = {s: f for s, f in needed.items() if s not in defined and s not in imports and s not in runtime}
from_runtime = [s for s in needed if s not in defined and s in runtime]
from_imports = [s for s in needed if s not in defined and s in imports]
print("%d objects, %d symbols defined, %d needed from DLL imports, %d from the C runtime" % (
    len(objects), len(defined), len(from_imports), len(from_runtime)))
print("defined more than once: %d" % len(duplicates))
for s, f in sorted(duplicates.items())[:20]:
    print("   %s: %s" % (s, ", ".join(f)))
print("needed but defined nowhere: %d" % len(missing))
for s, f in sorted(missing.items(), key=lambda kv: -len(kv[1])):
    print("   %-44s used by %d file(s), e.g. %s" % (s, len(f), sorted(f)[0]))
sys.exit(1 if duplicates or missing else 0)
