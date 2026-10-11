#!/usr/bin/env python3
"""Write what tools/link_draft.sh needs to link against the game's own DLLs.

    <out>/SDL2.def, <out>/fmod.def    export lists (from dump/imports.json) for llvm-dlltool
    <out>/alternatenames.rsp          linker options mapping the draft's names for FMOD's C++ methods
                                      (FMOD__System__init) to the DLL's real ones (?init@System@FMOD@@...)
"""
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
out = sys.argv[1]
imports = json.load(open(os.path.join(ROOT, "dump", "imports.json")))
decls = open(os.path.join(ROOT, "draft", "build", "ILB_draft_decls.h")).read()

for module in ("SDL2", "fmod"):
    with open(os.path.join(out, module + ".def"), "w") as f:
        f.write("LIBRARY %s.dll\nEXPORTS\n" % module)
        for entry in imports:
            if entry["module"] == module:
                name = entry["name"]
                m = None if name.startswith("?") else re.search(r"__stdcall %s\(([^)]*)\);" % re.escape(name), decls)
                if m:           # a __stdcall function exported without decoration (llvm-dlltool -k strips the @n)
                    params = m.group(1).strip()
                    name += "@%d" % (0 if params in ("", "void") else 4 * (params.count(",") + 1))
                f.write("    %s\n" % name)

with open(os.path.join(out, "alternatenames.rsp"), "w") as f:
    # C runtime functions whose real names start with an underscore, which IDA does not show
    for name in ("_stdio_common_vsprintf", "_stdio_common_vswprintf", "atoi64", "time64", "wgetenv"):
        f.write('"/alternatename:_%s=__%s"\n' % (name, name))
    for entry in imports:
        name = entry["name"]
        if not name.startswith("?"):
            continue
        plain = "__".join(reversed(name[1:].split("@@")[0].split("@")))
        m = re.search(r"__stdcall %s\(([^)]*)\);" % re.escape(plain), decls)
        if not m:
            print("no __stdcall prototype for", plain)
            continue
        params = m.group(1).strip()
        count = 0 if params in ("", "void") else params.count(",") + 1
        # __stdcall decoration: _name@bytes (every parameter here is 4 bytes)
        f.write('"/alternatename:_%s@%d=%s"\n' % (plain, count * 4, name))
