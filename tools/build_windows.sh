#!/bin/sh
# Build the Windows program (32-bit, SDL2 window + OpenGL, the same code as the Linux "play" build) on Linux:
# clang compiles for Windows, lld-link links, the Windows libraries come from xwin.
#   build/windows/ILB_windows.exe     put it next to ILB.gbp and SDL2.dll of your copy of the game
# Usage: tools/build_windows.sh [--cheats]     --cheats: with the testing shortcuts of port/cheats.c (god mode
#                                              and the like); without it they are left out
# Needs: clang, lld-link, llvm-dlltool (14 or newer) and the Windows SDK/CRT libraries fetched with xwin
# (https://github.com/Jake-Shadle/xwin: "xwin --arch x86 splat --output ~/xwin"). XWIN=<dir> if elsewhere.
# The game code must exist (draft/build/src).
cd "$(dirname "$0")/.."
cheats=
for arg in "$@"; do
    case "$arg" in
        --cheats) cheats=-DILB_CHEATS;;
        *) echo "usage: tools/build_windows.sh [--cheats]"; exit 1;;
    esac
done
xwin="${XWIN:-$HOME/xwin}"
out=build/windows
obj=$out/obj
tool() { command -v "$1-14" > /dev/null && echo "$1-14" || echo "$1"; }
LLD=$(tool lld-link); DLLTOOL=$(tool llvm-dlltool)
[ -d "$xwin/crt/lib/x86" ] || { echo "no Windows libraries in $xwin (see the top of this script)"; exit 1; }
[ -d draft/build/src ] || { echo "draft/build/src is missing"; exit 1; }
mkdir -p $obj $out/lib
# the import stubs are regenerated only where the IDA exports are present; otherwise the existing copy is used
if [ -f dump/imports.json ]; then python3 tools/native_stubs.py > /dev/null || exit 1; fi

res="$(clang -print-resource-dir)/include"
DEFS="-DILB_WINDOW -DILB_NET_INPUT -DILB_MOD_PER_STEP -DILB_MOD_SEASON_OUTFITS -DILB_MOD_SEASON_EVENTS -DILB_MOD_SEASON_BUILDINGS -DILB_MOD_LOCAL_COOP
      -DILB_FIX_STUCK_PLAYER -DILB_MOD_COOP_SPAWN -DILB_MOD_NET_MENU -DILB_MOD_START_FLOW -DILB_MOD_SOFT_COLLECT $cheats $ILB_WINDOWS_DEFS"
# the version the lobby matches copies by, made as CMakeLists.txt makes it (ILB_GAME_VERSION): git's name for
# this commit, then every module switched on, in CMake's order - so this program meets the Linux build of the
# same source and modules in the lobby
version=$(git describe --tags --match "v[0-9]*" --always --dirty 2>/dev/null || echo unknown)
for module in ILB_MOD_SEASON_OUTFITS ILB_MOD_SEASON_EVENTS ILB_MOD_SEASON_BUILDINGS ILB_MOD_LOCAL_COOP ILB_FIX_STUCK_PLAYER \
              ILB_MOD_COOP_SPAWN ILB_MOD_NET_MENU ILB_MOD_START_FLOW ILB_MOD_SOFT_COLLECT ILB_CHEATS; do
    case " $(echo $DEFS) " in *" -D$module "*) version="$version+$module";; esac
