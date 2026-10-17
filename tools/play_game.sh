#!/bin/sh
# Start the fast build of the game (SDL2 window, OpenGL; optimised, no sanitizers). Brings it up to date first:
# an old program file carries an old version, and the lobby shows games only to copies of the same version.
# Use this one to play. When something crashes, repeat it in the debug build (tools/run_game.sh), which is
# slower but says where and why.
cd "$(dirname "$0")/.."
root=$(pwd)
if [ -f build/play/build.ninja ]; then
    cmake --build build/play 2>&1 | grep -E "error|FAILED|ninja: build stopped|Game version" | head -30
    [ -x build/play/ILB_play ] || exit 1
else
    tools/build_native.sh play || exit 1
fi
run=draft/build/native/run
mkdir -p $run
ln -sf "${ILB_GAME:-$PWD/game}/ILB.gbp" $run/ILB.gbp
# every start keeps a copy of the save folder (the last 15), in draft/build/native/save_backups/
if [ -d $run/native_appdata ]; then
    mkdir -p draft/build/native/save_backups
    cp -a $run/native_appdata "draft/build/native/save_backups/$(date +%Y-%m-%d_%H-%M-%S)"
    ls -1d draft/build/native/save_backups/*/ 2>/dev/null | head -n -15 | xargs -r rm -rf
fi
cd $run
"$root/build/play/ILB_play" "$@" 2>&1 | tee ../game.log
grep -q "native: TRACE" ../game.log && python3 "$root/tools/native_trace.py" | tee -a ../game.log
:
