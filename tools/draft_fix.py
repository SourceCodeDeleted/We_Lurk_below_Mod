#!/usr/bin/env python3
"""Turn IDA's whole-program export into C that compiles, split into the original source files.

    draft/ILB_raw.c (+ ILB_raw_extra.c), draft/ILB_types.h (+ ILB_types_d3d.h)
        ->  draft/build/src/<file>.c        one per original source file (db/files.json)
            draft/build/ILB_draft_types.h   every type
            draft/build/ILB_draft_decls.h   every prototype; every global as a macro at its original address
            draft/build/ILB_defs.h          IDA's helper header
            draft/build/manifest.json       function address -> file, name, hand-written or not

Only mechanical rewrites are done here, each one a named rule below, so a fresh export can be re-run
through the same script. Nothing in draft/build/ is edited by hand. Hand-written support code is in
draft/include/. Afterwards: tools/draft_data.py (data image), tools/check_draft.sh, tools/draft_symbols.sh.
"""
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RAW = os.path.join(ROOT, "draft", "ILB_raw.c")
TYPES = os.path.join(ROOT, "draft", "ILB_types.h")
OUT = os.path.join(ROOT, "draft", "build")
RUNTIME_TAIL = 0x582B00     # C runtime and compiler support code starts here; it is not reconstructed

index = json.load(open(os.path.join(ROOT, "dump", "index.json")))
library = {f["ea"] for f in index if f["lib"] or f["ea"] >= RUNTIME_TAIL}


def sanitize(text):
    """C++-only spellings in names and declarations that have a direct C equivalent."""
    text = re.sub(r"\b(\w+)::(?=[\w$])", r"\1__", text)                 # A::B -> A__B
    text = re.sub(r"\benum __bitmask\b", "enum", text)
    text = re.sub(r"__spoils<[^>]*>\s*", "", text)
    text = text.replace("__hidden ", "")
    text = re.sub(r"\boperator (new|delete)(\[\])?", lambda m: "operator_%s%s" % (m.group(1), "_array" if m.group(2) else ""), text)
    return text


# ---------------------------------------------------------------------------------------------- types
types = sanitize(open(TYPES, errors="replace").read())

# rule: library types the decompiler used without copying them into the database's own type list (Direct3D
# and DXGI interfaces; exported by tools/ida/export_named_types.py) are appended, minus the ones both have
BLOCK_NAME = re.compile(r"^(?:typedef\s+)?(struct|union|enum)\s+(?:__declspec\((?:[^()]|\([^()]*\))*\)\s*|__\w+\s+)*([A-Za-z_$][\w$]*)\s*(?:\{|;|:)|^typedef\s[^;{]*?\b([A-Za-z_$][\w$]*)\s*(?:\)\([^;]*\))?;", re.S)
def blocks_of(text):
    for block in re.split(r"\n\s*\n", text):
        body = re.sub(r"^/\*.*?\*/\s*", "", block.strip(), flags=re.S)
        m = BLOCK_NAME.match(body)
        yield block, (m.group(2) or m.group(3), "{" in body) if m else None
have = {key for _, key in blocks_of(types) if key}
extra = sanitize(open(os.path.join(ROOT, "draft", "ILB_types_d3d.h"), errors="replace").read())
types += "\n\n" + "\n\n".join(block for block, key in blocks_of(extra) if key is None or key not in have) + "\n"

# rule: the SSE register types come from draft/include/ILB_prelude.h (with extra members that make IDA's
# casts between them and integers valid), not from the export
types = re.sub(r"^union (?:__declspec\(align\(16\)\) )?__m128i?\s*\{[^}]*\};\n", "", types, flags=re.M)
types = re.sub(r"^#define __int(8|16|32|64) .*\n", "", types, flags=re.M)

# every struct/union tag also becomes a plain type name, as the function bodies use them that way
tags = []
TAG_RE = re.compile(r"^(struct|union|enum)\s+(?:__declspec\((?:[^()]|\([^()]*\))*\)\s*|__(?:cppobj|unaligned|packed)\s+)*([A-Za-z_$][\w$]*)", re.M)
for kind, name in TAG_RE.findall(types):
    if (kind, name) not in tags:
        tags.append((kind, name))
existing_typedef = set(re.findall(r"^typedef\s+[^;{]*?\b([A-Za-z_$][\w$]*)\s*;", types, flags=re.M))
forward = []
for kind, name in tags:
    if kind != "enum" and name not in existing_typedef:
        forward.append("%s %s; typedef %s %s %s;" % (kind, name, kind, name, name))
# enums: the typedef follows the definition
types = re.sub(r"^(enum\s+([A-Za-z_$][\w$]*)[^;{]*\{[^}]*\};)",
               lambda m: m.group(1) + ("" if m.group(2) in existing_typedef else "\ntypedef enum %s %s;" % (m.group(2), m.group(2))),
               types, flags=re.M)

# rule: a table-row enum constant from db/enums.h that has the same name as a function (the emitters table
# row "warrior_dash" vs the function emitter_warrior_dash) is renamed with an _id suffix
raw_text = open(RAW, errors="replace").read()
function_names = set(re.findall(r"^[\w \*]+?\b(\w+)\([^;{]*\);", raw_text[:raw_text.index("// Data declarations")], flags=re.M))
renamed_constants = 0
def rename_constant(m):
    global renamed_constants
    if m.group(1) in function_names:
        renamed_constants += 1
        return "  %s_id =" % m.group(1)
    return m.group(0)
types = re.sub(r"^  (\w+) =", rename_constant, types, flags=re.M)

# rule: a simple typedef that draft/include/ILB_prelude.h already provides is not repeated
prelude_text = open(os.path.join(ROOT, "draft", "include", "ILB_prelude.h")).read()
prelude_names = set(re.findall(r"^typedef\s+[^;{]*?\b([A-Za-z_$][\w$]*)\s*;", prelude_text, flags=re.M))
types = re.sub(r"^typedef\s+[^;{()]*?\b([A-Za-z_$][\w$]*)\s*;\n",
               lambda m: "" if m.group(1) in prelude_names else m.group(0), types, flags=re.M)

os.makedirs(OUT, exist_ok=True)
with open(os.path.join(OUT, "ILB_draft_types.h"), "w") as f:
    f.write("// Generated by tools/draft_fix.py from IDA's type export. Do not edit.\n#pragma once\n")
    f.write('#include "ILB_prelude.h"\n\n')
    f.write("\n".join(forward) + "\n\n")
    f.write(types)

# --------------------------------------------------------------------------------------------- program
raw = sanitize(raw_text)
head, sep, rest = raw.partition("//----- (")
bodies = (sep + rest).split("//----- (")
bodies = [b for b in bodies if b]

# rule: draft/ILB_raw_extra.c (tools/ida/add_functions.py) holds functions decompiled after the export:
# ones IDA had not recognised, and re-decompiled ones. They replace or join the export's bodies.
extra_path = os.path.join(ROOT, "draft", "ILB_raw_extra.c")
extra_bodies = {}
if os.path.exists(extra_path):
    for b in sanitize(open(extra_path, errors="replace").read()).split("//----- (")[1:]:
        extra_bodies[int(b[:8], 16)] = b.rstrip("\n") + "\n\n"
bodies = [extra_bodies.pop(int(b[:8], 16), b) for b in bodies] + list(extra_bodies.values())
bodies.sort(key=lambda b: int(b[:8], 16))

kept, dropped = [], 0
for b in bodies:
    ea = int(b[:8], 16)
    if ea in library:
        dropped += 1
        continue
    kept.append("//----- (" + b)

RAW_HEADER = re.compile(r"^(?!//)([^\n;{}]*?\b([A-Za-z_$][\w$]*)(?:@<\w+>)?\(([^;{}]*)\))\n\{", re.M)
original_headers = {}
for b in kept:
    m = RAW_HEADER.search(b)
    if m:
        original_headers[int(b[9:17], 16)] = m

# rule: IDA's __usercall / __userpurge notation is not C; the parameter list is kept, the register
# annotations are dropped. Valid because caller and callee are both recompiled from this file.
def plain_prototypes(text):
    text = re.sub(r"@<[^>]*>", "", text)
    return re.sub(r"\b__user(call|purge)\b", "__cdecl", text)


head = plain_prototypes(head)
kept = [plain_prototypes(b) for b in kept]

