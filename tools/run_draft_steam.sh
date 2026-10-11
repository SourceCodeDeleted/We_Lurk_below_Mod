#!/bin/sh
# Run an exe from draft/build/steam_run the way Steam does: Proton Experimental inside the Steam Linux
# runtime container, with a private compat folder (no Steam prefix is touched). Bare Proton wine cannot
# open the game window on this machine; this can.
# Usage: tools/run_draft_steam.sh [seconds] [exe]      log: draft/build/steam_run.log
cd "$(dirname "$0")/.."
root=$(pwd)
seconds="${1:-60}"
exe="${2:-ILB_draft.exe}"
proton="${PROTON:-$HOME/.steam/steam/steamapps/common/Proton - Experimental}"
runtime="${STEAM_RUNTIME_DIR:-$HOME/.steam/steam/steamapps/common/SteamLinuxRuntime_4}"
export STEAM_COMPAT_DATA_PATH="${ILB_COMPAT:-$root/draft/build/compat}"
export STEAM_COMPAT_CLIENT_INSTALL_PATH="$HOME/.steam/steam"
export STEAM_COMPAT_INSTALL_PATH="$root/draft/build/steam_run"
export SteamAppId=0 SteamGameId=0
mkdir -p "$STEAM_COMPAT_DATA_PATH"
[ -f draft/build/ILB_draft.exe ] && cp draft/build/ILB_draft.exe draft/build/steam_run/
cd draft/build/steam_run
timeout "$seconds" "$runtime/run" -- "$proton/proton" run "$exe" > ../steam_run.log 2>&1
echo "exit status $? (124 = still running when the time was up)"
