#!/usr/bin/env python3
"""List calls through Direct3D / DXGI interfaces whose argument count differs from the interface definition.

The decompiler writes such a call with its own cast when it could not match the pushes to the method
("((int (__stdcall *)(ID3D11Device *, void *, SIZE_T, _DWORD, int *, int, int))dev->lpVtbl->CreateVertexShader)(...)").
With the wrong count the recompiled __stdcall call leaves the stack unbalanced, so each one is a certain bug.
Usage: tools/draft_com_calls.py [-v]      (-v prints the call text)
"""
import glob
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
types = open(os.path.join(ROOT, "draft", "build", "ILB_draft_types.h")).read()
methods, by_interface = {}, {}
for m in re.finditer(r"\(__stdcall \*(\w+)\)\(([^;]*)\);", types):
    args = m.group(2).strip()
    count = 0 if args in ("", "void") else args.count(",") + 1
    methods.setdefault(m.group(1), set()).add(count)
    interface = re.match(r"(\w+) \*", args)             # the "This" parameter names the interface
    if interface:
        by_interface[(interface.group(1), m.group(1))] = {count}

CALL = re.compile(r"\(\((?:[\w\s\*]+?)\((?:__\w+ ?)?\*\)\(([^()]*)\)\)(?:[\w.]+->)*(\w+)\)")
found = 0
for path in sorted(glob.glob(os.path.join(ROOT, "draft", "build", "src", "*.c"))):
    for body in open(path).read().split("//----- (")[1:]:
        for m in CALL.finditer(body):
            args = m.group(1).strip()
            count = 0 if args in ("", "void") else args.count(",") + 1
            want = methods.get(m.group(2))
            interface = re.match(r"(\w+) \*", args)
            if interface and (interface.group(1), m.group(2)) in by_interface:
                want = by_interface[(interface.group(1), m.group(2))]
            if want and count in want and "__stdcall *)(" not in m.group(0):
                found += 1
                print("%-28s %s  %-26s is not called as __stdcall" % (os.path.basename(path), body[:8], m.group(2)))
            if want and count not in want:
                found += 1
                line = body[:m.start()].count("\n")
                print("%-28s %s  %-26s passes %d, takes %s" % (os.path.basename(path), body[:8], m.group(2), count,
                                                              " or ".join(map(str, sorted(want)))))
                if "-v" in sys.argv:
                    end = body.index(";", m.end())
                    print("      " + " ".join(body[m.start():end].split())[:400])
print("%d interface calls with a wrong argument count or calling convention" % found)
