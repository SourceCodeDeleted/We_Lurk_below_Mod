#!/usr/bin/env python3
"""Decode .gs3 sprite files (format read from gge_gs3.cpp in ILB.exe) to PNG, and check the format reading.

    dword num_colors          0xFFFFFFFF = 32-bit sprites, otherwise an 8-bit palette of that many dwords follows
    dword num_sprites
    dword offset[num_sprites] from the start of the file
  each sprite:
    word height - 1, word width - 1
    rows of runs until `width` pixels are covered:
      32-bit: dword control: low 14 bits + high word = count - 1, bits 14-15 = kind
      8-bit:  byte control (bits 6-7 = kind), byte count - 1
      kind 0 = `count` literal pixels follow, 2 = one pixel repeated, 1 = skip `count` transparent pixels

Usage: gs3_to_png.py --check <dir>          decode every sprite of every .gs3 under dir, verify sizes
       gs3_to_png.py --manifest <dir> <out.txt>  name, size and pixel CRC of sprite 0 of every file
       gs3_to_png.py <file.gs3> <out.png> [sprite_index]
       gs3_to_png.py --raw <file.gs3> <out.bin> [sprite_index]   (width*height little-endian dwords)
"""
import os
import struct
import sys
import zlib


def decode(blob, index=0):
    """Returns (width, height, list of pixel dwords, bytes consumed, sprite offset, num_sprites)."""
    num_colors = struct.unpack_from("<I", blob, 0)[0]
    pos = 4
    palette = None
    if num_colors != 0xFFFFFFFF:
        palette = struct.unpack_from("<%dI" % num_colors, blob, pos)
        pos += 4 * num_colors
    num_sprites = struct.unpack_from("<I", blob, pos)[0]
    offsets = struct.unpack_from("<%dI" % num_sprites, blob, pos + 4)
    start = offsets[index]
    h1, w1 = struct.unpack_from("<HH", blob, start)
    width, height = w1 + 1, h1 + 1
    p = start + 4
    pixels = [0] * (width * height)
    for y in range(height):
        x = 0
        while x < width:
            if palette is None:
                ctrl = struct.unpack_from("<I", blob, p)[0]
                p += 4
                count = (ctrl & 0xFFFF3FFF) + 1
                kind = ctrl & 0xC000
                if kind == 0:
                    vals = struct.unpack_from("<%dI" % count, blob, p)
                    p += 4 * count
                    for i, v in enumerate(vals):
                        if x + i < width:
                            pixels[y * width + x + i] = v
                elif kind == 0x8000:
                    v = struct.unpack_from("<I", blob, p)[0]
                    p += 4
                    for i in range(count):
                        if x + i < width:
                            pixels[y * width + x + i] = v
                elif kind != 0x4000:
                    raise ValueError("bad 32-bit control %08X at %d" % (ctrl, p - 4))
            else:
                ctrl, c1 = blob[p], blob[p + 1]
                p += 2
                count = c1 + 1
                kind = ctrl & 0xC0
                if kind == 0:
                    for i in range(count):
                        if x + i < width:
                            pixels[y * width + x + i] = palette[blob[p + i]]
                    p += count
                elif kind == 0x80:
                    v = palette[blob[p]]
                    p += 1
                    for i in range(count):
                        if x + i < width:
                            pixels[y * width + x + i] = v
                elif kind != 0x40:
                    raise ValueError("bad 8-bit control %02X at %d" % (ctrl, p - 2))
            x += count
        if x != width:
            raise ValueError("row %d ends at %d, width %d" % (y, x, width))
    return width, height, pixels, p - start, start, num_sprites, offsets


def write_png(path, width, height, pixels):
    def chunk(tag, data):
        return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)
    raw = bytearray()
    for y in range(height):
        raw.append(0)
        for v in pixels[y * width:(y + 1) * width]:
            raw += bytes(((v >> 16) & 0xFF, (v >> 8) & 0xFF, v & 0xFF, v >> 24))   # stored as B, G, R, A
    open(path, "wb").write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0))
                           + chunk(b"IDAT", zlib.compress(bytes(raw), 6)) + chunk(b"IEND", b""))


if sys.argv[1] == "--check":
    files = sprites = bad = exact = 0
    kinds = {"8-bit": 0, "32-bit": 0}
    for root, _, names in os.walk(sys.argv[2]):
        for name in sorted(names):
            if not name.endswith(".gs3"):
                continue
            blob = open(os.path.join(root, name), "rb").read()
            files += 1
            kinds["32-bit" if blob[:4] == b"\xff\xff\xff\xff" else "8-bit"] += 1
            try:
                n = decode(blob, 0)[5]
                for i in range(n):
                    w, h, px, used, start, _, offsets = decode(blob, i)
                    sprites += 1
                    end = min([o for o in offsets if o > start] + [len(blob)])
                    if start + used == end:
                        exact += 1
            except Exception as e:
                bad += 1
                print("FAILED %s: %s" % (name, e))
    print("%d files (%s), %d sprites decoded, %d failed files; %d sprites end exactly where the next one starts" % (
        files, ", ".join("%d %s" % (v, k) for k, v in kinds.items()), sprites, bad, exact))
elif sys.argv[1] == "--manifest":
    # one line per file: pack name, 8 or 32 (bits), width, height, CRC-32 of sprite 0's pixels (little-endian dwords)
    out = open(sys.argv[3], "w")
    for root, _, names in os.walk(sys.argv[2]):
        for name in sorted(names):
            if name.endswith(".gs3"):
                path = os.path.join(root, name)
                blob = open(path, "rb").read()
                w, h, px = decode(blob, 0)[:3]
                crc = zlib.crc32(struct.pack("<%dI" % len(px), *px)) & 0xFFFFFFFF
                bits = 32 if blob[:4] == b"\xff\xff\xff\xff" else 8
                out.write("%s %d %d %d %08X\n" % (os.path.relpath(path, sys.argv[2]).replace("/", "\\"), bits, w, h, crc))
elif sys.argv[1] == "--raw":
    w, h, px = decode(open(sys.argv[2], "rb").read(), int(sys.argv[4]) if len(sys.argv) > 4 else 0)[:3]
    open(sys.argv[3], "wb").write(struct.pack("<II%dI" % len(px), w, h, *px))
else:
    w, h, px = decode(open(sys.argv[1], "rb").read(), int(sys.argv[3]) if len(sys.argv) > 3 else 0)[:3]
    write_png(sys.argv[2], w, h, px)
    print("%dx%d" % (w, h))
