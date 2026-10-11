#!/bin/sh
# List every variable the draft uses without setting it. Each one is a value the decompiler lost (usually a
# register that stays live across a call) and is a real bug in the draft until fixed.
# Output: draft/build/uninitialised.txt   (file:line: variable), and a count per file.
cd "$(dirname "$0")/.."
out=draft/build/uninitialised.txt
ls draft/build/src/*.c | xargs -P "$(nproc)" -I{} sh -c 'clang -target i686-pc-windows-msvc -x c -std=gnu2x -fms-extensions -msse2 -fsyntax-only \
    -Wno-everything -Wuninitialized -Wsometimes-uninitialized -nostdinc -isystem "$(clang -print-resource-dir)/include" \
    -I draft/include -I draft/build {} 2> {}.uninit; true'
cat draft/build/src/*.uninit | grep "warning:" | sed -E "s/: warning: variable '([^']*)'.*\[-W(sometimes-)?uninitialized\]/: \1 \2/; s|draft/build/src/||" | sort -t: -k1,1 -k2,2n > $out
rm -f draft/build/src/*.uninit
echo "$(wc -l < $out) uninitialised uses ($(grep -c 'sometimes-' $out) only on some paths), list in $out"
cut -d: -f1 $out | sort | uniq -c | sort -rn | head -8
