#!/bin/sh
# Symbol check without a linker: compile every draft file to an object and confirm that each symbol an
# object needs is defined by another object or is one of the DLL imports. Prints what is left over.
cd "$(dirname "$0")/.."
obj=draft/build/obj
# DRAFT_TRACE=1: a trace build, see draft/glue/trace.c
[ -n "$DRAFT_TRACE" ] && export TRACE_FLAGS="-finstrument-functions -DILB_TRACE"
rm -rf "$obj"; mkdir -p "$obj"
ls draft/build/src/*.c | xargs -P "$(nproc)" -I{} sh -c '
    f="{}"; o="'"$obj"'/$(basename "${f%.c}").o"
    clang -target i686-pc-windows-msvc -x c -std=gnu2x -fms-extensions -msse2 -c -O0 -Wno-everything $TRACE_FLAGS \
        -nostdinc -isystem "$(clang -print-resource-dir)/include" -I draft/include -I draft/build -I port "$f" -o "$o" 2> "${o%.o}.err" \
        || echo "FAILED $f"'
python3 tools/draft_symbols.py "$obj"
