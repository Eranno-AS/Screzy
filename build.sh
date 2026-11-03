#!/usr/bin/env bash
# Build Screzy.exe with the MinGW-w64 cross-compiler (Linux, macOS, WSL).
#   Debian/Ubuntu:  sudo apt install mingw-w64
#   macOS:          brew install mingw-w64
set -euo pipefail
cd "$(dirname "$0")"

CC="${CC:-x86_64-w64-mingw32-gcc}"
WINDRES="${WINDRES:-x86_64-w64-mingw32-windres}"
LIBS="-lwinmm -ldwmapi -lcomctl32 -lshlwapi -luser32 -lshell32 -ladvapi32 -lgdi32"

mkdir -p build
"$WINDRES" -I res res/screzy.rc -O coff -o build/res.o
"$CC" -O2 -Wall -mwindows -static -o build/Screzy.exe src/screzy.c build/res.o $LIBS
echo "Built build/Screzy.exe"
