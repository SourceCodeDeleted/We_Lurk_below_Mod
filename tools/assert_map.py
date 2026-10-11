#!/usr/bin/env python3
"""Wave 2: recover function names and source files from assert call sites.

The assert handler is __fastcall( ecx = expression, edx = file, stack: function signature, line, show_stack ).
Reads dump/ (index.json, strings.json, functions/*.c); writes db/asserts.json, db/names.json, db/files.json.
"""
import collections
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DUMP = os.path.join(ROOT, "dump")
DB = os.path.join(ROOT, "db")
ASSERT_EA = 0x401220

index = json.load(open(os.path.join(DUMP, "index.json")))
strings = {s["ea"]: s["text"] for s in json.load(open(os.path.join(DUMP, "strings.json")))}
SIG_RE = re.compile(r"__(cdecl|thiscall|stdcall|fastcall|vectorcall)\s+([\w:~<>]+)\s*\(")


# Call sites are read from the pseudo-C (the handler's prototype is applied in the work database), because
# the compiler merges the call instruction of neighbouring asserts and the disassembly then shows one call.
# Arguments may carry a cast, long literals are split into adjacent pieces, and the expression or line is
# sometimes a variable when several asserts share one call.
CAST = r"(?:\(\w[\w \*]*\)\s*)?"
STRS = r'((?:"(?:[^"\\]|\\.)*"\s*)+)'
CALL_RE = re.compile(r"assert_failed\(\s*%s(?:%s|\w+),\s*%s%s,\s*%s%s,\s*(\w+)\s*," % (
    CAST, STRS, CAST, STRS, CAST, STRS))
PIECE_RE = re.compile(r'"((?:[^"\\]|\\.)*)"')


def literal(text):
    if text is None:
        return None
    joined = "".join(PIECE_RE.findall(text))
    return joined.replace("\\\\", "\\").replace('\\"', '"')


sites = []
per_func = collections.defaultdict(list)
unparsed = 0
for fn in index:
    path = os.path.join(DUMP, "functions", "%08X.c" % fn["ea"])
    if not os.path.exists(path):
        continue
    body = open(path).read()
    found = 0
    for m in CALL_RE.finditer(body):
        found += 1
        site = {"func": fn["ea"], "call": m.start(), "expr": literal(m.group(1)),
                "file": literal(m.group(2)).rsplit("\\", 1)[-1], "sig": literal(m.group(3)),
                "line": int(m.group(4)) if m.group(4).isdigit() else None}
        sites.append(site)
        per_func[fn["ea"]].append(site)
    unparsed += body.count("assert_failed(") - found

# Resolve each function's own signature. Inlined callees bring their asserts along, so one function can hold
# several signatures. Two facts about this binary make the choice tractable (measured on the unambiguous set):
# functions are laid out one source file at a time, and within a file source line rises with address.
#   high   - the function holds one signature and no other function holds it
#   medium - several candidates; exactly one sits in the expected file with a line between the neighbouring
#            anchors, or the latest-defined such candidate is also the first assert in the function
#   low    - best guess only (latest-defined candidate that fits); the function may have no assert of its own
def key(site):
    return (site["sig"], site["file"])


cands = {}   # ea -> ordered list of (sig, file), by first call address
first_line = {}  # (ea, key) -> lowest line seen
hosts = collections.defaultdict(set)
for ea, ss in per_func.items():
    seen = []
    for s in sorted(ss, key=lambda s: s["call"]):
        k = key(s)
        if k not in seen:
            seen.append(k)
        if s["line"]:
            first_line[(ea, k)] = min(first_line.get((ea, k), 1 << 30), s["line"])
        hosts[k].add(ea)
    cands[ea] = seen

resolved = {}  # ea -> (key, confidence)
for ea, ks in cands.items():
    if len(ks) == 1 and len(hosts[ks[0]]) == 1:
        resolved[ea] = (ks[0], "high")

# A function with no assert of its own that inlines exactly one callee looks "high" by the rule above but
# carries the callee's name (0x402FC0 holds an inlined debug_log_add_char_str and sits among the
# ec_controller functions). Such a function shows up as a short run of one file away from that file's main
# block; those are demoted to "low".
runs = []
for ea in sorted(resolved):
    f = resolved[ea][0][1]
    if runs and runs[-1][0] == f:
        runs[-1][1].append(ea)
    else:
        runs.append((f, [ea]))
largest = {}
for f, eas in runs:
    largest[f] = max(largest.get(f, 0), len(eas))
demoted = 0
for f, eas in runs:
    if len(eas) <= 3 and len(eas) < largest[f]:
        for ea in eas:
            resolved[ea] = (resolved[ea][0], "low")
            demoted += 1