# rule: declarations of compiler-generated C++ members (`vftable', `scalar deleting destructor') cannot be
# written in C and belong to the runtime; drop them
head = "".join(line for line in head.splitlines(True) if "`" not in line)

# rule: the export comments out the imported functions' declarations
head = re.sub(r"^// extern ", "extern ", head, flags=re.M)

# rule: the export also comments out prototypes it could not express in standard C (register arguments)
# and ones it only guessed ("weak"); after the rule above they are ordinary prototypes
head = re.sub(r"^// ((?!extern )[A-Za-z_][^;{}/]*\([^;{}]*\);)(?: weak| idb)?[ \t]*$", r"\1", head, flags=re.M)
head = re.sub(r"; (?:weak|idb)[ \t]*$", ";", head, flags=re.M)

# rule: the math imports take and return a double (tools/ida/retype_libm.py gave IDA the same prototype;
# the export's own guess was a vector register and no argument)
head = re.sub(r"^[^\n;()]*\b(libm_sse2_(\w+)_precise)\([^\n;]*\);", lambda m: "double %s(double x%s);" % (
    m.group(1), ", double y" if m.group(2) in ("pow", "atan2") else ""), head, flags=re.M)

# rule: 16-byte integers do not exist on this target; they are 16-byte blocks here
head = re.sub(r"\b(?:unsigned )?__int128\b", "_OWORD", head)
kept = [re.sub(r"\b(?:unsigned )?__int128\b", "_OWORD", b) for b in kept]

# rule: headers are replaced by our own
head = head.replace("#include <windows.h>\n", "").replace("#include <math.h>\n", "")
head = head.replace("#include <defs.h>\n", '#include "ILB_draft_types.h"\n#include "ILB_defs.h"\n#include "ILB_helpers.h"\n')

# rule: IDA's helper header (LOBYTE, __PAIR64__, qmemcpy, ...) is used as shipped, except that its
# 4-byte "typedef int bool" is removed: the exported struct layouts need the 1-byte bool.
ida_include = os.environ.get("IDA_INCLUDE", os.path.expanduser("~/ida-home-pc-9.4/include"))
defs = open(os.path.join(ida_include, "defs.h")).read()
defs = defs.replace("typedef int bool;", "// typedef int bool;  (removed by tools/draft_fix.py)")
with open(os.path.join(OUT, "ILB_defs.h"), "w") as f:
    f.write("// Copy of IDA's defs.h made by tools/draft_fix.py, with one line removed. Do not edit.\n#define _WINDOWS_\n" + defs)

# ------------------------------------------------------------------------------------------------ data
# rule: every global becomes a macro into one image-sized array at its original address (plan step 4), so
# code that runs off the end of one symbol into the next still reads what the binary reads.
names = {n["name"]: n["ea"] for n in json.load(open(os.path.join(ROOT, "dump", "names.json")))}
names.update({sanitize(k): v for k, v in list(names.items())})
decl_head, _, data_section = head.partition("// Data declarations")
ADDRESS_STYLE = re.compile(r"^(?:dword|byte|word|qword|xmmword|flt|dbl|unk|off|stru|asc|loc|a[A-Z]\w*|tbyte)_?[0-9A-F]{5,8}$")

statements, cur, depth = [], "", 0
LITERAL = re.compile(r'"(?:[^"\\\\]|\\\\.)*"|\'(?:[^\'\\\\]|\\\\.)*\'')
for line in data_section.splitlines():
    cur += line + "\n"
    code = LITERAL.sub("", line)            # braces inside string and character constants do not nest
    stripped = re.sub(r"\s*//.*$", "", code)
    depth += code.count("{") - code.count("}")
    if depth <= 0 and stripped.rstrip().endswith(";"):
        statements.append(cur)
        cur, depth = "", 0

# rule: globals whose exported type is a guess that later decompilation contradicts. Both are "load" slots
# of the app mode table, like off_609970, and the functions they point to take the mode's argument.
DATA_TYPE_FIXES = {
    "off_609940": "char (__cdecl *off_609940)(int a1)",
    "off_609AF0": "char (__cdecl *off_609AF0)(int a1)",
    # a table of angles (pi/4, pi/2, ...), indexed as floats by the re-decompiled user
    "dword_5875A4": "float dword_5875A4[]",
    # exported as "_UNKNOWN" (one byte) but used whole; the type is what the uses need:
    "unk_38C7BB8": "char unk_38C7BB8[296]",     # the options menu's copy of the 296-byte config (qmemcpy in and out)
    "unk_611440": "float unk_611440",           # read and written through float locals
    "unk_5CA224": "int unk_5CA224",             # 1800, stored into int fields
    "unk_5975A0": "_OWORD unk_5975A0",          # 16-byte pieces of a string constant, copied as blocks
    "unk_5975B0": "_OWORD unk_5975B0",
    "unk_5975C0": "_OWORD unk_5975C0",
}
# rule: a "global" in the uninitialised part of the image whose address no relocated field holds is not a
# global at all: it is a plain number (an offset into the heap-allocated world, 0x3931820) that IDA gave a
# name because it lies inside the address range. Its macro yields the number itself instead of a place in
# the image, so "(char *)dword_3931820 + world" is world + 0x3931820 as in the original.
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from pe_relocs import Image
pe_image = Image()
reloc_values = [pe_image.value_at(slot) for slot in pe_image.slots]
relocated_anywhere = set(reloc_values)
INITIALISED_END = 0x609E00
plain_number_globals = 0
def place(ea):
    global plain_number_globals
    if ea >= INITIALISED_END and ea not in relocated_anywhere:
        plain_number_globals += 1
        return "(0x%X)" % ea
    return "ILB_ADDR(0x%X)" % ea
data_lines, externs, global_names, unplaced = [], [], {}, []
DECL_RE = re.compile(r"^(?P<pre>[^=;]*?[\s\*\(])(?P<name>[A-Za-z_$?@][\w$?@]*)(?P<post>(?:\[[^\]]*\])*(?:\)\([^;=]*\))?)\s*(?:=|;)", re.S)
for st in statements:
    text = re.sub(r"//.*", "", st).strip()
    if not text:
        continue
    for name, declaration in DATA_TYPE_FIXES.items():
        if re.search(r"[\s\*\(]%s[\)\[; =]" % re.escape(name), text.split("=")[0] + ";"):
            text = declaration + ";"
    is_extern = text.startswith("extern ")
    if is_extern:
        text = text[len("extern "):]
    m = DECL_RE.match(text)
    if is_extern and not (m and re.match(r"^[a-z]+_[0-9A-F]{6,8}$", m.group("name"))):
        # an imported function: IDA shows the import table slot (a pointer); calls are written directly
        externs.append(re.sub(r"\((__\w+) \*([\w$]+)\)\(", r"\1 \2(", text.split("\n")[0]) + "\n")
        continue
    if m and m.group("name") not in names:
        # IDA's placeholder names (dword_60A028, loc_4407EC) are not in its name list; the address is the name
        dummy = re.match(r"^[a-z]+_([0-9A-F]{6,8})$", m.group("name"))
        if dummy:
            names[m.group("name")] = int(dummy.group(1), 16)
    if not m or m.group("name") not in names:
        unplaced.append(text.split("\n")[0][:100])
        continue
    name, ea = m.group("name"), names[m.group("name")]
    unique = name if ADDRESS_STYLE.match(name) else "%s_%X" % (name, ea)
    global_names[name] = unique
    pre = m.group("pre").replace("__int128", "_OWORD")
    data_lines.append("typedef %s%s__t%s;\n#define %s (*(%s__t *)%s)\n" % (pre, unique, m.group("post"), unique, unique, place(ea)))

# rule: a placeholder-named global that the bodies use but the export did not declare gets the type its
# prefix states (dword_ = int, flt_ = float, ...)
PREFIX_TYPES = {"dword": "int", "word": "__int16", "byte": "char", "qword": "__int64", "xmmword": "_OWORD", "flt": "float",
                "dbl": "double", "off": "void *", "unk": "_UNKNOWN", "stru": "_UNKNOWN", "asc": "char", "loc": "_UNKNOWN"}
