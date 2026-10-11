#!/usr/bin/env python3
"""Wave 3: recover the per-unit-type custom data structs (unit.custom_data) from how the code reads them.

Every constant-offset access to custom_data in the typed pseudo-C is attributed to a unit type: by a
unit_type comparison on the same variable in that function, else by the function's source file. Field names
come from assert expressions (db/fields.json); everything else is field_<offset>. Writes db/custom_data.h and
db/custom_data.json. The item struct is hand-written in db/types.h; it is generated here only to measure how
well this method reproduces a known layout.
"""
import collections
import json
import os
import re

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FUNCS = os.path.join(ROOT, "dump", "functions")
files = json.load(open(os.path.join(ROOT, "db", "files.json")))
fields = json.load(open(os.path.join(ROOT, "db", "fields.json")))

KINDS = {0: "player", 1: "monster", 2: "missile", 3: "item", 4: "object", 5: "npc", 6: "ambient"}
KNOWN_SIZE = {"item": 364, "object": 332, "npc": 200, "monster": 2112}  # returned/copied by the *_save functions
BASE_KIND = {"icd": "item", "pcd": "player", "ocd": "object", "ncd": "npc", "acd": "ambient"}


def file_kind(name):
    if not name:
        return None
    if name.startswith(("g_player", "g_char_", "g_ui", "g_account", "g_client")):
        return "player"
    if name.startswith(("g_monsters", "g_boss")):
        return "monster"
    for prefix, kind in (("g_items", "item"), ("g_unique_items", "item"), ("g_objects", "object"),
                         ("g_npcs", "npc"), ("g_missiles", "missile"), ("g_ambient", "ambient")):
        if name.startswith(prefix):
            return kind
    return None


SIZE = {"_DWORD": 4, "int": 4, "unsigned int": 4, "float": 4, "_BYTE": 1, "char": 1, "bool": 1, "byte": 1,
        "unsigned __int8": 1, "_WORD": 2, "__int16": 2, "unsigned __int16": 2, "_QWORD": 8, "double": 8}
CTYPE = {"_DWORD": "dword", "int": "int", "unsigned int": "dword", "float": "float", "_BYTE": "byte",
         "char": "byte", "bool": "bool", "byte": "byte", "unsigned __int8": "byte", "_WORD": "word",
         "__int16": "short", "unsigned __int16": "word", "_QWORD": "qword", "double": "double"}
T = r"(?P<type>[A-Za-z_][\w ]*?)"
PATTERNS = [
    (re.compile(r"\*\(" + T + r" \*\)&(?P<var>\w+)->custom_data\[(?P<n>\d+)\](?!\s*[\[*+])"), "abs"),
    (re.compile(r"(?<![&\w])(?P<var>\w+)->custom_data\[(?P<n>\d+)\](?!\s*[\[*+])"), "byte"),
    (re.compile(r"\*\(\(" + T + r" \*\)(?P<var>custom_data\w*) \+ (?P<n>\d+)\)"), "scaled"),
    (re.compile(r"\*\(" + T + r" \*\)&(?P<var>custom_data\w*)\[(?P<n>\d+)\](?!\s*[\[*+])"), "abs"),
    (re.compile(r"\*\(" + T + r" \*\)(?P<var>custom_data\w*)\b(?!\s*[\[+\w])"), "zero"),
    (re.compile(r"(?<![&\w>])(?P<var>custom_data\w*)\[(?P<n>\d+)\](?!\s*[\[*+])"), "byte"),
]
ARRAY_RE = re.compile(r"custom_data\[(\d+) \* [^\]]*? \+ (\d+)\]")
KIND_RE = re.compile(r"\b(\w+)->unit_type (?:!=|==) (\d)\b")
ALIAS_RE = re.compile(r"\b(custom_data\w*) = (?:\([\w \*]+\))?(\w+)->custom_data;")

votes = collections.defaultdict(lambda: collections.defaultdict(collections.Counter))  # kind -> off -> type
arrays = collections.defaultdict(collections.Counter)
unattributed = 0
for fname in sorted(os.listdir(FUNCS)):
    if not fname.endswith(".c"):
        continue
    body = open(os.path.join(FUNCS, fname)).read()
    if "custom_data" not in body:
        continue
    fallback = file_kind(files.get(fname[:8], {}).get("file"))
    var_kinds = collections.defaultdict(set)
    for var, k in KIND_RE.findall(body):
        if int(k) in KINDS:
            var_kinds[var].add(KINDS[int(k)])
    alias = dict(ALIAS_RE.findall(body))

    def kind_of(var):
        var = alias.get(var, var)
        ks = var_kinds.get(var)
        if ks and len(ks) == 1:
            return next(iter(ks))
        return fallback

    seen_spans = []
    for rx, mode in PATTERNS:
        for m in rx.finditer(body):
            if any(a <= m.start() < b for a, b in seen_spans):
                continue
            seen_spans.append(m.span())
            t = m.groupdict().get("type") or "byte"
            t = t.strip()
            if t not in SIZE:
                continue
            n = int(m.groupdict().get("n") or 0)
            offset = n * SIZE[t] if mode == "scaled" else (0 if mode == "zero" else n)
            kind = kind_of(m.group("var"))
            if not kind:
                unattributed += 1
                continue
            votes[kind][offset][CTYPE[t]] += 1
    if fallback:
        for stride, base in ARRAY_RE.findall(body):
            arrays[fallback][(int(base), int(stride))] += 1

