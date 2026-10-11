#!/usr/bin/env python3
"""Wave 3: recover struct field names and offsets from assert expressions.

An assert's first argument is the source expression ("item->unit_type == unit_type_item"). When the pseudo-C
guards the call with a condition that reads exactly one memory location (*(_DWORD *)(a1 + 2080) != 3), the field
named in the expression lives at that offset. Every site votes; db/fields.json keeps the tallies.
"""
import collections
import json
import os
import re

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FUNCS = os.path.join(ROOT, "dump", "functions_untyped")  # snapshot taken before struct types were applied
CUSTOM_DATA_OFFSET = 2792  # unit.custom_data; *cd pointers are often folded into unit-relative offsets

STR = r'((?:"(?:[^"\\]|\\.)*"\s*)+)'
SITE_RE = re.compile(
    r"if \( (?P<cond>[^\n;{}]+) \)\s*\{?\s*(?:[\w()]+ = )?assert_failed\(\s*(?:\(\w[\w \*]*\)\s*)?" + STR)
CAST_RE = re.compile(r"\*\((?P<type>[\w ]+?) \*\)\((?P<var>\w+) \+ (?P<off>\d+)\)")
PLAIN_RE = re.compile(r"\*\((?P<var>\w+) \+ (?P<off>\d+)\)|(?<![\w.>])(?P<var2>\w+)\[(?P<off2>\d+)\]")
DEREF0_RE = re.compile(r"\*\((?P<type>[\w ]+?) \*\)(?P<var>\w+)\b(?! \+)")
DECL_RE = re.compile(r"^\s*(?:const )?(?P<type>[\w ]+?) \*(?:__\w+ )?(?P<var>\w+)(?:;|,|\)| //)", re.M)
PARAM_RE = re.compile(r"(?P<type>[\w ]+?) \*(?P<var>\w+)[,)]")
FIELD_RE = re.compile(r"\b([A-Za-z_]\w*)->([A-Za-z_]\w*)")
SIZE = {"_DWORD": 4, "int": 4, "unsigned int": 4, "float": 4, "_BYTE": 1, "char": 1, "bool": 1,
        "unsigned __int8": 1, "_WORD": 2, "__int16": 2, "unsigned __int16": 2}
CTYPE = {"_DWORD": "dword", "int": "int", "unsigned int": "dword", "float": "float", "_BYTE": "byte",
         "char": "byte", "bool": "bool", "unsigned __int8": "byte", "_WORD": "word", "__int16": "short",
         "unsigned __int16": "word"}

votes = collections.defaultdict(collections.Counter)   # (base, field) -> {(offset, ctype): n}
examples = {}
sites = paired = 0
for fname in sorted(os.listdir(FUNCS)):
    if not fname.endswith(".c"):
        continue
    body = open(os.path.join(FUNCS, fname)).read()
    if "assert_failed(" not in body:
        continue
    ptr_type = {m.group("var"): m.group("type").strip() for m in DECL_RE.finditer(body)}
    header = body.split("{", 1)[0]
    for m in PARAM_RE.finditer(header):
        ptr_type[m.group("var")] = m.group("type").strip().split("(")[-1].strip()
    for m in SITE_RE.finditer(body):
        sites += 1
        expr = "".join(re.findall(r'"((?:[^"\\]|\\.)*)"', m.group(2)))
        fields = FIELD_RE.findall(expr)
        if len(fields) != 1 or "[" in expr:
            continue
        cond = m.group("cond")
        reads = []
        for c in CAST_RE.finditer(cond):
            reads.append((int(c.group("off")), c.group("type").strip()))
        stripped = CAST_RE.sub("", cond)
        for c in DEREF0_RE.finditer(stripped):
            reads.append((0, c.group("type").strip()))
        stripped = DEREF0_RE.sub("", stripped)
        for c in PLAIN_RE.finditer(stripped):
            var = c.group("var") or c.group("var2")
            idx = int(c.group("off") or c.group("off2"))
            t = ptr_type.get(var)
            if t in SIZE:
                reads.append((idx * SIZE[t], t))
            else:
                reads.append((None, None))
        if len(reads) != 1 or reads[0][0] is None or reads[0][1] not in SIZE:
            continue
        offset, t = reads[0]
        base, field = fields[0]
        if base.endswith("cd") and offset >= CUSTOM_DATA_OFFSET:
            offset -= CUSTOM_DATA_OFFSET
        votes[(base, field)][(offset, CTYPE[t])] += 1
        examples.setdefault((base, field, offset), "%s: %s  <=  %s" % (fname[:8], expr, cond))
        paired += 1

out = []
for (base, field), tally in sorted(votes.items()):
    total = sum(tally.values())
    (offset, ctype), n = tally.most_common(1)[0]
    out.append({"base": base, "field": field, "offset": offset, "type": ctype, "votes": n, "total": total,
                "others": [[o, t, c] for (o, t), c in tally.most_common()[1:4]],
                "example": examples[(base, field, offset)]})
json.dump(out, open(os.path.join(ROOT, "db", "fields.json"), "w"), indent=1)
print("guarded assert sites: %d, paired with one field and one memory read: %d" % (sites, paired))
print("distinct (variable, field) pairs: %d, of which agreed by 2+ sites: %d" % (
    len(out), sum(1 for e in out if e["votes"] >= 2)))
