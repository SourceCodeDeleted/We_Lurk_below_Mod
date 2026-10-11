#!/bin/sh
# Run build/windows/ILB_windows.exe on Linux under Proton 8.0's wine, to test the Windows build.
# The game's data is linked into build/windows/run; saves are in that folder (native_appdata), and wine uses a
# private prefix (a copy of Proton's default one), so neither the game folder nor any Steam prefix is touched.
# Usage: tools/run_windows.sh [seconds]        log: build/windows/run/ILB_windows.log
#   PROTON=<Proton's dist folder>  ILB_GAME=<game folder>  ILB_RUN=<run folder>  plus the game's own ILB_* variables
cd "$(dirname "$0")/.."
root=$(pwd)
seconds="${1:-40}"
proton="${PROTON:-$HOME/.steam/steam/steamapps/common/Proton 8.0/dist}"
game="${ILB_GAME:-$PWD/game}"
export WINEPREFIX="${ILB_PREFIX:-$root/build/windows/wineprefix}"
run="${ILB_RUN:-build/windows/run}"
mkdir -p "$run"
ln -sf "$game/ILB.gbp" "$run/ILB.gbp"
cp "$game/SDL2.dll" build/windows/ILB_windows.exe "$run/"

if [ ! -d "$WINEPREFIX" ]; then
    cp -a "$proton/share/default_pfx" "$WINEPREFIX"
    # the default prefix links to Proton's files with relative paths; make them absolute
    python3 - "$proton/share/default_pfx" "$WINEPREFIX" <<'PY'
import os, sys
src, dst = sys.argv[1:]
for root, dirs, files in os.walk(dst):
    for name in files + dirs:
        p = os.path.join(root, name)
        if os.path.islink(p) and not os.path.exists(p):
            target = os.path.realpath(os.path.join(src, os.path.relpath(p, dst)))
            if os.path.exists(target):
                os.remove(p)
                os.symlink(target, p)
PY
fi
[ -e "$WINEPREFIX/dosdevices/c:" ] || ln -s ../drive_c "$WINEPREFIX/dosdevices/c:"
[ -e "$WINEPREFIX/dosdevices/z:" ] || ln -s / "$WINEPREFIX/dosdevices/z:"

export WINEDEBUG="${WINEDEBUG:-fixme-all,err-winediag}" WINEDLLOVERRIDES="winemenubuilder.exe=d"
export LD_LIBRARY_PATH="$proton/lib64:$proton/lib:$LD_LIBRARY_PATH"
cd "$run"
timeout "$seconds" "$proton/bin/wine" ILB_windows.exe > wine.log 2>&1
status=$?
"$proton/bin/wineserver" -k 2>/dev/null
if [ $status -eq 124 ]; then echo "still running after $seconds s (stopped by the timeout)"; else echo "exited with status $status"; fi
tail -5 wine.log