# ... except where the uses say otherwise: the material type table (14 rows of 6 function pointers) is
# indexed in bytes, "&off_5D42C8[24 * a1]", so IDA's type for it is a byte array despite the off_ name
FALLBACK_ELEMENT = {"off_5D42C8": "char"}
undeclared = set()
for b in kept:
    for prefix, address in re.findall(r"(?<![\w$])(dword|word|byte|qword|xmmword|flt|dbl|off|unk|stru|asc|loc)_([0-9A-F]{6,8})(?![\w$])", b):
        name = "%s_%s" % (prefix, address)
        if name not in global_names:
            undeclared.add(name)
            global_names[name] = name
            if re.search(r"(?<![\w$])%s\[" % name, b):         # used as an array
                data_lines.append("#define %s ((%s *)ILB_ADDR(0x%s))    // not declared by the export\n" % (
                    name, FALLBACK_ELEMENT.get(name, PREFIX_TYPES[prefix]), address))
            else:
                data_lines.append("#define %s (*(%s *)ILB_ADDR(0x%s))    // not declared by the export\n" % (name, PREFIX_TYPES[prefix], address))

# rule: a string constant that only the later-decompiled functions use (so the export did not declare it)
# is declared as a character array; whether it is wide is not known here
for b in kept:
    code = re.sub(r'//[^\n]*|"(?:[^"\\\n]|\\.)*"', "", b)
    for name in set(re.findall(r"(?<![\w$.>])(a[A-Z]\w*)(?![\w$(])", code)):
        if name in names and name not in global_names and 0x584000 <= names[name]:
            global_names[name] = name
            undeclared.add(name)
            data_lines.append("#define %s ((char *)ILB_ADDR(0x%X))    // not declared by the export\n" % (name, names[name]))

# rule: single places where the export is not valid C and no general rule applies; each is listed here with
# the function it is in, so a re-export that changes the text is noticed (the count is printed)
SITE_FIXES = [
    # the old-format unit loaders walk a byte buffer; the database typed the cursor as an undefined struct
    (0x57D810, "unit_save_data_v0", "unsigned __int8"),
    (0x57E990, "unit_save_data_v0", "unsigned __int8"),
    # differences of pointers to different character types
    (0x5341A0, "v22 = v18 - v25;", "v22 = (char *)v18 - (char *)v25;"),
    (0x530CA0, "(char *)this - (_BYTE *)dword_38C793C", "(char *)this - (char *)dword_38C793C"),
    (0x500470, "(char *)v40 + (char *)a6 - a5)", "(char *)v40 + ((char *)a6 - (char *)a5))"),
    # draw_char returns the width of the character it drew, in xmm0. For a space it returns straight after
    # computing "scale * width"; the decompiler shows the scale argument being returned instead, so spaces
    # had no width and words ran together. The other exit of the function returns the same variable.
    (0x54B460, "  if ( a2 == 32 )\n    return a5;", "  if ( a2 == 32 )\n    return LODWORD(v14);"),
    # sign tests the decompiler wrote on unsigned values ("x - y < 0", always false in C; the original tests
    # the sign bit). Found with clang's -Wtautological-unsigned-zero-compare; the counter form
    # "--*(_DWORD *)p < 0" has its own rule further down.
    (0x4416B0, "if ( *v6 - *(v6 - 2) >= 0 )", "if ( (int)(*v6 - *(v6 - 2)) >= 0 )"),
    (0x4978B0, "v18 = v41 + *(_DWORD *)v15 < 0;", "v18 = (int)(v41 + *(_DWORD *)v15) < 0;"),
    (0x4F9960, "v6 = v1[23] - 60 < 0;", "v6 = (int)(v1[23] - 60) < 0;"),
    (0x578FD0, "if ( *((_DWORD *)v6 + 563) - *((_DWORD *)v6 + 563) * edx0->armor_penetration / 100 >= 0 )",
               "if ( (int)(*((_DWORD *)v6 + 563) - *((_DWORD *)v6 + 563) * edx0->armor_penetration / 100) >= 0 )"),
    # dungeon_till: edx holds the tile's type across the same call (mov edx,[tile+0Ch]; mov ecx,edx; call;
    # imul ecx,edx,98h in the original)
    (0x438120, "  v9 = v8;\n", "  v9 = *(_DWORD *)(v6 + dword_38854F4 + 12);\n"),
    # check_dirt_to_grass: the same lost register as in dungeon_init_location just below (the tile array's
    # base, kept in edx across the call to sub_437300; IDA: "variable 'v13' is possibly undefined")
    (0x4373E0, "*(_DWORD *)(v13 + v12) = 1;", "*(_DWORD *)(dword_38854F4 + v12) = 1;"),
    # dungeon_init_location: the tile array's base stays in edx across a call and the decompiler lost it
    # (IDA: "variable 'v17' is possibly undefined"); every other access in the function uses the global
    (0x433E20, "*(_DWORD *)(v9 + v17) |= 0x8000u;", "*(_DWORD *)(v9 + dword_38854F4) |= 0x8000u;"),
    (0x433E20, "if ( *(_DWORD *)(v11 + v17 + 12) == 91 )", "if ( *(_DWORD *)(v11 + dword_38854F4 + 12) == 91 )"),
    # the LZW decompressor: the decompiler lost the register holding the source pointer across the call that
    # resets the table. The first code is the first 16-bit word of the source (the loop continues at index 1).
    (0x4050B0, "  v5 = *v4;\n  v6 = 1;", "  v5 = *(unsigned __int16 *)a1;\n  v6 = 1;"),
]
site_fixes_applied = 0
for n, b in enumerate(kept):
    for ea, old_text, new_text in SITE_FIXES:
        if int(b[9:17], 16) == ea and old_text in b:
            kept[n] = b = b.replace(old_text, new_text)
            site_fixes_applied += 1

# rule: calls through Direct3D / DXGI interfaces get the number of arguments the interface defines. Where the
# compiler pushed arguments of a later call early, the decompiler gives them all to the first call
# ("Map" with 9 arguments instead of 6, the following "Unmap" with none), and the recompiled __stdcall call
# then leaves the stack unbalanced. Too many: the extra trailing ones are dropped (they are the enclosing
# function's own stack arguments, read by mistake). Too few: the missing ones are listed here by hand, from
# the hand-written versions of the same functions in src/ (tools/draft_com_calls.py lists what is left).
COM_METHODS, COM_BY_INTERFACE = {}, {}
for m in re.finditer(r"\(__stdcall \*(\w+)\)\(([^;]*)\);", types):
    com_args = m.group(2).strip()
    com_count = 0 if com_args in ("", "void") else com_args.count(",") + 1
    COM_METHODS.setdefault(m.group(1), set()).add(com_count)
    interface = re.match(r"(\w+) \*", com_args)          # the "This" parameter names the interface
    if interface:
        COM_BY_INTERFACE[(interface.group(1), m.group(1))] = {com_count}
def com_counts(method, cast_types):
    """Argument counts the method can have; exact when the cast's first parameter names the interface."""
    interface = re.match(r"(\w+) \*", cast_types[0]) if cast_types else None
    return COM_BY_INTERFACE.get((interface.group(1), method)) if interface and (interface.group(1), method) in COM_BY_INTERFACE else COM_METHODS.get(method)
COM_VOID = {"Draw", "DrawIndexed", "Unmap"}
COM_MISSING = {     # (function, method): ("append" | "replace", argument text)
    (0x40C930, "Present"): ("append", "0"),                                           # flags
    (0x40DD90, "CreatePixelShader"): ("append", "0, &dword_383DF7C[2 * a1]"),         # class linkage, &shaders[index].pixel_shader
    (0x411CA0, "CreateShaderResourceView"): ("append", "0, (char *)&dword_38546CC[v7] + 4"),   # desc, &textures[index].resource_view
    (0x4115D0, "Draw"): ("append", "*(int *)ILB_ADDR(0x3846048 + 48 * a2), 0"),       # models[id].num_vertices, start
    (0x4115D0, "DrawIndexed"): ("replace", "v7, *(int *)ILB_ADDR(0x3846050 + 48 * a2), 0, 0"),  # models[id].num_indices
    (0x40F2C0, "Unmap"): ("replace", "ppImmediateContext, show_stack, 0"),            # the buffer just mapped (the context variable is lost too)
    (0x40F580, "Unmap"): ("replace", "v7, a4, 0"),
    (0x4106E0, "Unmap"): ("replace", "v6, a2, 0"),
    (0x412520, "Unmap"): ("replace", "v10, v9, 0"),
    (0x4100C0, "Unmap"): ("replace", "v7, a4, 0"),
}
COM_CALL = re.compile(r"\(\(((?:[\w\s\*]+?)\((?:__\w+ ?)?\*\)\()([^()]*)\)\)((?:[\w.]+->)*(\w+))\)\(")
com_fixed, com_left = 0, []
def split_arguments(text):
    parts, depth, cur = [], 0, ""
    for ch in text:
        if ch == "," and depth == 0:
            parts.append(cur.strip())
            cur = ""
            continue
        depth += ch in "([{"
        depth -= ch in ")]}"
        cur += ch
    if cur.strip():
        parts.append(cur.strip())
    return parts
