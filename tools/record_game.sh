#!/bin/sh
# Play the game while recording every input event, to test that the game can be replayed exactly (the basis
# for online play). Usage: tools/record_game.sh <name> [play|native]        (default: the fast build)
# Makes draft/build/native/recordings/<name>/ with a copy of the saves as they were before, the input
# (input.rec) and the game state checksum of every tick (sum_record.txt). Play normally, then quit the game.
# Afterwards: tools/replay_game.sh <name>
cd "$(dirname "$0")/.."
root=$(pwd)
name="${1:?usage: tools/record_game.sh <name> [play|native]}"
variant="${2:-play}"
[ -x build/$variant/ILB_$variant ] || tools/build_native.sh $variant || exit 1
run=$root/draft/build/native/run
rec=$root/draft/build/native/recordings/$name
[ -e "$rec" ] && { echo "$rec exists already; pick another name"; exit 1; }
mkdir -p "$rec" $run
ln -sf "${ILB_GAME:-$PWD/game}/ILB.gbp" $run/ILB.gbp
[ -d $run/native_appdata ] && cp -a $run/native_appdata "$rec/appdata_before"
cd $run
ILB_RECORD="$rec/input.rec" ILB_CHECKSUM="$rec/sum_record.txt" ASAN_OPTIONS=detect_leaks=0 "$root/build/$variant/ILB_$variant" 2>&1 | tee "$rec/record.log" | grep -v "^native: frame"
echo "recorded: $rec ($(wc -l < "$rec/sum_record.txt") ticks in play). Now: tools/replay_game.sh $name"
