#!/usr/bin/env python3
"""Assemble the per-function pseudo-C into one file per original source file, in source order, with the
recovered knowledge written in. Output: dump/by_file/<source file>.c - the working input for Wave 4.

Rewrites applied to the decompiler text:
  *(_DWORD *)&v->custom_data[112]   ->  ICD(v)->total_slevel       (ICD/PCD/MCD/OCD/NCD/XCD/ACD = custom data
                                                                    cast for item/player/monster/object/npc/
                                                                    missile/ambient; type decided per variable)
  v->unit_type != 3                 ->  v->unit_type != unit_type_item
  item->unit_class == 356           ->  item->unit_class == item_type_adamantine_pickaxe
  ...attribute_flags & 0x10000      ->  trailing comment naming the bits (2019 names, see db/defines.h)
"""
import collections
import json
import os
import re

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FUNCS = os.path.join(ROOT, "dump", "functions")
OUT = os.path.join(ROOT, "dump", "by_file")
index = json.load(open(os.path.join(ROOT, "dump", "index.json")))
names = json.load(open(os.path.join(ROOT, "db", "names.json")))
files = json.load(open(os.path.join(ROOT, "db", "files.json")))
custom = json.load(open(os.path.join(ROOT, "db", "custom_data.json")))

KINDS = {0: "player", 1: "monster", 2: "missile", 3: "item", 4: "object", 5: "npc", 6: "ambient"}
UNIT_TYPE = {0: "unit_type_player", 1: "unit_type_monster", 2: "unit_type_missile", 3: "unit_type_item",
             4: "unit_type_object", 5: "unit_type_npc", 6: "unit_type_ambient"}
MACRO = {"item": "ICD", "player": "PCD", "monster": "MCD", "object": "OCD", "npc": "NCD", "missile": "XCD",
         "ambient": "ACD"}
SIZE = {"_DWORD": 4, "int": 4, "unsigned int": 4, "float": 4, "_BYTE": 1, "char": 1, "bool": 1, "byte": 1,
        "_WORD": 2, "__int16": 2}

# member name by (kind, offset); the item struct comes from the hand-written header
members = {k: {m["offset"]: m["name"] for m in v["members"]} for k, v in custom.items()}
types_h = open(os.path.join(ROOT, "db", "types.h")).read()
item_body = types_h[types_h.index("struct item_custom_data"):]
members["item"] = {int(m.group(3)): m.group(1) for m in re.finditer(
    r"^\s+\w+\s+(\w+)(\[[^\]]+\])?;\s*//\s*(\d+)", item_body, re.M)}

class_enums = {}
enums_h = open(os.path.join(ROOT, "db", "enums.h")).read()
for kind, prefix in (("item", "item_type"), ("object", "object_type"), ("monster", "monster_type"),
                     ("missile", "missile_type")):
    block = re.search(r"enum %s_id.*?\{(.*?)\};" % prefix, enums_h, re.S)
    if block:
        class_enums[kind] = {int(v): n for n, v in re.findall(r"(\w+) = (\d+),", block.group(1))}

FLAG_FIELDS = {"attribute_flags": ["attribute_flag_"], "attribute2_flags": ["attribute2_flag_"],
               "trigger_flags": ["trigger_flag_"], "trigger2_flags": ["trigger2_flag_"],
               "status_flags": ["status_flag_"], "weapon_flags": ["wand_flag_", "melee_flag_"],
               "flags": ["unit_flag_"], "wizard_flags": ["wizard_flags_"], "warrior_flags": ["warrior_flags_"]}
flag_names = collections.defaultdict(dict)
for name, value in re.findall(r"#define (\w+)\s+(0x[0-9a-fA-F]+)\s*$", open(os.path.join(ROOT, "db", "defines.h")).read(), re.M):
    v = int(value, 16)
    if v and v & (v - 1) == 0:
        for field, prefixes in FLAG_FIELDS.items():
            if name.startswith(tuple(prefixes)):
                flag_names[field][v] = name


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


KIND_RE = re.compile(r"\b(\w+)->unit_type (?:!=|==) (\d)\b")
ALIAS_RE = re.compile(r"\b(custom_data\w*) = (?:\([\w \*]+\))?(\w+)->custom_data;")
T = r"(?P<type>[A-Za-z_][\w ]*?)"
stats = collections.Counter()


