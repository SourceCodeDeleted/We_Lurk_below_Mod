#!/usr/bin/env python3
"""Map game_globals (0x60F3B0, 0x20A0 bytes) and client_data from how the decompiled code uses them.

IDA shows each used field of game_globals as its own global (dword_60F3C4, byte_611418, ...); the address
gives the offset, the prefix the width. client_data fields are the constant offsets read through
game_globals.client. Names come from assert expressions where a guarded read can be paired with one.
Writes db/game_globals.json (and prints a summary); src/g_game_globals.h is maintained by hand from it.
"""
import collections
import json
import os
import re

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FUNCS = os.path.join(ROOT, "dump", "functions")
BASE, SIZE = 0x60F3B0, 0x20A0
CLIENT = "60F3C4"

GLOBAL_RE = re.compile(r"\b(dword|byte|word|flt|dbl|qword|xmmword|unk|off|stru)_([0-9A-F]{6,7})\b")
CLIENT_RE = re.compile(r"\*\((?P<type>[\w ]+?) \*\)\(\*\(_DWORD \*\)dword_%s \+ (?P<off>\d+)\)" % CLIENT)
CLIENT_VAR_RE = re.compile(r"\b(\w+) = \*\(_DWORD \*\)dword_%s;" % CLIENT)
SITE_RE = re.compile(r'if \( (?P<cond>[^\n;{}]+) \)\s*\{?\s*(?:[\w()]+ = )?assert_failed\(\s*"(?P<expr>(?:[^"\\]|\\.)*)"')
WIDTH = {"dword": 4, "byte": 1, "word": 2, "flt": 4, "dbl": 8, "qword": 8, "xmmword": 16, "unk": 1, "off": 4, "stru": 0}

fields = collections.defaultdict(collections.Counter)   # offset -> prefix -> uses
funcs = collections.defaultdict(set)
client_fields = collections.defaultdict(collections.Counter)
named, client_named = {}, {}
for fname in sorted(os.listdir(FUNCS)):
    if not fname.endswith(".c"):
        continue
    body = open(os.path.join(FUNCS, fname)).read()
    for prefix, addr in GLOBAL_RE.findall(body):
        ea = int(addr, 16)
        if BASE <= ea < BASE + SIZE:
            fields[ea - BASE][prefix] += 1
            funcs[ea - BASE].add(fname[:8])
    for m in CLIENT_RE.finditer(body):
        client_fields[int(m.group("off"))][m.group("type").strip()] += 1
    aliases = set(CLIENT_VAR_RE.findall(body))
    for var in aliases:
        for m in re.finditer(r"\*\((?P<type>[\w ]+?) \*\)\(%s \+ (?P<off>\d+)\)" % re.escape(var), body):
            client_fields[int(m.group("off"))][m.group("type").strip()] += 1
    for m in SITE_RE.finditer(body):
        expr, cond = m.group("expr"), m.group("cond")
        g = re.fullmatch(r"game_globals\.(\w+)(?: [=!<>]=? .*)?", expr)
        if g:
            gs = [int(a, 16) - BASE for p, a in GLOBAL_RE.findall(cond) if BASE <= int(a, 16) < BASE + SIZE]
            if len(gs) == 1:
                named.setdefault(gs[0], collections.Counter())[g.group(1)] += 1
        c = re.fullmatch(r"game_globals\.client->(\w+)(?: [=!<>]=? .*)?", expr)
        if c:
            cm = CLIENT_RE.search(cond) or (aliases and re.search(
                r"\*\((?P<type>[\w ]+?) \*\)\((?:%s) \+ (?P<off>\d+)\)" % "|".join(map(re.escape, aliases)), cond))
            if cm:
                client_named.setdefault(int(cm.group("off")), collections.Counter())[c.group(1)] += 1

out = {"game_globals": [], "client_data": []}
for off in sorted(fields):
    prefix, n = fields[off].most_common(1)[0]
    out["game_globals"].append({"offset": off, "address": BASE + off, "kind": prefix, "uses": sum(fields[off].values()),
                                "functions": len(funcs[off]),
                                "name": named[off].most_common(1)[0][0] if off in named else None})
for off in sorted(client_fields):
    t, n = client_fields[off].most_common(1)[0]
    out["client_data"].append({"offset": off, "type": t, "uses": sum(client_fields[off].values()),
                               "name": client_named[off].most_common(1)[0][0] if off in client_named else None})
json.dump(out, open(os.path.join(ROOT, "db", "game_globals.json"), "w"), indent=1)
print("game_globals fields seen: %d (named %d); client_data fields seen: %d (named %d)" % (
    len(out["game_globals"]), sum(1 for f in out["game_globals"] if f["name"]),
    len(out["client_data"]), sum(1 for f in out["client_data"] if f["name"])))
