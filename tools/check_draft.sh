#!/bin/sh
# Compile-check the mechanical draft, one process per source file (syntax and types only).
# Usage: tools/check_draft.sh            all of draft/build/src/*.c
#        tools/check_draft.sh file.c ... only these
#        DRAFT_FLAGS=-DILB_HANDWRITTEN tools/check_draft.sh   without the functions src/ already has
cd "$(dirname "$0")/.."
[ $# -eq 0 ] && set -- draft/build/src/*.c
printf '%s\n' "$@" | xargs -P "$(nproc)" -I{} sh -c '
    f="{}"
    timeout "${DRAFT_TIMEOUT:-900}" clang -target i686-pc-windows-msvc -x c -std=gnu2x -fms-extensions -msse2 -fsyntax-only $DRAFT_FLAGS \
        -ferror-limit=0 -Wno-everything -Werror=implicit-function-declaration \
        -nostdinc -isystem "$(clang -print-resource-dir)/include" -I draft/include -I draft/build -I port "$f" 2> "${f%.c}.err"
    status=$?
    # a compiler that was killed or crashed must not look like a clean file
    [ $status -ne 0 ] && ! grep -q "error:" "${f%.c}.err" && echo "error: clang exited with status $status" >> "${f%.c}.err"
    true'
total=0; clean=0; n=0
for f in "$@"; do
    e=$(grep -c "error:" "${f%.c}.err"); total=$((total + e)); n=$((n + 1)); [ "$e" -eq 0 ] && clean=$((clean + 1))
done
echo "$n files, $clean compile cleanly, $total errors in total"
[ "$total" -eq 0 ]
