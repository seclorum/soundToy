#!/bin/bash
# STOY.tap debug harness - same program `make test` runs, instrumented.
# Usage: ./debug/run_trace.sh [seconds] [bpgen args...]
#   e.g. ./debug/run_trace.sh 40 --callsites blob_restore blob_draw call
#        ./debug/run_trace.sh 40 --watch-lo \$7b04 --watch-len 16
set -u
cd "$(dirname "$0")/.."
SECS=${1:-45}; shift || true
mkdir -p debug
make build/build/STOY.tap >debug/build.log 2>&1 || { echo BUILD FAILED; tail debug/build.log; exit 1; }
python3 debug/bpgen.py -o debug/bps.txt "$@" || exit 1
pkill -x oricutron 2>/dev/null; sleep 1
LOG=debug/trace.log; rm -f $LOG
# identical invocation to the Makefile `test` target + debug hooks
# (NOTE: oricutron requires ABSOLUTE paths - relative ones fail detection)
ROOT=$PWD
./oricutron.seclorum/oricutron "$ROOT/build/build/STOY.tap" \
    -s "$ROOT/build/build/symbols" -r ":$ROOT/debug/bps.txt" >$LOG 2>&1 &
EMU=$!
for i in $(seq $SECS); do sleep 1; kill -0 $EMU 2>/dev/null || break; done
if kill -0 $EMU 2>/dev/null; then echo "STATUS: ALIVE after ${SECS}s"; kill $EMU 2>/dev/null; else echo "STATUS: EXITED (rc check below)"; fi
wait $EMU 2>/dev/null; echo "emulator exit: $?"
python3 debug/analyze.py $LOG
