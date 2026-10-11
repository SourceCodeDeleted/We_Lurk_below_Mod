#!/bin/sh
# Link the mechanical draft into draft/build/ILB_draft.exe, a windowed program like the original (needs lld-link-14 and the libraries from xwin).
# Run tools/draft_symbols.sh first: it compiles the objects this script links.
# Usage: tools/link_draft.sh            XWIN=<dir> to use libraries from somewhere other than ~/xwin
cd "$(dirname "$0")/.."
xwin="${XWIN:-$HOME/xwin}"
build=draft/build
game="${ILB_GAME:-$PWD/game}"
mkdir -p $build/lib

# import libraries for the two DLLs that ship with the game, from the list of what the original imports
python3 tools/draft_imports.py $build/lib || exit 1
for dll in SDL2 fmod; do
    llvm-dlltool-14 -m i386 -k -d $build/lib/$dll.def -l $build/lib/$dll.lib -D $dll.dll || exit 1
done

clang -target i686-pc-windows-msvc -x c -std=gnu2x -fms-extensions -msse2 -c -O0 -Wno-everything ${DRAFT_TRACE:+-DILB_TRACE} \
    -nostdinc -isystem "$(clang -print-resource-dir)/include" -I draft/include -I $build -I port \
    draft/glue/startup.c -o $build/obj/_startup.o || exit 1
rm -f $build/obj/_trace.o
if [ -n "$DRAFT_TRACE" ]; then      # trace build: the objects must come from DRAFT_TRACE=1 tools/draft_symbols.sh
    clang -target i686-pc-windows-msvc -x c -std=gnu2x -fms-extensions -msse2 -c -O0 -Wno-everything -DILB_TRACE \
        -nostdinc -isystem "$(clang -print-resource-dir)/include" -I draft/include -I $build -I port \
        draft/glue/trace.c -o $build/obj/_trace.o || exit 1
fi

lld-link-14 /out:$build/ILB_draft.exe /map:$build/ILB_draft.map /machine:x86 /subsystem:windows /entry:mainCRTStartup /safeseh:no /largeaddressaware \
    /libpath:"$xwin/crt/lib/x86" /libpath:"$xwin/sdk/lib/um/x86" /libpath:"$xwin/sdk/lib/ucrt/x86" /libpath:$build/lib \
    @$build/lib/alternatenames.rsp \
    $build/obj/*.o SDL2.lib fmod.lib \
    msvcrt.lib vcruntime.lib ucrt.lib oldnames.lib kernel32.lib user32.lib gdi32.lib advapi32.lib shell32.lib d3d11.lib dxgi.lib \
    > $build/link.log 2>&1
status=$?
grep -c "undefined symbol" $build/link.log | sed 's/^/undefined symbols: /'
[ $status -eq 0 ] && ls -la $build/ILB_draft.exe
exit $status
