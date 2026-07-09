#!/bin/bash
set -euo pipefail

INCREMENTAL_BUILD=false

for arg in "$@"; do
  case "$arg" in
    -i|--incremental) INCREMENTAL_BUILD=true ;;
  esac
done

if [ "$INCREMENTAL_BUILD" = false ]; then
  make clean
  ## rm -rf .zig-cache
fi

make -j10 && clear && ./main
