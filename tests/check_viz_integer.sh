#!/bin/sh
# The ESP8266 has no FPU: fail if the visualization engine calls any soft-float
# routine (__addsf3, __mulsf3, __divdf3, ...). Needs the PlatformIO xtensa toolchain.
set -e
cd "$(dirname "$0")/.."
TC="${XTENSA_BIN:-$HOME/.platformio/packages/toolchain-xtensa/bin}"
OUT="$(mktemp -d)"
trap 'rm -rf "$OUT"' EXIT
"$TC/xtensa-lx106-elf-g++" -std=gnu++17 -O3 -ffast-math -mlongcalls -fno-exceptions -fno-rtti \
    -Wall -Wextra -Werror -c tests/viz_xtensa.cpp -o "$OUT/viz.o"
FLOAT="$("$TC/xtensa-lx106-elf-nm" -u "$OUT/viz.o" | grep -E '__[a-z0-9]*(sf|df)' || true)"
if [ -n "$FLOAT" ]; then
    echo "FAIL viz_engine.h uses soft-float:"; echo "$FLOAT"; exit 1
fi
echo "PASS viz_engine.h has no soft-float calls ($("$TC/xtensa-lx106-elf-size" "$OUT/viz.o" | awk 'NR==2{print $1" B code, "$3" B RAM for 16x16"}'))"