done
echo "game version for the lobby: $version"
# objects made with other options are not reused
[ "$(cat $obj/defs.txt 2>/dev/null)" = "$(echo $DEFS)" ] || { rm -f $obj/*.o; echo $DEFS > $obj/defs.txt; }
# the game code and the layer under it: no system headers, they declare everything themselves
export GAME_CC="clang -target i686-pc-windows-msvc -x c -std=gnu2x -fms-extensions -msse2 -mfpmath=sse -c -g0 -O1
    -fno-strict-aliasing -fwrapv -fno-delete-null-pointer-checks -ftrivial-auto-var-init=zero
    -enable-trivial-auto-var-init-zero-knowing-it-will-be-removed-from-clang -Wno-everything
    -nostdinc -isystem $res -I draft/include -I draft/build -I port $(echo $DEFS)"
# the pieces written against the normal C headers (network link, Ogg Vorbis decoder)
# (-D_DLL: C library functions are taken from the C runtime DLL by import, never from a same-named game function)
SYS_CC="clang -target i686-pc-windows-msvc -x c -c -O1 -w -D_CRT_SECURE_NO_WARNINGS -D_DLL -D_MT -isystem $res
    -isystem $xwin/crt/include -isystem $xwin/sdk/include/ucrt -isystem $xwin/sdk/include/um -isystem $xwin/sdk/include/shared"
export OBJ=$obj

# compile what changed (a file is rebuilt when it is newer than its object; after changing a header, rm -r build/windows/obj)
fail=$out/failed.txt; rm -f $fail
ls draft/build/src/*.c draft/build/native/generated_stubs.c draft/native/*.c port/*.c | xargs -P "$(nproc)" -I{} sh -c '
    f="{}"; o="$OBJ/$(basename "${f%.c}").o"
    [ "$o" -nt "$f" ] && exit 0
    case "$f" in draft/build/src/*) extra="-include port/rand_trace.h";; *) extra="";; esac
    $GAME_CC $extra "$f" -o "$o" 2> "${o%.o}.err" || { echo "FAILED $f"; head -5 "${o%.o}.err"; echo "$f" >> '"$fail"'; }'
for f in port/net/*.c; do
    o="$obj/net_$(basename "${f%.c}").o"
    # (net_lobby.o again whenever the version changes: it is in the lobby's hello)
    [ "$o" -nt "$f" ] && [ "$(cat $obj/version.txt 2>/dev/null)" = "$version" ] || $SYS_CC -DILB_GAME_VERSION="\"$version\"" "$f" -o "$o" || echo "$f" >> $fail
done
echo "$version" > $obj/version.txt
o=$obj/stb_vorbis.o
[ $o -nt third_party/stb_vorbis.c ] || $SYS_CC -O2 -DSTB_VORBIS_NO_STDIO -DSTB_VORBIS_NO_PUSHDATA_API third_party/stb_vorbis.c -o $o || echo stb_vorbis >> $fail
o=$obj/windows_shim.o
[ $o -nt port/windows/windows_shim.c ] || $SYS_CC port/windows/windows_shim.c -o $o || echo windows_shim >> $fail
[ -f $fail ] && { echo "compile errors, see above"; exit 1; }
find $obj -name '*.err' -empty -delete

# The native layer stands in front of some functions (the same list as --wrap in CMakeLists.txt): every call
# of X goes to __wrap_X, and __real_X is the original. lld-link has no dependable option for that, so the
# objects' symbols are renamed (harmless to repeat) and the real SDL functions are reached through the DLL
# import directly. assert_failed is the game's own function, a register-call one.
WRAP="SDL_RWFromFile SDL_PollEvent SDL_CreateWindow SDL_GetWindowSurface SDL_FillRect SDL_UpdateWindowSurface
      SDL_GetMouseState SDL_GetKeyboardState SDL_SetWindowPosition SDL_NumJoysticks SDL_IsGameController SDL_GameControllerOpen
      SDL_GameControllerGetJoystick SDL_JoystickInstanceID SDL_GameControllerGetVendor SDL_GameControllerName
      SDL_GameControllerGetAxis SDL_GameControllerClose"
OBJCOPY=$(tool llvm-objcopy)
rm -f $obj/real_thunks.o
renames=$(for w in $WRAP; do printf -- '--redefine-sym _%s=___wrap_%s ' $w $w; done)
for o in $obj/*.o; do
    case $o in
        */ec_assert.o) $OBJCOPY $renames --redefine-sym @assert_failed@20=@__real_assert_failed@20 $o;;
        *)             $OBJCOPY $renames --redefine-sym @assert_failed@20=@__wrap_assert_failed@20 $o;;
    esac
done
for w in $WRAP; do printf '__asm__( ".globl ___real_%s\\n___real_%s:\\n jmp *__imp__%s\\n" );\n' $w $w $w; done > $out/real_thunks.c
clang -target i686-pc-windows-msvc -c $out/real_thunks.c -o $obj/real_thunks.o || exit 1

# import library for SDL2.dll: every SDL function some object calls
NM=$(tool llvm-nm)
{ echo "LIBRARY SDL2.dll"; echo "EXPORTS"; { $NM --undefined-only $obj/*.o 2>/dev/null | sed -n 's/^ *U _\(SDL_[A-Za-z0-9_]*\)$/\1/p'; for w in $WRAP; do echo $w; done; } | sort -u; } > $out/lib/SDL2.def
$DLLTOOL -m i386 -k -d $out/lib/SDL2.def -l $out/lib/SDL2.lib -D SDL2.dll || exit 1

$LLD /out:$out/ILB_windows.exe /map:$out/ILB_windows.map /machine:x86 /subsystem:windows /entry:mainCRTStartup /safeseh:no /largeaddressaware /stack:8388608 \
    /libpath:"$xwin/crt/lib/x86" /libpath:"$xwin/sdk/lib/um/x86" /libpath:"$xwin/sdk/lib/ucrt/x86" /libpath:$out/lib \
    $obj/*.o SDL2.lib \
    libcmt.lib libvcruntime.lib ucrt.lib oldnames.lib kernel32.lib user32.lib ws2_32.lib \
    /nodefaultlib:msvcrt.lib /nodefaultlib:vcruntime.lib /nodefaultlib:libucrt.lib \
    > $out/link.log 2>&1
status=$?
grep -c "undefined symbol" $out/link.log | sed 's/^/undefined symbols: /'
[ $status -eq 0 ] && ls -la $out/ILB_windows.exe
exit $status
