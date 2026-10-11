#!/usr/bin/env python3
"""Print what a compiled Direct3D shader (.vso / .pso from ILB.gbp) does: constants, inputs, outputs, code.

Usage: tools/dxbc_dis.py dump/gbp/shaders/dx11/sprite.pso [...]
Used to rewrite the game's 13 shader pairs as GLSL for the OpenGL renderer (port/shaders/).
"""
import struct
import sys

OPCODES = """add and break breakc call callc case continue continuec cut default deriv_rtx deriv_rty discard div
dp2 dp3 dp4 else emit emitthencut endif endloop endswitch eq exp frc ftoi ftou ge iadd if ieq ige ilt imad imax
imin imul ine ineg ishl ishr itof label ld ld_ms log loop lt mad min max customdata mov movc mul ne nop not or
resinfo ret retc round_ne round_ni round_pi round_z rsq sample sample_c sample_c_lz sample_l sample_d sample_b
sqrt switch sincos udiv ult uge umul umad umax umin ushr utof xor dcl_resource dcl_constantbuffer dcl_sampler
dcl_index_range dcl_gs_output_topology dcl_gs_input_primitive dcl_max_output_vertex_count dcl_input
dcl_input_sgv dcl_input_siv dcl_input_ps dcl_input_ps_sgv dcl_input_ps_siv dcl_output dcl_output_sgv
dcl_output_siv dcl_temps dcl_indexable_temp dcl_global_flags""".split()
OPERAND = {0: "r", 1: "v", 2: "o", 3: "x", 6: "s", 7: "t", 8: "cb"}


def cstring(data, at):
    return data[at:data.index(b"\0", at)].decode("latin1")


def operand(tokens, at):
    """(text, next index)"""
    token = tokens[at]
    at += 1
    while token & 0x80000000 and at < len(tokens):       # extended operand tokens (modifiers)
        ext = tokens[at]
        at += 1
        modifier = {1: "-", 2: "abs ", 3: "-abs "}.get((ext >> 6) & 0xFF, "")
        token = (token & 0x7FFFFFFF) | (0 if not ext & 0x80000000 else 0x80000000)
        prefix = modifier
        break
    else:
        prefix = ""
    components = token & 3
    mode = (token >> 2) & 3
    kind = (token >> 12) & 0xFF
    dims = (token >> 20) & 3
    if kind == 4:                                           # immediate
        count = 1 if components == 1 else 4
        values = [struct.unpack("<f", struct.pack("<I", v))[0] for v in tokens[at:at + count]]
        raw = tokens[at:at + count]
        text = "l(" + ", ".join(("%g" % f) if abs(f) > 1e-6 and abs(f) < 1e9 else str(r if r < 0x80000000 else r - (1 << 32)) for f, r in zip(values, raw)) + ")"
        return prefix + text, at + count
    indices = []
    for i in range(dims):
        representation = (token >> (22 + 3 * i)) & 7
        if representation == 0:
            indices.append(str(tokens[at]))
            at += 1
        elif representation == 2:
            sub, at = operand(tokens, at)
            indices.append(sub)
        elif representation == 3:
            base = tokens[at]
            sub, at = operand(tokens, at + 1)
            indices.append("%d + %s" % (base, sub))
        else:
            indices.append("?")
    name = OPERAND.get(kind, "op%d_" % kind)
    text = name + (indices[0] if indices else "") + "".join("[%s]" % i for i in indices[1:])
    if components == 2:
        if mode == 0:
            mask = (token >> 4) & 15
            text += "." + "".join(c for i, c in enumerate("xyzw") if mask & (1 << i))
        elif mode == 1:
            swizzle = (token >> 4) & 0xFF
            text += "." + "".join("xyzw"[(swizzle >> (2 * i)) & 3] for i in range(4))
        else:
            text += "." + "xyzw"[(token >> 4) & 3]
    return prefix + text, at


def show(path):
    data = open(path, "rb").read()
    assert data[:4] == b"DXBC", path
    count, = struct.unpack_from("<I", data, 28)
    print("==== %s (%d bytes)" % (path.split("/")[-1], len(data)))
    for i in range(count):
        offset, = struct.unpack_from("<I", data, 32 + 4 * i)
        tag, size = data[offset:offset + 4].decode(), struct.unpack_from("<I", data, offset + 4)[0]
        chunk = data[offset + 8:offset + 8 + size]
        if tag == "RDEF":
            num_cb, cb_at, num_bind, bind_at = struct.unpack_from("<IIII", chunk, 0)
            for b in range(num_bind):
                name, kind, _, _, _, point, _, _ = struct.unpack_from("<8I", chunk, bind_at + 32 * b)
                print("  bind %-8s slot %d  %s" % ({0: "cbuffer", 2: "texture", 3: "sampler"}.get(kind, kind), point, cstring(chunk, name)))
            for c in range(num_cb):
                name, num_var, var_at, cb_size, _, _ = struct.unpack_from("<6I", chunk, cb_at + 24 * c)
                print("  cbuffer %s (%d bytes)" % (cstring(chunk, name), cb_size))
                for v in range(num_var):
                    vname, start, vsize, flags, type_at, _ = struct.unpack_from("<6I", chunk, var_at + 24 * v)
                    cls, typ, rows, cols = struct.unpack_from("<4H", chunk, type_at)
                    print("      +%-3d %-22s %dx%d (%d bytes)%s" % (start, cstring(chunk, vname), rows, cols, vsize, "" if flags & 2 else "  unused"))
        elif tag in ("ISGN", "OSGN"):
            n, = struct.unpack_from("<I", chunk, 0)
            for e in range(n):
                name, index, sysval, ctype, register, mask = struct.unpack_from("<5IB", chunk, 8 + 24 * e)
                print("  %s %s%d  %s%d.%s" % ("in " if tag == "ISGN" else "out", cstring(chunk, name), index,
                                               "v" if tag == "ISGN" else "o", register, "".join(c for k, c in enumerate("xyzw") if mask & (1 << k))))
        elif tag in ("SHDR", "SHEX"):
            tokens = struct.unpack_from("<%dI" % (len(chunk) // 4), chunk, 0)
            at = 2
            while at < len(tokens):
                token = tokens[at]
                opcode, length = token & 0x7FF, (token >> 24) & 0x7F
                if opcode == 53:                            # custom data: length in the next token
                    length = tokens[at + 1]
                    at += length
                    continue
                name = OPCODES[opcode] if opcode < len(OPCODES) else "op%d" % opcode
                if (token >> 13) & 1 and not name.startswith("dcl"):
                    name += "_sat"
                cur, parts = at + 1, []
                if token & 0x80000000:
                    cur += 1
                try:
                    if name in ("dcl_temps", "dcl_global_flags"):
                        parts = [str(tokens[cur])] if name == "dcl_temps" else []
                    else:
                        while cur < at + length:
                            text, cur = operand(tokens, cur)
                            parts.append(text)
                            if name.startswith("dcl_input_ps_s") or name.endswith("_sgv") or name.endswith("_siv"):
                                break
                except Exception as e:                      # a declaration form this tool does not decode
                    parts.append("(%s)" % e)
                print("    %-18s %s" % (name, ", ".join(parts)))
                at += max(length, 1)


for path in sys.argv[1:]:
    show(path)
