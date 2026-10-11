#!/bin/sh
# Syntax-check every file in src/ as 32-bit MSVC-flavoured C++ against the stub headers.
# This proves the code parses and type-checks; it does not produce a binary.
cd "$(dirname "$0")/.."
status=0
for f in src/*.cpp; do
    if clang++ -target i686-pc-windows-msvc -fsyntax-only -fms-extensions -Wall -Wno-unused-variable \
        -Wno-unused-function -nostdinc -isystem tools/stub_include/crt -isystem tools/stub_include/platform \
        -isystem "$(clang++ -print-resource-dir)/include" -Isrc "$f"; then
        echo "ok    $f"
    else
        echo "FAIL  $f"
        status=1
    fi
done
exit $status
