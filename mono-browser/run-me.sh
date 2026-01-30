#!/usr/bin/env bash
set -e

APP="mono-browser"
CXX="${CXX:-g++}"

sudo pacman -S --needed --noconfirm gtk3 webkit2gtk vte3 || true

CFLAGS="-O2 -Wall -Wextra -std=c++17"
PKG="$(pkg-config --cflags --libs gtk+-3.0 webkit2gtk-4.1 vte-2.91)"

mkdir -p build

echo "[build] compiling..."
$CXX $CFLAGS \
  src/main.cpp \
  src/browser.cpp \
  src/term.cpp \
  -o "build/$APP" \
  $PKG

echo "[build] ok -> build/$APP"
echo "[run] launching..."
./build/$APP
