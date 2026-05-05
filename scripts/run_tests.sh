#!/bin/bash
set -euo pipefail

make clean

BUILD_START=$(date +%s)
make test || exit 1
BUILD_SECS=$(( $(date +%s) - BUILD_START ))

make check-luals-stubs
clear
scripts/check_luals_fixtures.sh
./test_runner --build-secs=$BUILD_SECS "$@"