def fix_com_calls(b):
    global com_fixed
    ea, at, out = int(b[9:17], 16), 0, []
    for m in COM_CALL.finditer(b):
        cast_types = split_arguments(m.group(2)) if m.group(2).strip() not in ("", "void") else []
        want = com_counts(m.group(4), cast_types)
        if not want or len(cast_types) in want or m.start() < at:
            continue
        depth, end = 1, m.end()
        while depth:
            depth += b[end] == "("
            depth -= b[end] == ")"
            end += 1
        arguments = split_arguments(b[m.end():end - 1])
        fewer = [n for n in want if n < len(cast_types)]
        if fewer:
            keep = max(fewer)
            text = "((%s%s))%s)(%s)" % (m.group(1), ", ".join(cast_types[:keep]), m.group(3), ", ".join(arguments[:keep]))
        elif (ea, m.group(4)) in COM_MISSING:
            mode, extra = COM_MISSING[(ea, m.group(4))]
            call = "%s(%s)" % (m.group(3), extra if mode == "replace" else ", ".join(arguments + [extra]))
            text = "(%s, 0)" % call if m.group(4) in COM_VOID else "(%s)" % call
        else:
            com_left.append("%08X %s" % (ea, m.group(4)))
            continue
        out.append(b[at:m.start()] + text)
        at = end
        com_fixed += 1
    return "".join(out) + b[at:]
kept = [fix_com_calls(b) for b in kept]

# rule: interface methods are __stdcall whatever the decompiler's cast says (a tail call is shown as __cdecl,
# and then the recompiled caller removes the arguments a second time)
com_conventions = 0
def stdcall_methods(m):
    global com_conventions
    if m.group(4) not in COM_METHODS or "__stdcall *)(" in m.group(1):
        return m.group(0)
    com_conventions += 1
    return "((" + re.sub(r"\((?:__\w+ ?)?\*\)\($", "(__stdcall *)(", m.group(1)) + m.group(2) + "))" + m.group(3) + ")("
kept = [COM_CALL.sub(stdcall_methods, b) for b in kept]

# rule: Direct3D is not used. Every call through a Direct3D / DXGI interface becomes a call of the function
# with the same name and arguments in the OpenGL backend (port/gge_gl_backend.h): "ctx->lpVtbl->Map(ctx, ...)"
# and the cast forms become "gge_gl_Map(ctx, ...)". The device creation call is replaced the same way, and
# what is left of the interface tables (a pointer to one kept in a local) is dropped.
BACKEND = set(re.findall(r"\bgge_gl_([A-Z]\w*)\(", open(os.path.join(ROOT, "port", "gge_gl_backend.h")).read()))
backend_calls, backend_missing = 0, set()
PLAIN_COM = re.compile(r"(?<![\w$.>])(?:[\w.]+->)+(%s)\(" % "|".join(sorted(COM_METHODS, key=len, reverse=True)))
def to_backend(b):
    global backend_calls
    def cast_call(m):
        global backend_calls
        if m.group(4) not in COM_METHODS:
            return m.group(0)
        if m.group(4) not in BACKEND:
            backend_missing.add(m.group(4))
            return m.group(0)
        backend_calls += 1
        return "gge_gl_%s(" % m.group(4)
    def plain_call(m):
        global backend_calls
        if m.group(1) not in BACKEND:
            backend_missing.add(m.group(1))
            return m.group(0)
        backend_calls += 1
        return "gge_gl_%s(" % m.group(1)
    b = COM_CALL.sub(cast_call, b)
    b = PLAIN_COM.sub(plain_call, b)
    # objects whose type IDA did not know are released through the raw table: (*(... **)(*(_DWORD *)x + 8))(x)
    # (whatever calling convention and argument list the decompiler guessed; Release takes the object only)
    while True:
        m = re.search(r"\(\*\([\w ]+\(__\w+ \*\*\)\([^()]*\)\)\((?:\*\(_DWORD \*\)|\*\*\(_DWORD \*\*\))[^;()]+ \+ 8\)\)\(", b)
        if not m:
            break
        depth, end = 1, m.end()
        while depth:
            depth += b[end] == "("
            depth -= b[end] == ")"
            end += 1
        b = b[:m.start()] + "gge_gl_Release(" + split_arguments(b[m.end():end - 1])[0] + ")" + b[end:]
        backend_calls += 1
    b = b.replace("D3D11CreateDeviceAndSwapChain(", "gge_gl_create_device(")
    return re.sub(r"(?<![\w$.>])[\w.]+->lpVtbl\b(?!\s*->)", "0", b)
kept = [to_backend(b) for b in kept]

# rule: IDA casts 8-byte structs to and from 64-bit integers, and vector registers to each other, when a
# value travels through a register pair; these are bit copies (macros in draft/include/ILB_helpers.h)
def register_casts(b):
    b = re.sub(r"= \((vector2d|DXGI_RATIONAL)\)(.+);$", r"= ILB_FROM_INT64(\1, \2);", b, flags=re.M)
    b = re.sub(r"= \(__int64\)(v\d+|stru_[0-9A-F]+);$", r"= ILB_AS_INT64(\1);", b, flags=re.M)
    b = re.sub(r"= \(__m128\)(\w+(?:\([^()]*\))?);$", r"= ILB_TO_M128(\1);", b, flags=re.M)
    vectors = dict((name, kind) for kind, name in re.findall(r"^  (__m128i?) (v\d+); //", b, flags=re.M))
    if vectors:
        b = re.sub(r"^(\s+)(v\d+) = 0;$", lambda m: "%s%s = (%s){ 0 };" % (m.group(1), m.group(2), vectors[m.group(2)])
                   if m.group(2) in vectors else m.group(0), b, flags=re.M)
    return b
kept = [register_casts(b) for b in kept]

# rule: stack variables keep their original positions relative to each other. The decompiler shows one
# structure or buffer on the stack as several separate variables ("_BYTE v6[4]; int v7; int v8;" for a
# 20-byte SDL_DisplayMode that a callee fills through &v6), and block operations run from one variable into
# the next. A compiler is free to place separate locals anywhere, so each function's stack variables become
# members of one union, each at the offset IDA reports ([esp+8h]); the names stay usable through macros.
STACK_LOCAL = re.compile(r"^  (?P<type>[^;\n()]*?[\s\*])(?P<name>[A-Za-z_]\w*)(?P<dims>(?:\[[^\]\n]*\])*); // \[esp\+(?P<esp>[0-9A-F]+)h\](?: \[ebp(?P<sign>[+-])(?P<ebp>[0-9A-F]+)h\])?[^\n]*\n", re.M)
stack_functions = 0
def keep_stack_layout(b):
    global stack_functions
    found = [m for m in STACK_LOCAL.finditer(b) if m.group("sign") != "+"]      # ebp+n is the argument area
    if not found:
        return b
    base = min(int(m.group("esp"), 16) for m in found)
    members, names_here = [], []
    for m in found:
        offset = int(m.group("esp"), 16) - base
        pad = "char ILB_pad[%d]; " % offset if offset else ""
        members.append("    struct __attribute__((packed)) { %s%sv%s; } m_%s;      // esp+%Xh\n" % (
            pad, m.group("type"), m.group("dims"), m.group("name"), int(m.group("esp"), 16)))
        names_here.append(m.group("name"))
    # the union also spans the rest of the original frame: a variable is often only the first field of a
    # larger structure whose other fields this function never names ("char v6[8]" for a room_definition
    # that a callee fills in completely). With a frame pointer the distance to the top of the frame is
    # known ([ebp-N]); without one, a fixed allowance is used.
    tops = [int(m.group("esp"), 16) + int(m.group("ebp"), 16) for m in found if m.group("ebp")]
    highest = max(int(m.group("esp"), 16) for m in found)
    span = (max(tops) if tops else highest + 256) - base
    members.append("    char ILB_frame[%d];      // the original frame up to %s\n" % (
        span, "its frame pointer" if tops else "256 bytes past the last named variable (no frame pointer)"))
    block = "  union\n  {\n" + "".join(members) + "  } ILB_stack;\n"
    out, at = [], 0
    for n, m in enumerate(found):
        out.append(b[at:m.start()])
        if n == 0:
            out.append("\0")                # placeholder: the union must not be touched by the renaming below
        at = m.end()
    out.append(b[at:])
    b = "".join(out)
    # every use of the variable (not a struct member of the same name, not text in a string or comment)
    use = re.compile(r'"(?:[^"\\\n]|\\.)*"|\'(?:[^\'\\\n]|\\.)*\'|//[^\n]*|(?<![\w$.>])(?<!struct )(?<!union )(?<!enum )(%s)(?![\w$])' % "|".join(map(re.escape, names_here)))
    b = use.sub(lambda m: "ILB_stack.m_%s.v" % m.group(1) if m.group(1) else m.group(0), b)
    stack_functions += 1
    return b.replace("\0", block)

