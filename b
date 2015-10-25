#!/bin/bash

SCRIPT_PATH=$(cd $(dirname $0) ; pwd -P)
BUILD_PATH=$SCRIPT_PATH/build

if command -v ninja >/dev/null 2>&1; then
    BUILD_GENERATOR=Ninja
    BUILD_COMMAND=ninja
else
    BUILD_GENERATOR="Unix Makefiles"
    BUILD_COMMAND=make
fi

[ ! -d $BUILD_PATH ] && mkdir -p $BUILD_PATH
[ ! -d $BUILD_PATH/CMakeFiles ] && (cd $BUILD_PATH; cmake -G "$BUILD_GENERATOR" $SCRIPT_PATH)
(cd $BUILD_PATH; $BUILD_COMMAND "$@")

