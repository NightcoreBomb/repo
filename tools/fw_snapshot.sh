#!/usr/bin/env bash
# fw_snapshot.sh — збирає всі варіанти прошивки ЛР4 і кладе .hex у docs/metrics/fw/ (їх і вимірює run_lab4.sh).
# lab3.hex — з тегу lab3 (git worktree), тож порівнюється саме здана версія ЛР3.
set -euo pipefail
cd "$(dirname "$0")/.."
OUT=docs/metrics/fw; mkdir -p "$OUT"
make -C firmware -s                                                     && cp firmware/build/firmware.{hex,elf} "$OUT/" && mv "$OUT/firmware.hex" "$OUT/lab4.hex" && mv "$OUT/firmware.elf" "$OUT/lab4.elf"
make -C firmware -s OSTEST=1 BUILD_DIR=build-ostest                     && cp firmware/build-ostest/firmware.hex    "$OUT/lab4-ostest.hex"
make -C firmware -s TICK_HZ=100 BUILD_DIR=build-tick100                 && cp firmware/build-tick100/firmware.hex   "$OUT/lab4-tick100.hex"
make -C firmware -s STACK_CHECK=0 OSTEST=1 BUILD_DIR=build-nocheck      && cp firmware/build-nocheck/firmware.hex   "$OUT/lab4-nocheck.hex"
make -C firmware -s HEAP_BYTES=3000 BUILD_DIR=build-heapsmall           && cp firmware/build-heapsmall/firmware.hex "$OUT/lab4-heapsmall.hex"
W=$(mktemp -d); git worktree add -q "$W" lab3
make -C "$W/firmware" -s && cp "$W/firmware/build/firmware.hex" "$OUT/lab3.hex" && cp "$W/firmware/build/firmware.elf" "$OUT/lab3.elf"
git worktree remove --force "$W"
arm-none-eabi-size "$OUT"/lab3.elf "$OUT"/lab4.elf