named = collections.defaultdict(dict)
for e in fields:
    kind = BASE_KIND.get(e["base"])
    if e["base"] == "mcd":  # used for both monster and missile custom data
        kind = file_kind(files.get(e["example"][:8], {}).get("file"))
    if kind:
        named[kind][e["offset"]] = (e["field"], e["type"], e["votes"])

result = {}
out = ["// Generated by tools/custom_data.py. Do not edit by hand; promote a struct to db/types.h to refine it.",
       "// Layouts are inferred from accesses in the decompiled code. field_<offset> names are placeholders,",
       "// named fields come from assert expressions. unknown_* ranges were never accessed at a constant",
       "// offset: they are arrays, nested structs, padding or unused. Types are the most common access width.",
       "", "#pragma once", ""]
for kind in ("player", "monster", "missile", "object", "npc", "ambient", "item"):
    offs = votes.get(kind, {})
    if not offs:
        continue
    size = KNOWN_SIZE.get(kind)
    members = []
    pos = 0
    for off in sorted(offs):
        ctype, n = offs[off].most_common(1)[0]
        width = {"dword": 4, "int": 4, "float": 4, "byte": 1, "bool": 1, "word": 2, "short": 2,
                 "qword": 8, "double": 8}[ctype]
        total = sum(offs[off].values())
        if off < pos:  # overlaps the previous member (mixed-width access); keep the earlier one
            continue
        if size and off + width > size:
            continue
        name, src = "field_%d" % off, ""
        if off in named[kind]:
            name, ntype, nv = named[kind][off]
            if ctype in ("dword", "int", "byte") and ntype != ctype and {ntype, ctype} <= {"dword", "int"}:
                ctype = ntype
            if ctype == "byte" and ntype == "byte" and name.startswith(("is_", "in_", "has_", "can_")):
                ctype = "bool"
            src = "  name from assert"
        if ctype == "qword":
            ctype, width = "dword", 4
        members.append((off, ctype, name, total, src))
        pos = off + width
    end = size or pos
    result[kind] = {"size": size, "size_confirmed": bool(size), "last_access_end": pos,
                    "members": [{"offset": o, "type": t, "name": n, "accesses": c} for o, t, n, c, s in members],
                    "arrays": [{"base": b, "stride": s, "accesses": c}
                               for (b, s), c in arrays.get(kind, collections.Counter()).most_common(12)]}
    note = "sizeof %d (from the save function)" % size if size else "size unknown; last constant access ends at %d" % pos
    if kind == "item":
        continue  # hand-written in db/types.h; kept in the json as a self-check of this method
    out.append("struct %s_custom_data   // %s, %d members seen" % (kind, note, len(members)))
    out.append("{")
    pos = 0
    for off, ctype, name, total, src in members:
        if off > pos:
            out.append("    byte        unknown_%d[%d];" % (pos, off - pos))
        out.append("    %-40s // %4d  x%d%s" % ("%-11s %s;" % (ctype, name), off, total, src))
        pos = off + {"dword": 4, "int": 4, "float": 4, "byte": 1, "bool": 1, "word": 2, "short": 2}[ctype]
    if end > pos:
        out.append("    byte        unknown_%d[%d];" % (pos, end - pos))
    out.append("};")
    if result[kind]["arrays"]:
        out.append("// arrays seen in this type's own source files (base offset, element stride, uses): " + ", ".join(
            "%d/%d/x%d" % (a["base"], a["stride"], a["accesses"]) for a in result[kind]["arrays"]))
    out.append("")
open(os.path.join(ROOT, "db", "custom_data.h"), "w").write("\n".join(out))
json.dump(result, open(os.path.join(ROOT, "db", "custom_data.json"), "w"), indent=1)

print("accesses without a unit type: %d" % unattributed)
for kind, r in result.items():
    namedn = sum(1 for m in r["members"] if not m["name"].startswith("field_"))
    print("  %-8s members %4d (named %2d)  size %s  last access end %d" % (
        kind, len(r["members"]), namedn, r["size"] or "?", r["last_access_end"]))
