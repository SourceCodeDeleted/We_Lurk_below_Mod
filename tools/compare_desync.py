#!/usr/bin/env python3
"""Compare the two copies' records of a network game that fell out of step.

Each copy writes net_desync_<step>.txt next to its log when the copies disagree (only with ILB_NET_DEBUG=1;
see "disagreement report" in draft/native/native_input.c). Both files hold the same steps, oldest first: the
random state, a hash of every 4 KB page of the game's globals and of every live unit. This shows, step by
step, what differs. Pages that always differ hold each copy's own business (picture, sound, window); a page
that agrees at one step and differs at the next is where the copies part, and the globals in it are named.

Usage: compare_desync.py <one copy's file> <the other copy's file> [--all]
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def read(path):
    steps, current = {}, None
    with open(path) as f:
        header = f.readline().strip()
        for line in f:
            p = line.split()
            if p[0] == "step":
                current = {"rand": p[3], "units": {}, "pages": {}}
                steps[int(p[1])] = current
            elif p[0] == "page":
                current["pages"][int(p[1], 16)] = p[2]
            elif p[0] == "unit":
                current["units"][int(p[1], 16)] = {"type": p[3], "class": p[5], "motion": p[7], "whole": p[9]}
    return header, steps


def global_names():
    """Address -> name of every global the generated declarations know (draft/build/ILB_draft_decls.h)."""
    names = {}
    path = os.path.join(ROOT, "draft", "build", "ILB_draft_decls.h")
    if os.path.exists(path):
        for m in re.finditer(r"#define (\w+) \(\*\([^)]*\)ILB_ADDR\(0x([0-9A-Fa-f]+)\)\)", open(path).read()):
            names.setdefault(int(m.group(2), 16), m.group(1))
    return names


def names_in(page, names):
    found = [f"{names[a]}" for a in sorted(names) if page <= a < page + 4096]
    return ", ".join(found[:12]) + (f" ... ({len(found)} in all)" if len(found) > 12 else "") if found else "(no named globals)"


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    if len(args) != 2:
        sys.exit(__doc__)
    show_all = "--all" in sys.argv
    (ha, a), (hb, b) = read(args[0]), read(args[1])
    print(f"A: {args[0]}\n   {ha}\nB: {args[1]}\n   {hb}\n")
    names = global_names()
    common = sorted(set(a) & set(b))
    if not common:
        sys.exit("the two files have no step in common")
    earlier = set()
    for step in common:
        sa, sb = a[step], b[step]
        pages = {p for p in sa["pages"] if sa["pages"][p] != sb["pages"].get(p)}
        ua, ub = sa["units"], sb["units"]
        only_a, only_b = sorted(set(ua) - set(ub)), sorted(set(ub) - set(ua))
        moved = [u for u in sorted(set(ua) & set(ub)) if ua[u]["motion"] != ub[u]["motion"]]
        changed = [u for u in sorted(set(ua) & set(ub)) if ua[u]["whole"] != ub[u]["whole"] and u not in moved]
        rand = "same" if sa["rand"] == sb["rand"] else "DIFFERS (%s / %s)" % (sa["rand"], sb["rand"])
        print(f"step {step}: random state {rand}; "
              f"units {len(ua)} / {len(ub)}; pages differing {len(pages)}")
        for u in only_a[:10]:
            print(f"  unit {u:x} (type {ua[u]['type']} class {ua[u]['class']}) only in A")
        for u in only_b[:10]:
            print(f"  unit {u:x} (type {ub[u]['type']} class {ub[u]['class']}) only in B")
        for u in moved[:10]:
            print(f"  unit {u:x} (type {ua[u]['type']} class {ua[u]['class']}) position or movement differs")
        if changed and show_all:
            for u in changed:
                print(f"  unit {u:x} (type {ua[u]['type']} class {ua[u]['class']}) other fields differ")
        elif changed:
            print(f"  ({len(changed)} units differ only in other fields - usually each copy's own picture data; --all lists them)")
        new = sorted(pages - earlier) if earlier or step != common[0] else []
        for p in (sorted(pages) if show_all or step == common[0] and len(pages) < 40 else new):
            print(f"  page {p:x}{' (newly different)' if p in new else ''}: {names_in(p, names)}")
        earlier = pages
    print("\nPages that differ at every step are usually each copy's own (picture, sound, window); look at the first")
    print("step where units or the random state differ, and at the pages that became different just before it.")


if __name__ == "__main__":
    main()
