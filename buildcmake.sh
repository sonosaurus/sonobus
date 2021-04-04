#!/bin/bash

OPTS="--verbose"
CONFIG="Release"

if [ "$1" -eq "debug" ]; then
  CONFIG="Debug"
fi
  
  
cmake --build build --config $CONFIG $OPTS

if [ -d build32 ] ; then
   cmake --build build32 --config $CONFIG $OPTS  
fi

