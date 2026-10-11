#!/bin/sh
# Run the native debug build in a scratch folder next to the game's pack file.
# Usage: tools/run_native.sh [frames]        log: draft/build/native/run.log
cd "$(dirname "$0")/.."
root=$(pwd)
game="${ILB_GAME:-$PWD/game}"
run=draft/build/native/run
mkdir -p $run
ln -sf "$game/ILB.gbp" $run/ILB.gbp
cd $run
ILB_FRAMES="${1:-300}" ASAN_OPTIONS="detect_leaks=0:abort_on_error=0:halt_on_error=1:detect_stack_use_after_return=0" \
    UBSAN_OPTIONS="print_stacktrace=1" timeout "${NATIVE_TIMEOUT:-120}" "$root/draft/build/native/ILB_native" > ../run.log 2>&1
status=$?
cd "$root"
echo "exit status $status (124 = stopped by the timeout)"
grep -v "^$" draft/build/native/run.log | head -60
