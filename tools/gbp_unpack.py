#!/usr/bin/env python3
"""Unpack ILB.gbp (the game's pack file). Format read from packfile_load / pack_file_load in ec_fileio.cpp:

    dword file_count
    file_count x { dword stored_size; dword uncompressed_size; dword unknown; byte name_len; byte pad[3];
                   byte name[name_len] }      name bytes are stored +66
    file data, concatenated in directory order (stored_size bytes each)

A file whose stored size differs from its uncompressed size is LZW-compressed as 16-bit codes: 0-255 literals,
256 = reset table, new entries from 257 up to a 4096-entry table.

Usage: gbp_unpack.py <ILB.gbp> [--list] [--extract <dir>] [--only <substring>]
"""
import argparse
import os
import struct


def read_directory(blob):
    count = struct.unpack_from("<I", blob, 0)[0]
    pos = 4
    entries = []
    for _ in range(count):
        stored, size, unknown, name_len = struct.unpack_from("<IIIB", blob, pos)
        name = bytes((b - 66) & 0xFF for b in blob[pos + 16:pos + 16 + name_len]).decode("latin-1")
        entries.append({"name": name, "stored": stored, "size": size, "unknown": unknown})
        pos += 16 + name_len
    for e in entries:
        e["offset"] = pos
        pos += e["stored"]
    return entries, pos


def lzw_decode(raw):
    codes = struct.unpack_from("<%dH" % (len(raw) // 2), raw)
    prefix, suffix, first = {}, {}, {}
    out = bytearray()

    def expand(code):
        stack = []
        while code > 255:
            stack.append(suffix[code])
            code = prefix[code]
        out.append(code)
        while stack:
            out.append(stack.pop())

    def first_of(code):
        return code if code <= 255 else first[code]

    def add(prev, ch):
        nonlocal size
        if size < 4096:
            prefix[size], suffix[size], first[size] = prev, ch, first_of(prev)
            size += 1

    size = 257
    prev = codes[0]
    expand(prev)
    i = 1
    while i < len(codes):
        code = codes[i]
        i += 1
        if code == 256:
            size = 257
            if i >= len(codes):
                break
            code = codes[i]
            i += 1
            expand(code)
        elif code >= size:
            ch = first_of(prev)
            add(prev, ch)
            expand(prev)
            out.append(ch)
        else:
            expand(code)
            add(prev, first_of(code))
        prev = code
    return bytes(out)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("pack")
    ap.add_argument("--list", action="store_true")
    ap.add_argument("--extract")
    ap.add_argument("--only", default="")
    args = ap.parse_args()

    blob = open(args.pack, "rb").read()
    entries, end = read_directory(blob)
    print("%d files, directory + data end at %d of %d bytes" % (len(entries), end, len(blob)))
    bad = packed = 0
    for e in entries:
        if args.only not in e["name"]:
            continue
        if args.list:
            print("%10d %10d %08X %s" % (e["stored"], e["size"], e["unknown"], e["name"]))
        if args.extract:
            raw = blob[e["offset"]:e["offset"] + e["stored"]]
            if e["stored"] != e["size"]:
                packed += 1
                raw = lzw_decode(raw)
            if len(raw) != e["size"]:
                bad += 1
                print("SIZE MISMATCH %s: got %d, expected %d" % (e["name"], len(raw), e["size"]))
            path = os.path.join(args.extract, e["name"].replace("\\", "/"))
            os.makedirs(os.path.dirname(path), exist_ok=True)
            open(path, "wb").write(raw)
    if args.extract:
        print("extracted to %s: %d compressed, %d size mismatches" % (args.extract, packed, bad))


if __name__ == "__main__":
    main()
