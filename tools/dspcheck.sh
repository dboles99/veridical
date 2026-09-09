#!/bin/sh
# Needs libRack.dll on the PATH, which means a Rack installation rather than
# just the SDK. RACK_APP is where Rack.exe lives.
set -e
RACK_DIR=${RACK_DIR:-../Rack-SDK}
RACK_APP=${RACK_APP:-"/c/Program Files/VCV/Rack2Pro"}
cd "$(dirname "$0")/.."
g++ -std=c++11 -O2 -g -march=nehalem -D_USE_MATH_DEFINES \
	-Wall -Wextra -Wno-unused-parameter \
	-I"$RACK_DIR/include" -I"$RACK_DIR/dep/include" \
	-o tools/dspcheck.exe tools/dspcheck.cpp \
	-L"$RACK_DIR" -lRack
PATH="$RACK_APP:$PATH" ./tools/dspcheck.exe
