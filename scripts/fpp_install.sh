#!/bin/bash
set -e

BASEDIR="$(cd "$(dirname "$0")/.." && pwd)"
FPPDIR="${FPPDIR:-/opt/fpp}"
SRCDIR="${FPPDIR}/src"

make -C "$BASEDIR" clean
make -C "$BASEDIR" "SRCDIR=${SRCDIR}"
