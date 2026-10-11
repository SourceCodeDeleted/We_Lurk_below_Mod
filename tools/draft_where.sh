#!/bin/sh
# Start draft/build/ILB_draft.exe (tools/run_draft.sh), attach wine's debugger after a while and print which
# game functions are on the main thread's stack. For finding where a run without a crash is waiting.
# Usage: ILB_PREFIX=<prefix> tools/draft_where.sh [seconds before attaching]
cd "$(dirname "$0")/.."
wait_s="${1:-25}"
proton="${PROTON:-$HOME/.steam/steam/steamapps/common/Proton 8.0/dist}"
export WINEPREFIX="${ILB_PREFIX:-$(pwd)/draft/build/wineprefix}"
out=draft/build/where
mkdir -p $out
"$proton/bin/wineserver" -k 2>/dev/null; sleep 2
ILB_PREFIX="$WINEPREFIX" tools/run_draft.sh $((wait_s + 60)) > $out/run_out.txt 2>&1 &
sleep "$wait_s"
export WINEDEBUG=-all LD_LIBRARY_PATH="$proton/lib64:$proton/lib"
pid=$(timeout 20 "$proton/bin/wine" winedbg --command "info proc" 2>&1 | grep -i "ILB_draft" | awk '{print $1}' | head -1)
[ -z "$pid" ] && { echo "the program is not running any more"; cat $out/run_out.txt; exit 1; }
printf 'attach 0x%s\nbt all\n' "$pid" | timeout 30 "$proton/bin/wine" winedbg > $out/bt.txt 2>&1
frame=$(grep -m1 '=>0' $out/bt.txt | sed 's/.*(0x0*\([0-9a-f]*\)).*/\1/')
grep -m1 -A4 'Backtracing for thread' $out/bt.txt | cut -c1-120
printf 'attach 0x%s\nx/900x 0x%s\ndetach\nquit\n' "$pid" "$frame" | timeout 40 "$proton/bin/wine" winedbg > $out/stack.txt 2>&1
"$proton/bin/wineserver" -k 2>/dev/null
python3 tools/draft_backtrace.py --stack $out/stack.txt draft/build/ILB_draft.map
