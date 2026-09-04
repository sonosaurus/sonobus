#!/bin/bash

DEFS=""
CONFIG="Release"

if [ "$1" = "debug" ] ; then
  CONFIG="Debug"
fi

cmake -DCMAKE_BUILD_TYPE=$CONFIG $DEPS -B build




