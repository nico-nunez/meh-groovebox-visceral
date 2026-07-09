#!/bin/bash
set -euo pipefail

CLEAN=0
RUNNER_ARGS=()
INCREMENTAL_BUILD=false

for arg in "$@"; do
  case "$arg" in
    -i|--incremental) INCREMENTAL_BUILD=true ;;
    *) RUNNER_ARGS+=("$arg") ;;
  esac
done

if [ "$INCREMENTAL_BUILD" = false ]; then
  make clean
  ## rm -rf .zig-cache/
fi

BUILD_START=$(date +%s)
make -j10 test || exit 1
## zig build test || exit 1
BUILD_SECS=$(( $(date +%s) - BUILD_START ))

make -j10 check-luals-stubs
## zig build check-luals-stubs
clear
scripts/check_luals_fixtures.sh
./test_runner --build-secs=$BUILD_SECS ${RUNNER_ARGS[@]+"${RUNNER_ARGS[@]}"}