order = sorted(fn["ea"] for fn in index)
for _ in range(3):  # later passes use names fixed by earlier ones as anchors
    anchors = sorted((ea, k[1], first_line.get((ea, k))) for ea, (k, c) in resolved.items()
                     if c in ("high", "medium") and first_line.get((ea, k)))
    anchor_eas = [a[0] for a in anchors]
    import bisect
    owned = {k for k, c in resolved.values() if c != "low"}
    for ea in order:
        if (ea in resolved and resolved[ea][1] != "low") or ea not in cands:
            continue
        i = bisect.bisect_left(anchor_eas, ea)
        prev = anchors[i - 1] if i > 0 else None
        nxt = anchors[i] if i < len(anchors) else None
        fits = []
        for k in cands[ea]:
            if k in owned:
                continue
            line = first_line.get((ea, k))
            ok = False
            for a, lo_side in ((prev, True), (nxt, False)):
                if a and a[1] == k[1] and line:
                    other = nxt if lo_side else prev
                    lo = a[2] if lo_side else (other[2] if other and other[1] == k[1] else 0)
                    hi = (other[2] if other and other[1] == k[1] else 1 << 30) if lo_side else a[2]
                    if lo < line < hi:
                        ok = True
            if ok:
                fits.append((line, k))
        if not fits:
            continue
        fits.sort()
        best = fits[-1][1]
        first = cands[ea][0]
        if len(fits) == 1 or best == first:
            resolved[ea] = (best, "medium")
        else:
            resolved[ea] = (best, "low")
        owned.add(best)

names, files = {}, {}
for fn in index:
    ea = fn["ea"]
    ss = per_func.get(ea, [])
    entry = {"ida_name": fn["name"], "size": fn["size"]}
    own = None
    if ea in resolved:
        own, conf = resolved[ea]
        entry.update(name=SIG_RE.search(own[0]).group(2), sig=own[0], confidence=conf)
    elif ea in cands:
        entry.update(name=None, confidence="ambiguous")
    if ea in cands:
        entry["inlined"] = [k[0] for k in cands[ea] if k != own]
        names["%08X" % ea] = entry
    if own and conf != "low":  # a low name may be an inlined callee's, so its file is not trusted either
        files["%08X" % ea] = {"file": own[1], "source": "assert", "line": first_line.get((ea, own))}

# File for everything else: between two functions of the same file, the file is known; at a boundary it is
# one of the two. Library code is left unassigned.
known = sorted((int(k, 16), v["file"]) for k, v in files.items())
known_eas = [k[0] for k in known]
for fn in index:
    k = "%08X" % fn["ea"]
    if k in files or fn["lib"]:
        continue
    i = bisect.bisect_left(known_eas, fn["ea"])
    prev = known[i - 1][1] if i > 0 else None
    nxt = known[i][1] if i < len(known) else None
    if prev and prev == nxt:
        files[k] = {"file": prev, "source": "neighbours", "line": None}
    elif prev or nxt:
        files[k] = {"file": None, "source": "boundary", "either": [prev, nxt], "line": None}

# Names established by reading the code, kept in db/name_overrides.json: they win over anything inferred.
overrides_path = os.path.join(DB, "name_overrides.json")
if os.path.exists(overrides_path):
    by_ea = {"%08X" % fn["ea"]: fn for fn in index}
    for k, o in json.load(open(overrides_path)).items():
        entry = names.setdefault(k, {"ida_name": by_ea[k]["name"], "size": by_ea[k]["size"]})
        if entry.get("sig") and entry.get("name") != o["name"]:
            entry.setdefault("inlined", []).insert(0, entry.pop("sig"))
        entry.update(name=o["name"], confidence="manual", why=o["why"])
        if o.get("file"):
            files[k] = {"file": o["file"], "source": "manual", "line": files.get(k, {}).get("line")}

os.makedirs(DB, exist_ok=True)
json.dump(sites, open(os.path.join(DB, "asserts.json"), "w"), indent=1)
json.dump(names, open(os.path.join(DB, "names.json"), "w"), indent=1)
json.dump(files, open(os.path.join(DB, "files.json"), "w"), indent=1)

all_sigs = {t for t in strings.values() if SIG_RE.search(t)}
conf = collections.Counter(e.get("confidence") for e in names.values())
print("assert call sites: %d (calls the parser could not read: %d)" % (len(sites), unparsed))
print("functions with asserts: %d of %d" % (len(per_func), len(index)))
print("signature strings in exe: %d, seen at assert sites: %d, assigned to a function: %d" % (
    len(all_sigs), len({k[0] for k in hosts}), len({k[0] for k, c in resolved.values()})))
print("naming confidence:", dict(conf), "| demoted misplaced single-signature hosts: %d" % demoted)
print("source file:", dict(collections.Counter(v["source"] for v in files.values())),
      "unassigned: %d" % (len(index) - len(files)))
if "-v" in sys.argv:
    missing = sorted(all_sigs - {k[0] for k in hosts})
    print("signatures never seen at an assert site: %d" % len(missing))
    for m in missing[:15]:
        print("   ", m)
