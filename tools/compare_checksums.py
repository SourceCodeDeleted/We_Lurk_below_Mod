#!/usr/bin/env python3
"""Compare two per-tick checksum files (ILB_CHECKSUM, draft/native/native_main.c) and show where they part.

Usage: compare_checksums.py <a.txt> <b.txt>      exit status 0 = identical over their common length
"""
import sys

a = open(sys.argv[1]).read().splitlines()
b = open(sys.argv[2]).read().splitlines()
n = min(len(a), len(b))
for i in range(n):
    if a[i] != b[i]:
        fa, fb = a[i].split(), b[i].split()
        names = [fa[j] for j in range(1, len(fa) - 1, 2) if j + 1 < len(fb) and fa[j + 1] != fb[j + 1]]
        print("OUT OF STEP at tick %s (line %d of %d): differs in %s" % (fa[0], i + 1, n, ", ".join(names)))
        for line in (a[max(0, i - 1)], a[i], b[i]):
            print("   " + line[:170])
        sys.exit(1)
print("identical for %d ticks (files have %d and %d)" % (n, len(a), len(b)))
