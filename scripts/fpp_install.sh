#!/bin/bash
set -e

# Build against the headers and libfpp belonging to the FPP 10 installation
# on this device. This is important because FPP 10 checks native-plugin ABI.
BASEDIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$BASEDIR"
make clean
make

# No restartFlag is required. The plugin advertises c++ via callbacks.sh and
# declares FPP_PLUGIN_SUPPORTS_UNLOAD(), so FPP 10 can load/reload it at runtime.
