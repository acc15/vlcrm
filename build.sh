#!/bin/sh

rm -rf build
cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_SYSTEM_NAME=Windows \
    -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
    -DCMAKE_C_FLAGS=-s \
    -DVLC_HEADERS=/home/acc15/dev/thirdparty/vlc/include \
    -DVLC_LIB=/home/acc15/Downloads/vlc-3.0.21 \
    -B build && cmake --build build
