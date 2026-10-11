#!/usr/bin/env python3
"""Write a small made-up input recording (format of draft/native/native_input.c) for determinism tests:
walking left and right, jumping, mouse moves and clicks, from tick 500 on. Used with a scripted start
(ILB_SCRIPT=load or coop), so no menu input is needed.

Usage: make_test_recording.py <out.rec> [seed] [last tick]
"""
import random
import struct
import sys

out, seed, last = sys.argv[1], int(sys.argv[2]) if len(sys.argv) > 2 else 1234, int(sys.argv[3]) if len(sys.argv) > 3 else 3000
rng = random.Random(seed)
events = []


def key(tick, down, scancode, sym):
    e = bytearray(56)
    struct.pack_into("<III", e, 0, 0x300 if down else 0x301, tick * 16, 1)
    e[12] = 1 if down else 0
    struct.pack_into("<II", e, 16, scancode, sym)
    events.append((tick, bytes(e)))


def motion(tick, x, y, buttons):
    e = bytearray(56)
    struct.pack_into("<IIIIIii", e, 0, 0x400, tick * 16, 1, 0, buttons, x, y)
    events.append((tick, bytes(e)))


def button(tick, down, x, y):
    e = bytearray(56)
    struct.pack_into("<IIII", e, 0, 0x401 if down else 0x402, tick * 16, 1, 0)
    e[16], e[17], e[18] = 1, 1 if down else 0, 1
    struct.pack_into("<ii", e, 20, x, y)
    events.append((tick, bytes(e)))


KEYS = {"a": (4, ord("a")), "d": (7, ord("d")), "space": (44, 32)}
tick = 500
while tick < last:
    name = rng.choice(["a", "d", "d", "space"])
    hold = rng.randint(5, 60)
    key(tick, True, *KEYS[name])
    key(tick + hold, False, *KEYS[name])
    x, y = rng.randint(300, 1600), rng.randint(200, 800)
    motion(tick + 1, x, y, 0)
    if rng.random() < 0.6:
        button(tick + 2, True, x, y)
        button(tick + 2 + rng.randint(1, 30), False, x, y)
    tick += hold + rng.randint(1, 40)
events.sort(key=lambda t: t[0])
with open(out, "wb") as f:
    f.write(b"ILBREC1\n" + struct.pack("<I", seed))
    for t, e in events:
        f.write(struct.pack("<I", t) + e)
print("%d events up to tick %d -> %s" % (len(events), events[-1][0], out))
