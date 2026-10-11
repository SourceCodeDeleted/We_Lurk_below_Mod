#!/bin/sh
# Start the playable Linux build (SDL2 window, OpenGL). Builds it first if needed.
# The game's pack file is linked into a run folder; saves are not kept yet (the per-user folder is a stand-in).
cd "$(dirname "$0")/.."
root=$(pwd)
[ -x build/native/ILB_native ] || tools/build_native.sh native || exit 1
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
# everything the game prints also goes to draft/build/native/game.log, so a crash can be read afterwards
ASAN_OPTIONS=detect_leaks=0 "$root/build/native/ILB_native" "$@" 2>&1 | tee ../game.log
# after a crash: the functions the game was in, by name
grep -q "native: TRACE" ../game.log && python3 "$root/tools/native_trace.py" | tee -a ../game.log