# rule: "arguments" in ebx, esi, edi or ebp are removed. Every function preserves those registers, so a
# caller never loads them on purpose; the decompiler lists them where it mis-tracked the stack, and the extra
# parameters shift the real ones for any code that reaches the function through a pointer (callback tables,
# button handlers). The parameter is dropped from the definition (it stays as an unset local, in case the
# body mentions it), the matching argument is dropped from every direct call, and where a function hands
# one of its own dropped parameters on through a function pointer, that argument and its place in the
# pointer's type are dropped too.
CALLEE_SAVED = ("ebx", "esi", "edi", "ebp")
SIGNATURE = re.compile(r"^(?!//)([^\n;{}]*?\b([A-Za-z_$][\w$]*)\([^;{}]*\))\n\{", re.M)

# ... and for a function that is reached through a pointer (its address is stored in a table or taken in
# code: button handlers, mode callbacks, material callbacks), every register "argument" goes the same way.
# Such a function is an ordinary C callback in the original; its callers push the arguments and set no
# registers, so a listed ecx/edx/xmm parameter cannot be real, and keeping it shifts the real ones.
function_starts = {int(b[9:17], 16) for b in kept}
address_taken = {v for slot, v in zip(pe_image.slots, reloc_values)
                 if v in function_starts and (pe_image.section_of(slot) or {}).get("name") in (".rdata", ".data")}
names_by_ea = {}
for b in kept:
    sig = SIGNATURE.search(b)
    if sig:
        names_by_ea[int(b[9:17], 16)] = sig.group(2)
ea_by_name = {v: k for k, v in names_by_ea.items()}
taken_in_code = re.compile(r'"(?:[^"\\\n]|\\.)*"|//[^\n]*|(?<![\w$.>])(%s)(?![\w$(])' % "|".join(
    sorted(map(re.escape, ea_by_name), key=len, reverse=True)))
for b in kept:
    for m in taken_in_code.finditer(b):
        if m.group(1) and ea_by_name[m.group(1)] != int(b[9:17], 16):
            address_taken.add(ea_by_name[m.group(1)])
dropped_positions, dropped_names = {}, {}
pointer_reached = 0
for n, b in enumerate(kept):
    raw = original_headers.get(int(b[9:17], 16))
    m = SIGNATURE.search(b)
    if not raw or not m or "__user" not in raw.group(1):
        continue
    raw_params = split_arguments(raw.group(3))
    open_paren = m.group(1).index("(")
    plain_params = split_arguments(m.group(1)[open_paren + 1:-1])
    registers = [(re.search(r"@<(\w+)>$", x) or [None, None])[1] for x in raw_params]
    by_pointer = int(b[9:17], 16) in address_taken
    positions = [i for i, r in enumerate(registers) if r in CALLEE_SAVED or (by_pointer and r is not None)]
    if len(raw_params) != len(plain_params) or not positions:
        continue
    pointer_reached += by_pointer
    real = [x for i, x in enumerate(plain_params) if i not in positions]
    gone = [plain_params[i] for i in positions]
    header = m.group(1)[:open_paren] + "(" + (", ".join(real) if real else "") + ")"
    locals_text = "".join("\n  %s; // a register (%s) the decompiler listed as an argument; never set" % (g, registers[i])
                          for g, i in zip(gone, positions))
    kept[n] = b[:m.start(1)] + header + "\n{" + locals_text + b[m.end():]
    dropped_positions[m.group(2)] = (positions, len(plain_params))
    dropped_names[int(b[9:17], 16)] = [re.search(r"([A-Za-z_]\w*)$", g).group(1) for g in gone]

def matching_paren(text, start):
    depth, at = 1, start
    while depth:
        depth += text[at] == "("
        depth -= text[at] == ")"
        at += 1
    return at
pointer_arguments_dropped, direct_arguments_dropped, direct_left = 0, 0, []
if dropped_positions:
    DIRECT = re.compile(r'"(?:[^"\\\n]|\\.)*"|//[^\n]*|(?<![\w$.>])(%s)\(' % "|".join(sorted(map(re.escape, dropped_positions), key=len, reverse=True)))
    def drop_direct(b):
        global direct_arguments_dropped
        header = SIGNATURE.search(b)
        out, at = [], 0
        for m in DIRECT.finditer(b):
            if not m.group(1) or m.start() < at or (header and header.start() <= m.start() < header.end()):
                continue
            end = matching_paren(b, m.end())
            arguments = split_arguments(b[m.end():end - 1])
            positions, total = dropped_positions[m.group(1)]
            if len(arguments) != total:
                direct_left.append(m.group(1))
                continue
            out.append(b[at:m.end()] + ", ".join(x for i, x in enumerate(arguments) if i not in positions) + ")")
            at = end
            direct_arguments_dropped += len(positions)
        return "".join(out) + b[at:]
    kept = [drop_direct(b) for b in kept]

    # calls through pointers that hand a dropped parameter on
    POINTER_CALL = re.compile(r"(?<![\w$.>])(v\d+)\(|\(\(((?:[\w\s\*]+?)\((?:__\w+ ?)?\*\)\()([^()]*)\)\)([^()\n]*?)\)\(")
    def drop_pointer(b):
        global pointer_arguments_dropped
        names = dropped_names.get(int(b[9:17], 16))
        if not names:
            return b
        edits = []
        for m in POINTER_CALL.finditer(b):
            end = matching_paren(b, m.end())
            arguments = split_arguments(b[m.end():end - 1])
            positions = [i for i, x in enumerate(arguments) if x in names]
            if not positions:
                continue
            keep = lambda items: ", ".join(x for i, x in enumerate(items) if i not in positions)
            if m.group(1):
                # a function-pointer variable: its declaration and the cast it was assigned through lose the places too
                variable = m.group(1)
                declaration = re.search(r"\((?:__\w+ ?)?\*%s\)\(([^()]*)\)" % variable, b)
                if not declaration or len(split_arguments(declaration.group(1))) != len(arguments):
                    continue
                old_types = declaration.group(1)
                new_types = keep(split_arguments(old_types))
                edits.append((declaration.start(1), declaration.end(1), new_types))
                for cast in re.finditer(r"\((?:__\w+ ?)?\*\)\((%s)\)\)" % re.escape(old_types), b):
                    edits.append((cast.start(1), cast.end(1), new_types))
                edits.append((m.end(), end - 1, keep(arguments)))
            else:
                types = split_arguments(m.group(3))
                if len(types) != len(arguments):
                    continue
                edits.append((m.start(3), m.end(3), keep(types)))
                edits.append((m.end(), end - 1, keep(arguments)))
            pointer_arguments_dropped += len(positions)
        for start, stop, text in sorted(set(edits), reverse=True):
            b = b[:start] + text + b[stop:]
        return b
    kept = [drop_pointer(b) for b in kept]

# rule: where the prototype list and a definition disagree (the list is not refreshed when a function is
# retyped), the definition wins
SIGNATURE = re.compile(r"^(?!//)([^\n;{}]*?\b([A-Za-z_$][\w$]*)\([^;{}]*\))\n\{", re.M)
definitions = {}
for b in kept:
    m = SIGNATURE.search(b)
    if m:
        definitions[m.group(2)] = " ".join(m.group(1).split()) + ";"
prototypes_replaced = 0
def definition_wins(m):
    global prototypes_replaced
    name = m.group(1)
    if name in definitions and definitions[name] != m.group(0):
        prototypes_replaced += 1
        return definitions.pop(name)
    return m.group(0)
