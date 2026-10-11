#!/bin/sh
# Run draft/build/ILB_draft.exe under Proton 8.0's wine for a limited time and summarise what happened.
# The game's data and DLLs are linked (not copied) into draft/build/run; saves go to a private wine prefix
# (a copy of Proton's default one), so neither the game folder nor any Steam prefix is touched.
# Usage: tools/run_draft.sh [seconds]        log: draft/build/run.log
cd "$(dirname "$0")/.."
root=$(pwd)
seconds="${1:-40}"
proton="${PROTON:-$HOME/.steam/steam/steamapps/common/Proton 8.0/dist}"
game="${ILB_GAME:-$PWD/game}"
export WINEPREFIX="${ILB_PREFIX:-$root/draft/build/wineprefix}"
run=draft/build/run
mkdir -p $run
for f in ILB.gbp SDL2.dll fmod.dll; do ln -sf "$game/$f" $run/$f; done
for f in d3d11 dxgi; do ln -sf "$proton/lib/wine/dxvk/$f.dll" $run/$f.dll; done     # Direct3D 11 through DXVK, as Proton does
cp draft/build/ILB_draft.exe $run/

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

export WINEDEBUG="${WINEDEBUG:-fixme-all,err-winediag}" WINEDLLOVERRIDES="winemenubuilder.exe=d;d3d11,dxgi=n"
export LD_LIBRARY_PATH="$proton/lib64:$proton/lib:$LD_LIBRARY_PATH"
cd $run
timeout "$seconds" "$proton/bin/wine" ILB_draft.exe > ../run.log 2>&1
status=$?
"$proton/bin/wineserver" -k 2>/dev/null
cd "$root"
if [ $status -eq 124 ]; then echo "still running after $seconds s (stopped by the timeout)"; else echo "exited with status $status"; fi
python3 tools/draft_backtrace.py draft/build/run.log draft/build/ILB_draft.map
