#!/bin/sh
# Run the draft exe headless (it waits inside SDL_Init under bare wine), then print memory of the data
# image at ORIGINAL addresses: tools/draft_peek.sh <hex address>[:words] ...
# Usage: ILB_PREFIX=<prefix> tools/draft_peek.sh 60C164:4 5D4278:12
cd "$(dirname "$0")/.."
proton="${PROTON:-$HOME/.steam/steam/steamapps/common/Proton 8.0/dist}"
export WINEPREFIX="${ILB_PREFIX:-$(pwd)/draft/build/wineprefix}"
base=$(grep -m1 ' _ILB_image ' draft/build/ILB_draft.map | awk '{print $3}')
"$proton/bin/wineserver" -k 2>/dev/null; sleep 2
ILB_PREFIX="$WINEPREFIX" tools/run_draft.sh 80 > draft/build/peek_run.txt 2>&1 &
sleep "${PEEK_WAIT:-25}"
export WINEDEBUG=-all LD_LIBRARY_PATH="$proton/lib64:$proton/lib"
pid=$(timeout 20 "$proton/bin/wine" winedbg --command "info proc" 2>&1 | grep -i "ILB_draft" | awk '{print $1}' | head -1)
[ -z "$pid" ] && { echo "the program is not running"; cat draft/build/peek_run.txt; exit 1; }
{
    printf 'attach 0x%s\n' "$pid"
    for spec in "$@"; do
        address=${spec%%:*}; words=4; [ "$spec" != "$address" ] && words=${spec##*:}
        printf 'x/%sx 0x%x\n' "$words" $(( 0x$base + 0x$address - 0x400000 ))
    done
    printf 'detach\nquit\n'
} | timeout 60 "$proton/bin/wine" winedbg 2>&1 | python3 -c "
import re, sys
base = int('$base', 16)
for line in sys.stdin:
    m = re.search(r'0x([0-9a-f]+) ilb_draft\+0x[0-9a-f]+:\s+(.*)', line)
    if m:
        print('%08X: %s' % (int(m.group(1), 16) - base + 0x400000, m.group(2).strip()))
print('(image base in this build: %08X; a pointer into the image P corresponds to original P - %08X)' % (base, base - 0x400000))
"
"$proton/bin/wineserver" -k 2>/dev/null
