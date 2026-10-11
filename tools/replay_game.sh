#!/bin/sh
# Replay a session recorded with tools/record_game.sh and compare the game state tick by tick with the
# recording. Usage: tools/replay_game.sh <name> [play|native]
# The replay runs in its own folder with a copy of the saves from before the recording; the real saves are
# not touched. A window opens and the game plays itself; do not expect to control it (closing it works).
cd "$(dirname "$0")/.."
root=$(pwd)
name="${1:?usage: tools/replay_game.sh <name> [play|native]}"
variant="${2:-play}"
[ -x build/$variant/ILB_$variant ] || tools/build_native.sh $variant || exit 1
rec=$root/draft/build/native/recordings/$name
[ -f "$rec/input.rec" ] || { echo "no recording at $rec"; exit 1; }
run="$rec/replay_run"
rm -rf "$run"
mkdir -p "$run"
ln -sf "${ILB_GAME:-$PWD/game}/ILB.gbp" "$run/ILB.gbp"
[ -d $root/draft/build/native/run/mods ] && ln -sf $root/draft/build/native/run/mods "$run/mods"
[ -d "$rec/appdata_before" ] && cp -a "$rec/appdata_before" "$run/native_appdata"
cd "$run"
ILB_REPLAY="$rec/input.rec" ILB_CHECKSUM="$rec/sum_replay_$variant.txt" ASAN_OPTIONS=detect_leaks=0 "$root/build/$variant/ILB_$variant" > "$rec/replay_$variant.log" 2>&1
grep "native input\|TRACE\|ERROR" "$rec/replay_$variant.log" | head -5
python3 "$root/tools/compare_checksums.py" "$rec/sum_record.txt" "$rec/sum_replay_$variant.txt"
