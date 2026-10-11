#!/bin/sh
# Run the native debug build under gdb and print a backtrace where it stops (crash, abort or the timeout).
# Usage: tools/gdb_native.sh [frames] [extra gdb -ex commands...]
cd "$(dirname "$0")/.."
root=$(pwd)
frames="${1:-300}"; [ $# -gt 0 ] && shift
run=draft/build/native/run
mkdir -p $run
ln -sf "${ILB_GAME:-$PWD/game}/ILB.gbp" $run/ILB.gbp
cd $run
ILB_FRAMES="$frames" ASAN_OPTIONS="detect_leaks=0:handle_segv=0:handle_abort=0:abort_on_error=1" \
    timeout -s INT "${NATIVE_TIMEOUT:-120}" gdb -batch -ex "handle SIGINT stop" -ex run "$@" -ex bt "$root/draft/build/native/ILB_native" 2>&1 \
    | grep -v "^\[\|^warning\|Download\|libthread_db" | tail -${GDB_TAIL:-30} | cut -c1-240