all_definitions = dict(definitions)
decl_head = re.sub(r"^(?!//|extern )[^\n;{}()]*?\b([A-Za-z_$][\w$]*)\([^\n;{}]*\);", definition_wins, decl_head, flags=re.M)
declared = set(re.findall(r"^(?!//|extern )[^\n;{}()]*?\b([A-Za-z_$][\w$]*)\([^\n;{}]*\);", decl_head, flags=re.M))
decl_head += "\n// functions the export's prototype list does not have\n" + "".join(
    "%s\n" % proto for name, proto in all_definitions.items() if name not in declared)

# rule: a number that happens to lie inside the program's address range (0x800000 as a flag, a 33 MB size)
# is printed by IDA as the address of a global: "(unsigned int)&unk_800000". The exe's relocation table
# lists every real address in the code, so "&name" is turned back into the number when no relocated field
# anywhere holds that value. When the value is relocated somewhere, but not inside this function's main
# body, it is left as an address and counted (it may be code in a detached part of the function).
import bisect
reloc_addresses = [pe_image.base + slot for slot in pe_image.slots]
function_end = {f["ea"]: f["end"] for f in index}
all_starts = sorted(set(function_end) | {int(b[9:17], 16) for b in kept})
numbers_restored, numbers_uncertain = 0, 0
ADDRESS_OF = re.compile(r"&((?:unk|loc|dword|byte|word|flt|stru|off|qword|xmmword|dbl|asc)_([0-9A-F]{6,8}))\b(?!\[)")
def restore_numbers(b):
    global numbers_restored, numbers_uncertain
    ea = int(b[9:17], 16)
    end = function_end.get(ea) or all_starts[min(bisect.bisect_right(all_starts, ea), len(all_starts) - 1)]
    here = set(reloc_values[bisect.bisect_left(reloc_addresses, ea):bisect.bisect_left(reloc_addresses, end)])
    def one(m):
        global numbers_restored, numbers_uncertain
        value = int(m.group(2), 16)
        if value in here:
            return m.group(0)
        if value in relocated_anywhere:
            numbers_uncertain += 1
            return m.group(0)
        numbers_restored += 1
        return "0x%X" % value
    return ADDRESS_OF.sub(one, b)
kept = [restore_numbers(b) for b in kept]

# rule: a register value the decompiler could not trace gets a local declaration. After
# tools/ida/recover_register_args.py such a value sometimes carries the name of a callee's register
# argument (reg_ecx) without being declared anywhere in the function. It stays unset: these are the same
# lost values that tools/draft_uninit.sh lists.
lost_registers = 0
def declare_lost_registers(b):
    global lost_registers
    m = SIGNATURE_EARLY.search(b)
    if not m:
        return b
    lost = sorted(set(re.findall(r"(?<![\w$.>])(reg_e[a-ds][xi])(?![\w$])", b[m.end():])) - set(re.findall(r"\b(reg_e[a-ds][xi])\b", m.group(0))))
    # (a global can carry such a name too: IDA renamed the unit list at 0x38C7250 to reg_ecx after it was
    # passed as that argument; it is placed like any other global)
    lost = [n for n in lost if n not in names and not re.search(r"^  [^;\n]*\b%s; //" % n, b, re.M)]
    if not lost:
        return b
    lost_registers += len(lost)
    return b[:m.end()] + "".join("\n  struct unit *%s; // never set: a register value the decompiler lost" % n for n in lost) + b[m.end():]
SIGNATURE_EARLY = re.compile(r"^(?!//)[^\n;{}]*?\b[A-Za-z_$][\w$]*\([^;{}]*\)\n\{", re.M)
kept = [declare_lost_registers(b) for b in kept]

# rule: a hexadecimal constant of 0x80000000 or more without a "u" suffix is a negative int. IDA writes
# unsigned constants with the suffix ("0x780u") and signed ones without, but C gives an unsuffixed hex
# literal that does not fit an int the type unsigned int. "v4 / 0xFFFFFFFC" is a division by -4; as unsigned
# it is 0 (found in change_dirt_layer, where it made each zone's dirt run over the next zone).
SIGNED_HEX = re.compile(r'"(?:[^"\\\n]|\\.)*"|//[^\n]*|MEMORY\[[^\]\n]*\]|(?<![\w.])(0x[89A-F][0-9A-F]{7})(?![0-9A-Fa-fuUlL])')
signed_constants = 0
def signed_hex(m):
    global signed_constants
    if not m.group(1):
        return m.group(0)
    signed_constants += 1
    return "(int)" + m.group(1)
kept = [SIGNED_HEX.sub(signed_hex, b) for b in kept]

# rule: a one-character wide string looks like a narrow one in memory ("_" then a zero byte), and IDA prints
# it that way, cast to a wide pointer: (__int16 *)"_". It is the wide literal L"_"; as a narrow literal it is
# only two bytes long and the reader runs past its end.
wide_literals = 0
def wide_literal(b):
    global wide_literals
    b, n = re.subn(r'(\((?:const )?(?:wchar_t|__int16|_WORD|unsigned __int16) \*\))("(?:[^"\\\n]|\\.)*")', r"\1L\2", b)
    wide_literals += n
    return b
kept = [wide_literal(b) for b in kept]

# rule: "--*(_DWORD *)p < 0" is a countdown reaching below zero. _DWORD is unsigned, so as written the test
# is never true: missiles never expired (their frames-left counter ran to minus thousands). The decrement's
# result is compared as a signed number, which is what the original instruction (dec, then jump if sign) does.
signed_countdowns = 0
def signed_countdown(b):
    global signed_countdowns
    b, n = re.subn(r"(?<![\w)])(--\*\(_DWORD \*\)(?:(?!&&)(?:->|[^;<>=|]))+?) < 0", r"(int)\1 < 0", b)
    signed_countdowns += n
    return b
kept = [signed_countdown(b) for b in kept]

# rule: "*(_DWORD *)p <= 0" and "... > 0" are signed tests. An unsigned "<= 0" would have been printed as
# "== 0", so where the decompiler prints these on an (unsigned) _DWORD read, the original compares signed.
# As unsigned, -1 counts as greater than zero: the mouse wheel scrolled the same way in both directions.
ZERO_TEST = re.compile(r" (<=|>) 0(?![.\dxuUlLA-Fa-f])")
signed_zero_tests = 0
def signed_zero_test(b):
    global signed_zero_tests
    out, at = [], 0
    for m in ZERO_TEST.finditer(b):
        depth, i = 0, m.start() - 1
        while i >= at:
            c = b[i]
            if c in ")]":
                depth += 1
            elif c in "([":
                if depth == 0:
                    break
                depth -= 1
            elif depth == 0 and c in " ,=&|!?:;\n":
                break
            i -= 1
        operand = b[i + 1:m.start()]
        if re.match(r"^\*\(\(?_DWORD \*\)", operand):
            out.append(b[at:i + 1] + "(int)" + operand)
            at = m.start()
            signed_zero_tests += 1
    return "".join(out) + b[at:]
kept = [signed_zero_test(b) for b in kept]

# rule: rand() takes no arguments. Where the compiler reserved a stack slot with "push ecx" just before
# calling it, the decompiler shows a call through a cast with an argument, in one place as __stdcall:
# "((int (__stdcall *)(int))rand)(v9)". Recompiled, that expects rand to remove the argument, which it does
# not, and the stack is off by four bytes from then on (the crash in monster_do_rat).
rand_casts = 0
def plain_rand(b):
    global rand_casts
    b, n = re.subn(r"\(\([\w\s\*]+\((?:__\w+ )?\*\)\([^()]*\)\)rand\)\([^()]*\)", "rand()", b)
    rand_casts += n
    return b
kept = [plain_rand(b) for b in kept]

# rule: MEMORY[address] is IDA's notation for an access through a constant (usually null-based) pointer
kept = [re.sub(r"\bMEMORY\[(0x[0-9A-Fa-f]+|\d+)\]", r"(*(unsigned int *)\1)", b) for b in kept]
# rule: a vector result stored into a 16-byte integer is a bit copy
kept = [re.sub(r"\(_OWORD\)(_mm_\w+\(.*\));$", r"ILB_M128_BITS(\1);", b, flags=re.M) for b in kept]

