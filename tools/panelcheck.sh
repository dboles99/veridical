#!/bin/sh
set -e
RACK_DIR=${RACK_DIR:-../Rack-SDK}
cd "$(dirname "$0")/.."
gcc -O2 -Wall -I"$RACK_DIR/dep/include" -o tools/panelcheck.exe tools/panelcheck.c -lm
echo "--- text elements, of which there should be none"
grep -l "<text\|<flowRoot\|<tspan" res/*.svg || echo "none"
echo "--- nanosvg, at Rack's 75 DPI"
./tools/panelcheck.exe res/*-dark.svg res/*-light.svg
