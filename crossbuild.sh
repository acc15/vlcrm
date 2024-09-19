#!/bin/sh

set -eux

build() {
    echo "building $1"
    build_dir="build/$1"
    tc=""
    if [ -n "${3:-}" ]; then
        tc="--toolchain ./toolchains/$3.cmake"
    fi
    rm -rf "$build_dir"
    # shellcheck disable=SC2086
    cmake -DCMAKE_BUILD_TYPE=Release $tc -B "$build_dir"
    cmake --build "$build_dir"
    mkdir -p "dist/$1"
    cp "$build_dir/libvlcrm_plugin.$2" "dist/$1"
}

if [ "$(uname)" = Darwin ]; then
    build macos-arm64 dylib macos-arm64
    build macos-intel64 dylib macos-intel64
else
    build linux so
fi
build win32 dll mingw-win32
build win64 dll mingw-win64
