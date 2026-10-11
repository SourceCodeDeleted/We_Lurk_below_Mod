#!/bin/sh
# Build the Linux program with CMake. Usage: tools/build_native.sh [headless|native|play] [--cheats]   (default headless)
#   headless: no window or sound, AddressSanitizer + UBSan -> draft/build/native/ILB_native (for the run scripts)
#   native:   SDL2 window + OpenGL                          -> build/native/ILB_native
#   --cheats: with the testing shortcuts of port/cheats.c (god mode and the like). Without it they are left
#             out, also when the build folder had them before.
# Needs the 32-bit C start files (sudo apt install gcc-multilib).
cd "$(dirname "$0")/.."
variant=headless
cheats=OFF
for arg in "$@"; do
    case "$arg" in
        --cheats) cheats=ON;;
        headless|native|play) variant="$arg";;
        *) echo "usage: tools/build_native.sh [headless|native|play] [--cheats]"; exit 1;;
    esac
done
# the import stubs are regenerated only where the IDA exports are present; otherwise the existing copy is used
if [ -f dump/imports.json ]; then python3 tools/native_stubs.py > /dev/null || exit 1; fi
cmake -G Ninja -B build/$variant -DILB_VARIANT=$variant -DILB_CHEATS=$cheats > build_$variant.cmake.log 2>&1 || { tail -20 build_$variant.cmake.log; exit 1; }
rm -f build_$variant.cmake.log
cmake --build build/$variant 2>&1 | grep -E "error|FAILED|warning: unused|ninja: build stopped|Linking|no work" | head -30
[ -f build/$variant/ILB_$variant ] || exit 1
if [ "$variant" = headless ]; then
    mkdir -p draft/build/native && cp build/headless/ILB_headless draft/build/native/ILB_native
fi
ls -la build/$variant/ILB_$variant
