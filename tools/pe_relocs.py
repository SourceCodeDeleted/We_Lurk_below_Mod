#!/usr/bin/env python3
"""The exe's sections and relocation table, shared by the draft tools.

A relocation slot is a 32-bit field the Windows loader adjusts: it holds a real address. A constant that
merely looks like an address (0x800000 used as a flag) has no slot. That distinction is exact.
"""
import os
import struct

EXE = os.environ.get("ILB_EXE", os.path.join(os.environ.get("ILB_GAME", os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "game")), "ILB.exe"))


class Image:
    def __init__(self, path=EXE):
        self.exe = exe = open(path, "rb").read()
        pe = struct.unpack_from("<I", exe, 0x3C)[0]
        assert exe[pe:pe + 4] == b"PE\0\0"
        num_sections, = struct.unpack_from("<H", exe, pe + 6)
        optional_size, = struct.unpack_from("<H", exe, pe + 20)
        optional = pe + 24
        self.base, = struct.unpack_from("<I", exe, optional + 28)
        self.size, = struct.unpack_from("<I", exe, optional + 56)
        reloc_rva, reloc_size = struct.unpack_from("<II", exe, optional + 96 + 5 * 8)
        self.sections = []
        for i in range(num_sections):
            o = optional + optional_size + i * 40
            vsize, rva, raw_size, raw = struct.unpack_from("<IIII", exe, o + 8)
            self.sections.append({"name": exe[o:o + 8].rstrip(b"\0").decode(), "rva": rva, "vsize": vsize,
                                  "raw": raw, "raw_size": raw_size})
        # slots: rva of every relocated 32-bit field, ascending
        self.slots, o, end = [], self.file_offset(reloc_rva), self.file_offset(reloc_rva) + reloc_size
        while o < end:
            page, size = struct.unpack_from("<II", exe, o)
            if size == 0:
                break
            for entry, in struct.iter_unpack("<H", exe[o + 8:o + size]):
                if entry >> 12 == 3:
                    self.slots.append(page + (entry & 0xFFF))
            o += size
        self.slots.sort()

    def section_of(self, rva):
        for s in self.sections:
            if s["rva"] <= rva < s["rva"] + max(s["vsize"], s["raw_size"]):
                return s
        return None

    def file_offset(self, rva):
        s = self.section_of(rva)
        return s["raw"] + (rva - s["rva"])

    def value_at(self, rva):
        """The 32-bit value stored in the file at this rva (0 for memory that is not in the file)."""
        s = self.section_of(rva)
        if s is None or rva - s["rva"] + 4 > s["raw_size"]:
            return 0
        return struct.unpack_from("<I", self.exe, s["raw"] + rva - s["rva"])[0]
