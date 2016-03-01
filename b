#!/bin/bash
set -e

SCRIPT_PATH=$(cd $(dirname $0); pwd -P)

(exec scripts/get_cmake.sh)
CMAKE="$SCRIPT_PATH/external/cmake/cmake"

BUILD_TYPE=DEBUG
if [ -n "$1" ]; then
    BUILD_TYPE=$1; shift
fi

BUILD_PATH=$SCRIPT_PATH/build/ninja/$BUILD_TYPE
BUILD_GENERATOR="Unix Makefiles"
if command -v ninja >/dev/null 2>&1; then
    BUILD_GENERATOR=Ninja
fi

[ ! -d $BUILD_PATH ] && mkdir -p $BUILD_PATH
[ ! -d $BUILD_PATH/CMakeFiles ] &&
    (cd $BUILD_PATH; "$CMAKE" -DCMAKE_BUILD_TYPE=$BUILD_TYPE -G "$BUILD_GENERATOR" $SCRIPT_PATH)
(cd $BUILD_PATH; "$CMAKE" --build . -- "$@")

