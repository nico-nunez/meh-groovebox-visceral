#!/bin/bash
set -euo pipefail

for arg in "$@"; do
  case "$arg" in
    -c|--clean) make clean ;;
  esac
done

make && clear && ./main