# rule: a global with an ordinary name (Buffer, Text, config...) is renamed NAME_ADDRESS everywhere except in
# functions that have a local or parameter of that name
plain_globals = {k: v for k, v in global_names.items() if k != v}
plain_re = re.compile(r"(?<![\w$.>])(%s)(?![\w$])" % "|".join(sorted(map(re.escape, plain_globals), key=len, reverse=True))) if plain_globals else None
LOCAL_RE = re.compile(r"^\s+[^;()=]*?[\s\*]([A-Za-z_]\w*)(?:\[[^\]]*\])*; //", re.M)


def rename_globals(body):
    if not plain_re:
        return body
    header = body[:body.index("{")] if "{" in body else body
    shadowed = set(LOCAL_RE.findall(body)) | set(re.findall(r"[\s\*](\w+)\s*[,)]", header))
    return plain_re.sub(lambda m: m.group(1) if m.group(1) in shadowed else plain_globals[m.group(1)], body)


# rule: a type whose name is also used for variables (unit, box, ...) is written with its tag in pointer
# casts and declarations, which is unambiguous
struct_tags = {name for kind, name in tags if kind != "enum"}
variable_names = set()
for b in kept:
    variable_names.update(LOCAL_RE.findall(b))
ambiguous = sorted(struct_tags & variable_names)
ambiguous_re = re.compile(r"(?<![\w$.>])(?<!struct )(%s) \*(?=[\w\)\*,])" % "|".join(map(re.escape, ambiguous))) if ambiguous else None

fixed = []
for b in kept:
    b = rename_globals(b)
    if ambiguous_re:
        b = ambiguous_re.sub(r"struct \1 *", b)
    fixed.append(keep_stack_layout(b))      # last: it removes the declarations the two rules above look at
kept = fixed
if ambiguous_re:
    decl_head = ambiguous_re.sub(r"struct \1 *", decl_head)

# --------------------------------------------------------------------------------------------- output
files = json.load(open(os.path.join(ROOT, "db", "files.json")))
with open(os.path.join(OUT, "ILB_draft_decls.h"), "w") as f:
    f.write("// Generated by tools/draft_fix.py: prototypes of every function and the address of every global.\n#pragma once\n")
    f.write(decl_head)
    f.write("\n// imported functions\n" + "".join(externs))
    f.write("\n// globals, at their addresses in the original image\nextern char ILB_image[];\n#define ILB_ADDR( ea ) ( ILB_image + ( ( ea ) - 0x400000 ) )\n")
    f.write("".join(data_lines))

src_dir = os.path.join(OUT, "src")
os.makedirs(src_dir, exist_ok=True)
for old in os.listdir(src_dir):
    os.remove(os.path.join(src_dir, old))
# rule: a function that src/ already has hand-written (same test as tools/progress.py: a confidently named
# function whose name is defined in src/<its file>) is kept in the draft for reference but compiled only
# when ILB_HANDWRITTEN is not defined, so the draft is complete on its own and can also be built around src/
db_names = json.load(open(os.path.join(ROOT, "db", "names.json")))
src_text = {}
def handwritten_params(ea, file_name):
    """Number of parameters of the hand-written version, or None if there is none."""
    entry = db_names.get("%08X" % ea, {})
    path = os.path.join(ROOT, "src", file_name)
    if not entry.get("name") or entry.get("confidence") not in ("high", "medium", "manual") or not os.path.exists(path):
        return None
    if file_name not in src_text:
        src_text[file_name] = re.sub(r"//[^\n]*", "", open(path).read())
    m = re.search(r"^[A-Za-z_][\w \*&:<>]*?\b%s\s*\(([^;{}]*?)\)\s*\{" % re.escape(entry["name"]), src_text[file_name], re.M)
    if not m:
        return None
    params = m.group(1).strip()
    return 0 if params in ("", "void") else params.count(",") + 1

