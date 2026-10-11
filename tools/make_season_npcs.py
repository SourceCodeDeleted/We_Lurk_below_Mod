#!/usr/bin/env python3
"""Make the seasonal art that port/mod_season_outfits.c and port/mod_season_buildings.c swap in by in-game
season: three vendor sprite sheets and the autumn furniture.

Every release of the game ships ONE vendor sheet (units\\npcs\\npcs.gs3) and ONE object sheet
(units\\objects_base.gs3) drawn for the time of year it came out, so the art has to come from three of the
owner's own copies of the game:

    plain      a build from outside the holidays (the Steam build of 2020-08-02 was used)
    halloween  an autumn build                    (the Steam build of 2024-11 was used)
    christmas  the GOG build this project decompiles (2022-01)

The vendor sheets share one layout (512x256, 32x32 cells). A cell that is empty in an older sheet (a resident
added later) is filled from the christmas sheet, which is the one the rebuilt game is made for.

The object sheets share the layout of their top 512x256 pixels, where the furniture is; an autumn build has
pumpkins painted onto some of it. One rectangle there holds different objects from release to release (the
Christmas tree, lights and presents in this build) and is kept from the christmas sheet.

Output, as the game's own sheet decoder produces it (little-endian pixel dwords), in
draft/build/native/run/mods/ (not in the repository: it is the game's art):

    npcs_plain.raw, npcs_halloween.raw, npcs_christmas.raw      512x256 each
    objects_halloween.raw                                       the top 512x256 of the object sheet for Autumn

Usage: make_season_npcs.py <plain ILB.gbp> <halloween ILB.gbp> [christmas ILB.gbp]
"""
import os
import struct
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TOOLS = os.path.join(ROOT, "tools")

CELL = 32


HOLIDAY_OBJECTS = (256, 192, 352, 256)       # x0, y0, x1, y1 in the object sheet: differs per release, see above


def sheet(pack, name="npcs", height=256):
    with tempfile.TemporaryDirectory() as tmp:
        subprocess.run([sys.executable, os.path.join(TOOLS, "gbp_unpack.py"), "--extract", tmp, "--only", name, pack],
                       check=True, stdout=subprocess.DEVNULL)
        found = [os.path.join(d, f) for d, _, files in os.walk(tmp) for f in files if f.lower() == name + ".gs3"]
        if len(found) != 1:
            sys.exit("%s: expected one %s.gs3, found %d" % (pack, name, len(found)))
        raw = os.path.join(tmp, "sheet.raw")
        subprocess.run([sys.executable, os.path.join(TOOLS, "gs3_to_png.py"), "--raw", found[0], raw], check=True, stdout=subprocess.DEVNULL)
        data = open(raw, "rb").read()
    if len(data) == 8 + 512 * height * 4 and struct.unpack_from("<II", data) == (512, height):
        data = data[8:]                     # the decoder writes width and height first
    if len(data) != 512 * height * 4:
        sys.exit("%s: %s sheet is %d bytes, not 512x%d pixels" % (pack, name, len(data), height))
    return list(struct.unpack("<%dI" % (512 * height), data))


def autumn_furniture(halloween, christmas):
    """Top 512x256 of the halloween object sheet, the per-release rectangle taken from the christmas one."""
    pixels = halloween[:512 * 256]
    x0, y0, x1, y1 = HOLIDAY_OBJECTS
    changed = 0
    for y in range(256):
        for x in range(512):
            i = y * 512 + x
            if x0 <= x < x1 and y0 <= y < y1:
                pixels[i] = christmas[i]
            elif pixels[i] != christmas[i]:
                changed += 1
    return pixels, changed


def fill_empty_cells(pixels, fallback):
    filled = 0
    for cy in range(0, 256, CELL):
        for cx in range(0, 512, CELL):
            cell = [(y * 512 + x) for y in range(cy, cy + CELL) for x in range(cx, cx + CELL)]
            if not any(pixels[i] for i in cell) and any(fallback[i] for i in cell):
                for i in cell:
                    pixels[i] = fallback[i]
                filled += 1
    return filled


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    christmas_pack = sys.argv[3] if len(sys.argv) > 3 else os.path.join(os.environ.get("ILB_GAME", os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "game")), "ILB.gbp")
    christmas = sheet(christmas_pack)
    out_dir = os.path.join(ROOT, "draft", "build", "native", "run", "mods")
    os.makedirs(out_dir, exist_ok=True)
    for name, pack in (("plain", sys.argv[1]), ("halloween", sys.argv[2]), ("christmas", christmas_pack)):
        pixels = christmas if name == "christmas" else sheet(pack)
        filled = 0 if name == "christmas" else fill_empty_cells(pixels, christmas)
        path = os.path.join(out_dir, "npcs_%s.raw" % name)
        open(path, "wb").write(struct.pack("<%dI" % len(pixels), *pixels))
        print("%s: %d cells taken from the christmas sheet -> %s" % (name, filled, path))
    pixels, changed = autumn_furniture(sheet(sys.argv[2], "objects_base", 512), sheet(christmas_pack, "objects_base", 512))
    path = os.path.join(out_dir, "objects_halloween.raw")
    open(path, "wb").write(struct.pack("<%dI" % len(pixels), *pixels))
    print("autumn furniture: %d pixels differ from the christmas sheet -> %s" % (changed, path))


if __name__ == "__main__":
    main()
