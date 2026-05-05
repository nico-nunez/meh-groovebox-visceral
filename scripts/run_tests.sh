#!/bin/bash
make clean

BUILD_START=$(date +%s)
make test || exit 1
BUILD_SECS=$(( $(date +%s) - BUILD_START ))

clear
./test_runner --build-secs=$BUILD_SECS "$@"