# rule: functions rewritten by hand for the port. The decompiled body is kept for reference but not compiled.
REPLACED_BY_PORT = {
    0x40D960: "port/gge_display_modes.c",       # gge_create_display_mode_list: asked DXGI, now asks SDL
}
# rule: optional hand-written modules in port/ (changes to the game that are NOT in the original) get one
# call at the start of a decompiled function, compiled only when the module's option is defined. With the
# option off the function is exactly the decompiled one.
ENTRY_HOOKS = {      # function: (option, declaration, statement); the function's own parameter names may be used
    0x526230: ("ILB_MOD_START_FLOW", "void ILB_mod_start_flow( void );", "ILB_mod_start_flow();"),     # quests_init (new character)
    0x5265B0: ("ILB_MOD_START_FLOW", "void ILB_mod_start_flow( void );", "ILB_mod_start_flow();"),     # quests_load (loaded character)
    0x5266C0: ("ILB_MOD_START_FLOW", "void ILB_mod_start_flow_quest_new( unsigned int );",              # quest_set_new
               "ILB_mod_start_flow_quest_new( a1 );"),
    0x4CA510: ("ILB_MOD_PER_STEP", "void ILB_mod_season_outfits( void ); void ILB_mod_coop_spawn( void ); void ILB_mod_season_buildings( void );",   # process_npcs: once per step of play
               "ILB_mod_season_outfits(); ILB_mod_coop_spawn(); ILB_mod_season_buildings();"),
    0x4937E0: ("ILB_MOD_PER_STEP", "void ILB_mod_coop_spawn_loading( void ); void ILB_mod_season_buildings_loading( void );",   # the loading screen's step
               "ILB_mod_coop_spawn_loading(); ILB_mod_season_buildings_loading();"),
    0x578FD0: ("ILB_CHEATS", "int ILB_cheat_god( unsigned int );", "if ( ILB_cheat_god( ecx0 ) ) return 0;"),       # unit_take_damage: god mode
    0x57BA20: ("ILB_CHEATS", "int ILB_cheat_god( unsigned int );", "if ( ILB_cheat_god( a1 ) ) return 0;"),         # unit_take_undefended_damage
    0x4E9C00: ("ILB_CHEATS", "int ILB_cheat_god_die( struct unit * );", "if ( ILB_cheat_god_die( a1 ) ) return 0;"),   # do_player_die
    0x43DA90: ("ILB_MOD_SEASON_BUILDINGS", "int ILB_mod_season_buildings_no_respawn( void );",         # dungeon_event_respawn_buildings
               "if ( ILB_mod_season_buildings_no_respawn() ) return;"),                                 # Christmas decoration is for Winter
    0x406D30: ("ILB_NET_INPUT", "int ILB_input_tick_allowed( void );",                                  # "is a simulation step due?"
               "if ( !ILB_input_tick_allowed() ) { byte_16B4D99 = 0; return 0; }"),                    # two copies over a network wait for each other
    0x00435840: ("ILB_FIX_STUCK_PLAYER", "void ILB_fix_stuck_player( struct unit * );",                       # dungeon_place_unit
               "ILB_fix_stuck_player( this );"),
    0x40BBE0: ("ILB_NET_INPUT", "void ILB_input_views( unsigned int );", "ILB_input_views( a1 );"),     # gge_texture_memory_rendering_surface_add
    0x004938C0: ("ILB_MOD_NET_MENU", "int ILB_mod_net_menu_play( int );",                                      # mm_do_play (the Play button)
               "if ( ILB_mod_net_menu_play( a1 ) ) return 0;"),
    0x0042ABF0: ("ILB_MOD_NET_MENU", "int ILB_mod_net_menu_character( unsigned int );",                       # cs_do_select
               "if ( ILB_mod_net_menu_character( a2 ) ) return 0;"),
    0x465B00: ("ILB_NET_INPUT", "int *ILB_input_select_client( void );",                               # keyboard/mouse event: which player
               "if ( ILB_input_select_client() ) return ILB_input_select_client();"),
    0x465B40: ("ILB_NET_INPUT", "int *ILB_input_select_client( void );",                               # controller event: which player
               "if ( ILB_input_select_client() ) return ILB_input_select_client();"),
    0x493AB0: ("ILB_NET_INPUT", "void ILB_input_snapshot_start_state( void );",                        # the title menu's set-up: first time,
               "ILB_input_snapshot_start_state();"),                                                    # remember the game's start-up state
    0x462750: ("ILB_NET_INPUT", "int ILB_input_net_quit( int );",                                       # game_return_to_mainmenu
               "if ( ILB_input_net_quit( 1 ) ) return 0;"),                                             # in a network game only the
    0x4626E0: ("ILB_NET_INPUT", "int ILB_input_net_quit( int );",                                       # game_force_shutdown
               "if ( ILB_input_net_quit( 2 ) ) return;"),                                               # player who chose it leaves
    0x5275F0: ("ILB_MOD_START_FLOW", "int ILB_mod_start_flow_reward( unsigned int );",                  # quest_get_reward_item_class
               "if ( ILB_mod_start_flow_reward( a1 ) != -2 ) return ILB_mod_start_flow_reward( a1 );"),
}
# rule: the same for one expression inside a function: with the module's option defined the expression is
# replaced, without it the decompiled text is compiled unchanged.
VALUE_HOOKS = [      # function, option, declaration, decompiled text, replacement
    (0x407E50, "ILB_NET_INPUT", "void *ILB_game_malloc( unsigned int );",                # drb_malloc: the game's one way to get memory
     "v8 = malloc(a1);", "v8 = ILB_game_malloc(a1);"),
    (0x407F90, "ILB_NET_INPUT", "void ILB_game_free( void * );",                        # drb_free: ... and to give it back
     "free((void *)dword_170550C[1030 * v1]);", "ILB_game_free((void *)dword_170550C[1030 * v1]);"),
    # sub_4610D0 sets the scale of the game's picture from the size of a view. With two views it zooms out
    # (two players share one screen); two copies of a network game each show one full-size view, and there
    # the picture should be exactly as large as in a single game (draft/native/native_input.c)
    (0x4610D0, "ILB_NET_INPUT", "int ILB_input_full_views( void );",
     "  if ( (unsigned int)dword_6113F4 <= 1 )\n", "  if ( (unsigned int)dword_6113F4 <= 1 || ILB_input_full_views() )\n"),
    (0x4610D0, "ILB_NET_INPUT", "int ILB_input_full_views( void );",
     "  if ( (unsigned int)dword_6113F4 > 1 )\n", "  if ( (unsigned int)dword_6113F4 > 1 && !ILB_input_full_views() )\n"),
    # dungeon_set_view_focus: with two views the camera is pulled back to a fixed distance (31.125) so that
    # each half of a shared screen shows enough; with one full-size view per copy the player's own camera
    # distance setting applies, as in a single game
    (0x004366F0, "ILB_NET_INPUT", "int ILB_input_full_views( void );",
     "  v15 = 31.125;\n  ILB_stack.m_v40.v = 31.125;\n",
     "  v15 = ILB_input_full_views() ? *(float *)&dword_60A084 : 31.125;\n  ILB_stack.m_v40.v = v15;\n"),
    (0x4A36B0, "ILB_MOD_SEASON_EVENTS", "int ILB_mod_season_currency( int );",          # monster_drop_seasonal_currency
     "item_new(a1->world_id, 1022,", "item_new(a1->world_id, ILB_mod_season_currency( 1022 ),"),
    (0x43BEF0, "ILB_MOD_SEASON_BUILDINGS", "int ILB_mod_season_building_tile( int );",   # dungeon_spawn_blueprint: a template tile
     "switch ( *(_DWORD *)(a5 + 4 * v7) )", "switch ( ILB_mod_season_building_tile( *(_DWORD *)(a5 + 4 * v7) ) )"),
    (0x4CD2F0, "ILB_MOD_SEASON_BUILDINGS", "void ILB_mod_season_buildings_art( void );",  # objects_update_seasons_gfx, Autumn
     "result = gge_texture_update_combined_gs3files(aUnitsObjectsAu_5E07CC);",
     "result = gge_texture_update_combined_gs3files(aUnitsObjectsAu_5E07CC); ILB_mod_season_buildings_art();"),
]
groups, last_file = {}, "_before_first_file"
manifest = {}
entry_hooks_placed = 0
value_hooks_placed = 0
for b in kept:
    ea = int(b[9:17], 16)
    if ea in ENTRY_HOOKS:
        option, declaration, statement = ENTRY_HOOKS[ea]
        b, placed = re.subn(r"\)\n\{\n", lambda m: ")\n{\n#ifdef %s\n  %s\n  %s\n#endif\n" % (option, declaration, statement), b, count=1)
        entry_hooks_placed += placed
    for hook_ea, option, declaration, old_text, new_text in VALUE_HOOKS:
        if ea == hook_ea and old_text in b:
            b = b.replace(old_text, "\n#ifdef %s\n%s\n#else\n%s\n#endif\n" % (option, new_text, old_text))
            b = re.sub(r"\)\n\{\n", lambda m: ")\n{\n#ifdef %s\n  %s\n#endif\n" % (option, declaration), b, count=1)
            value_hooks_placed += 1
    loc = files.get("%08X" % ea, {})
    if loc.get("file"):
        last_file = loc["file"]                 # a function on a boundary stays with the earlier neighbour
    m = SIGNATURE.search(b)
    draft_params = None
    if m:
        params = m.group(1)[m.group(1).index("(") + 1:-1].strip()
        draft_params = 0 if params in ("", "void") else params.count(",") + 1
    written = handwritten_params(ea, last_file)
    if ea in REPLACED_BY_PORT:
        b = "#if 0       // rewritten by hand: %s\n%s#endif\n\n" % (REPLACED_BY_PORT[ea], b.rstrip("\n") + "\n")
    elif written is not None:
        b = "#ifndef ILB_HANDWRITTEN     // src/%s has this function\n%s#endif\n\n" % (last_file, b.rstrip("\n") + "\n")
    groups.setdefault(last_file, []).append(b)
    manifest["%08X" % ea] = {"file": last_file, "name": re.sub(r"__regs$", "", m.group(2)) if m else None, "handwritten": written is not None,
                             "draft_params": draft_params, "handwritten_params": written}
for name, parts in groups.items():
    with open(os.path.join(src_dir, os.path.splitext(name)[0] + ".c"), "w") as f:
        f.write("// %s - mechanical draft generated by tools/draft_fix.py from IDA's export. Do not edit.\n" % name)
        f.write('#include "ILB_draft_decls.h"\n\n')
        f.write("".join(parts))
json.dump(manifest, open(os.path.join(OUT, "manifest.json"), "w"), indent=0)

print("hooks for port modules placed: entry %d of %d, value %d of %d" % (entry_hooks_placed, len(ENTRY_HOOKS), value_hooks_placed, len(VALUE_HOOKS)))
print("data: %d globals placed (%d renamed NAME_ADDRESS), %d imports, %d declarations not placed" % (
    len(data_lines), len(plain_globals), len(externs), len(unplaced)))
for u in unplaced[:12]:
    print("   not placed:", u)
print("globals used but not declared by the export: %d" % len(undeclared))
print("single-site fixes applied: %d of %d; prototypes replaced by their definition's: %d" % (site_fixes_applied, len(SITE_FIXES), prototypes_replaced))
print("numbers IDA printed as addresses, restored: %d; left as addresses but not certain: %d" % (numbers_restored, numbers_uncertain))
print("interface calls given the right number of arguments: %d; still wrong: %s; given the __stdcall convention: %d" % (
    com_fixed, ", ".join(com_left) or "none", com_conventions))
print("lost register values declared as unset locals: %d" % lost_registers)
print("functions with register \"arguments\" removed: %d, of which reached through pointers: %d (" % (len(dropped_positions), pointer_reached), end="")
print("%d arguments dropped at direct calls, %d at pointer calls; calls left alone: %d)" % (
    direct_arguments_dropped, pointer_arguments_dropped, len(direct_left)))
print("named \"globals\" that are plain numbers (never relocated): %d" % plain_number_globals)
print("large hexadecimal constants made signed, as IDA means them: %d" % signed_constants)
print("Direct3D calls turned into OpenGL backend calls: %d; methods the backend lacks: %s" % (backend_calls, ", ".join(sorted(backend_missing)) or "none"))
print("calls of rand() through a cast made plain: %d" % rand_casts)
print("narrow literals that are really wide strings: %d" % wide_literals)
print("unsigned countdown tests made signed: %d" % signed_countdowns)
print("unsigned reads tested against zero made signed: %d" % signed_zero_tests)
print("types used with their tag because a variable shares the name: %s" % ", ".join(ambiguous))
print("functions whose stack variables are kept at their original offsets: %d" % stack_functions)
print("source files: %d, functions that src/ also has hand-written: %d" % (
    len(groups), sum(1 for v in manifest.values() if v["handwritten"])))
print("types: %d tags, %d forward typedefs, %d enum constants renamed" % (len(tags), len(forward), renamed_constants))
print("functions: %d kept, %d library/runtime dropped" % (len(kept), dropped))
