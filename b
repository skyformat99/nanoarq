#!/bin/bash
set -e

SCRIPT_PATH=$(cd $(dirname $0) ; pwd -P)
BUILD_PATH=$SCRIPT_PATH/build

#### find or grab cmake

CMAKE=cmake

if ! command -v cmake >/dev/null 2>&1; then
    CMAKE_DIR="$SCRIPT_PATH/external/cmake"
    HOST_OS=$(uname -s)

    CMAKE_PREFIX=cmake-3.3.2-$HOST_OS-x86_64
    if [ "$HOST_OS" == "Darwin" ]; then
        CMAKE="$CMAKE_DIR/$CMAKE_PREFIX/CMake.app/Contents/bin/cmake"
    else
        CMAKE="$CMAKE_DIR/$CMAKE_PREFIX/bin/cmake"
    fi

    # Actually go wget the latest build
    if [ ! -f "$CMAKE" ]; then
        echo CMake not found in path or at $CMAKE, retrieving...
        mkdir -p "$CMAKE_DIR"
        CMAKE_ARCHIVE="$CMAKE_PREFIX.tar.gz"
        rm -f "$CMAKE_DIR/$CMAKE_ARCHIVE"
        wget -P "$CMAKE_DIR" https://cmake.org/files/v3.3/$CMAKE_ARCHIVE
        tar xzf "$CMAKE_DIR/$CMAKE_ARCHIVE" -C "$CMAKE_DIR"
        rm "$CMAKE_DIR/$CMAKE_ARCHIVE"
    fi
fi

#### prefer ninja, fall back to make

BUILD_GENERATOR="Unix Makefiles"
BUILD_COMMAND=make

if command -v ninja >/dev/null 2>&1; then
    BUILD_GENERATOR=Ninja
    BUILD_COMMAND=ninja
fi

#### everything builds inside of 'build' subdirectory

[ ! -d $BUILD_PATH ] && mkdir -p $BUILD_PATH
[ ! -d $BUILD_PATH/CMakeFiles ] && (cd $BUILD_PATH; "$CMAKE" -G "$BUILD_GENERATOR" $SCRIPT_PATH)
(cd $BUILD_PATH; $BUILD_COMMAND "$@")

