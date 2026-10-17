#!/bin/sh
# Check that this machine's build computes exactly what another platform's build computed: replay a session
# recorded there and compare the game state tick by tick. This is what lets Linux and Windows copies play one
# network game together.
#   tools/check_determinism.sh [bundle]       default: tests/determinism/windows_session.tgz
# A bundle holds input.rec (every input event of the session), sum_windows.txt (the recording machine's
# checksum of every tick) and appdata_before/ (the saves the session started from). The session loads the
# character in slot 1 by script (ILB_SCRIPT=load) and runs 2400 ticks; a window opens for about a minute and
# plays by itself. Needs the play build (tools/build_native.sh play) and the game's ILB.gbp (game/ or ILB_GAME).
cd "$(dirname "$0")/.."
root=$(pwd)
bundle="${1:-tests/determinism/windows_session.tgz}"
[ -f "$bundle" ] || { echo "no bundle at $bundle"; exit 1; }
[ -x build/play/ILB_play ] || tools/build_native.sh play || exit 1
gbp="${ILB_GAME:-$root/game}/ILB.gbp"
[ -f "$gbp" ] || { echo "no ILB.gbp at $gbp (set ILB_GAME to the folder of your copy of the game)"; exit 1; }

work=$(mktemp -d)
tar xzf "$bundle" -C "$work" || exit 1
mkdir "$work/run"
cp -a "$work/appdata_before" "$work/run/native_appdata"
ln -s "$gbp" "$work/run/ILB.gbp"
cd "$work/run"
ILB_SCRIPT=load ILB_SLOT=1 ILB_FRAMES=2400 ILB_LOBBY=127.0.0.1:1 ILB_REPLAY="$work/input.rec" ILB_CHECKSUM="$work/sum_here.txt" \
    "$root/build/play/ILB_play" > "$work/replay.log" 2>&1
grep "native input" "$work/replay.log" | head -3
python3 "$root/tools/compare_checksums.py" "$work/sum_windows.txt" "$work/sum_here.txt"
status=$?
echo "(files kept in $work)"
exit $status