def annotate(body, fallback):
    var_kinds = collections.defaultdict(set)
    for var, k in KIND_RE.findall(body):
        if int(k) in KINDS:
            var_kinds[var].add(KINDS[int(k)])
    alias = dict(ALIAS_RE.findall(body))

    def kind_of(var):
        owner = alias.get(var, var)
        ks = var_kinds.get(owner)
        return next(iter(ks)) if ks and len(ks) == 1 else fallback

    def member(var, offset, original, owner_expr):
        kind = kind_of(var)
        name = members.get(kind, {}).get(offset) if kind else None
        if not name:
            stats["custom_data left raw"] += 1
            return original
        stats["custom_data named"] += 1
        return "%s(%s)->%s" % (MACRO[kind], owner_expr, name)

    def sub_field(m):
        t = (m.groupdict().get("type") or "byte").strip()
        if t not in SIZE:
            return m.group(0)
        return member(m.group("var"), int(m.group("n")), m.group(0), m.group("var"))

    def sub_local(m, scaled):
        t = (m.groupdict().get("type") or "byte").strip()
        if t not in SIZE:
            return m.group(0)
        var = m.group("var")
        offset = int(m.group("n")) * (SIZE[t] if scaled else 1)
        return member(var, offset, m.group(0), alias.get(var, var))

    body = re.sub(r"\*\(" + T + r" \*\)&(?P<var>\w+)->custom_data\[(?P<n>\d+)\](?!\s*[\[*+])", sub_field, body)
    body = re.sub(r"(?<![&\w])(?P<var>\w+)->custom_data\[(?P<n>\d+)\](?!\s*[\[*+])", sub_field, body)
    body = re.sub(r"\*\(\(" + T + r" \*\)(?P<var>custom_data\w*) \+ (?P<n>\d+)\)", lambda m: sub_local(m, True), body)
    body = re.sub(r"\*\(" + T + r" \*\)&(?P<var>custom_data\w*)\[(?P<n>\d+)\](?!\s*[\[*+])", lambda m: sub_local(m, False), body)
    body = re.sub(r"(?<![&\w>])(?P<var>custom_data\w*)\[(?P<n>\d+)\](?!\s*[\[*+])", lambda m: sub_local(m, False), body)

    def sub_type(m):
        stats["unit_type constants"] += 1
        return "%s->unit_type %s %s" % (m.group(1), m.group(2), UNIT_TYPE.get(int(m.group(3)), m.group(3)))
    body = re.sub(r"\b(\w+)->unit_type (!=|==) (\d)\b", sub_type, body)

    def sub_class(m):
        kind = kind_of(m.group(1))
        name = class_enums.get(kind, {}).get(int(m.group(3)))
        if not name:
            return m.group(0)
        stats["unit_class constants"] += 1
        return "%s->unit_class %s %s" % (m.group(1), m.group(2), name)
    body = re.sub(r"\b(\w+)->unit_class (!=|==) (\d+)\b", sub_class, body)

    out = []
    for line in body.split("\n"):
        notes = []
        for m in re.finditer(r"\b(?:stats\.)?(\w*flags) (\|=|&=|&|\|) (~?)(0x[0-9A-Fa-f]+|\d+)u?", line):
            table = flag_names.get(m.group(1))
            if not table:
                continue
            value = int(m.group(4), 0)
            if m.group(3) == "~" or m.group(2) == "&=":
                value = ~value & 0xFFFFFFFF if m.group(2) == "&=" and m.group(3) != "~" else value
            bits = [1 << b for b in range(32) if value >> b & 1]
            if not bits or len(bits) > 6:
                continue
            notes.append(" | ".join(table.get(b, "0x%X?" % b) for b in bits))
            stats["flag notes"] += 1
        out.append(line + ("  /* %s */" % "; ".join(notes) if notes else ""))
    return "\n".join(out)


groups = collections.defaultdict(list)
for fn in index:
    key = "%08X" % fn["ea"]
    path = os.path.join(FUNCS, key + ".c")
    if not os.path.exists(path):
        continue
    loc = files.get(key, {})
    if fn["lib"]:
        bucket = "_library.c"
    elif loc.get("file"):
        bucket = loc["file"]
    elif loc.get("either"):
        bucket = "_between_%s_and_%s.c" % tuple((x or "none").rsplit(".", 1)[0] for x in loc["either"])
    else:
        bucket = "_unknown.c"
    groups[bucket].append((fn["ea"], key, loc))

os.makedirs(OUT, exist_ok=True)
for old in os.listdir(OUT):
    os.remove(os.path.join(OUT, old))
for bucket, funcs in groups.items():
    funcs.sort()  # address order equals source order within a file
    fallback = file_kind(bucket)
    with open(os.path.join(OUT, bucket if bucket.endswith(".c") else bucket + ".c"), "w") as f:
        f.write("// %s - %d functions, decompiled from ILB.exe and annotated by tools/by_file.py.\n" % (bucket, len(funcs)))
        f.write("// Not compilable: this is the reading copy for rewriting the file by hand.\n\n")
        for ea, key, loc in funcs:
            entry = names.get(key, {})
            f.write("//" + "-" * 77 + "\n")
            f.write("// %s  %s  name confidence: %s%s\n" % (
                key, entry.get("name") or "(unnamed)", entry.get("confidence", "none"),
                "  file from %s" % loc.get("source") if loc.get("source") and loc.get("source") != "assert" else ""))
            f.write(annotate(open(os.path.join(FUNCS, key + ".c")).read(), fallback))
            f.write("\n\n")
print("files written: %d" % len(groups))
print(dict(stats))
