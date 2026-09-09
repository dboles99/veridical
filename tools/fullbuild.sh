#!/bin/sh
# Clean build from scratch, then package, then run both checks.
set -e
cd "$(dirname "$0")/.."
export RACK_DIR=${RACK_DIR:-/d/Music/dev-vcv/Rack-SDK}
make clean >/dev/null 2>&1 || true
echo "=== build"
make -j4 2>&1 | tee /tmp/veridical-build.log
echo "=== warnings and errors in that build"
grep -Ei 'warning|error' /tmp/veridical-build.log || echo "none"
echo "=== dist"
make dist 2>&1 | tail -6
ls -l dist/*.vcvplugin
echo "=== panels"
sh tools/panelcheck.sh
echo "=== dsp"
sh tools/dspcheck.sh
