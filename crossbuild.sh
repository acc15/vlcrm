#!/bin/sh

set -euxo pipefail

build() {
    echo "building $1"
    tc=""
    if [[ -n ${3:-} ]]; then
        tc="--toolchain ./toolchains/$3.cmake"
    fi
    rm -rf build
    cmake -DCMAKE_BUILD_TYPE=Release $tc -B build
    cmake --build build
    mkdir -p dist/$1
    cp build/libvlcrm_plugin.$2 dist/$1
}

if [[ $(uname) == "Darwin" ]]; then
    build "macos-arm64" "dylib" "macos-arm64"
    build "macos-intel64" "dylib" "macos-intel64"
else
    build "linux" "so"
fi
build "win32" "dll" "mingw-win32"
build "win64" "dll" "mingw-win64"
