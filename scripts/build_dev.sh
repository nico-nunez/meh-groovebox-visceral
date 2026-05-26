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
fi

make && clear && ./main
