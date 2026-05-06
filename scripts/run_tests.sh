#!/bin/bash
set -euo pipefail

CLEAN=0
RUNNER_ARGS=()

for arg in "$@"; do
  case "$arg" in
    -c|--clean) CLEAN=1 ;;
    *) RUNNER_ARGS+=("$arg") ;;
  esac
done

if [ $CLEAN -eq 1 ]; then
  make clean
fi

BUILD_START=$(date +%s)
make test || exit 1
BUILD_SECS=$(( $(date +%s) - BUILD_START ))

make check-luals-stubs
clear
scripts/check_luals_fixtures.sh
./test_runner --build-secs=$BUILD_SECS ${RUNNER_ARGS[@]+"${RUNNER_ARGS[@]}"}
