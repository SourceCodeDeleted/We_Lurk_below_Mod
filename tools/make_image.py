#!/usr/bin/env python3
"""Make the data image the program is built from, out of your own copy of the game.

    tools/make_image.py [<game folder>/ILB.exe]      ->  draft/build/ILB_image.bin

The image is the bytes of ILB.exe's sections as Windows loads them, which the game code reads its tables and
texts from; it is part of the game and so not in the repository. The game code in draft/build/src was made
from the GOG build of 2022-01-09 and only fits the image of that ILB.exe; any other is refused.

This is the first part of tools/draft_data.py on its own, for building from a checkout without the IDA
export; the result is checked against the image that script made.
"""
import hashlib
import os
import struct
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EXE = sys.argv[1] if len(sys.argv) > 1 else os.environ.get(
    "ILB_EXE", os.path.join(os.environ.get("ILB_GAME", os.path.join(ROOT, "game")), "ILB.exe"))
OUT = os.path.join(ROOT, "draft", "build", "ILB_image.bin")
EXPECTED = "a4e4f90bb509c552dc79bd17fbdb87fe092d389953aae59ca403ece52cc1e6c4"

if not os.path.isfile(EXE):
    sys.exit("make_image: %s not found (give the path of your copy's ILB.exe, or set ILB_GAME)" % EXE)
exe = open(EXE, "rb").read()
pe = struct.unpack_from("<I", exe, 0x3C)[0] if len(exe) > 0x40 else 0
if exe[pe:pe + 4] != b"PE\0\0":
    sys.exit("make_image: %s is not a Windows program" % EXE)
num_sections, = struct.unpack_from("<H", exe, pe + 6)
optional_size, = struct.unpack_from("<H", exe, pe + 20)

sections = []
for i in range(num_sections):
    o = pe + 24 + optional_size + i * 40
    name = exe[o:o + 8].rstrip(b"\0").decode(errors="replace")
    vsize, rva, raw_size, raw = struct.unpack_from("<IIII", exe, o + 8)
    if name in (".text", ".rdata", ".data"):
        sections.append((rva, min(raw_size, vsize), raw))
if not sections:
    sys.exit("make_image: %s has none of the sections expected" % EXE)
first = min(rva for rva, size, raw in sections)
blob = bytearray(max(rva + size for rva, size, raw in sections) - first)
for rva, size, raw in sections:
    blob[rva - first:rva - first + size] = exe[raw:raw + size]

if hashlib.sha256(blob).hexdigest() != EXPECTED:
    sys.exit("make_image: %s is not the ILB.exe the game code was made from (the GOG build of 2022-01-09)" % EXE)
os.makedirs(os.path.dirname(OUT), exist_ok=True)
open(OUT, "wb").write(blob)
print("make_image: %s written (%d bytes)" % (os.path.relpath(OUT, ROOT), len(blob)))
