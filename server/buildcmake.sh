#!/bin/bash

OPTS=""
CONFIG="Release"

if [ "$1" = "debug" ]; then
  CONFIG="Debug"
fi

JOBS=2
if  nproc &> /dev/null ; then
  JOBS=$(nproc)
elif sysctl -n hw.logicalcpu  &> /dev/null; then
  JOBS=$(sysctl -n hw.logicalcpu)
elif [ -n "$NUMBER_OF_PROCESSORS" ] ; then
  JOBS=$NUMBER_OF_PROCESSORS
fi

OPTS="-j ${JOBS}"
  
cmake --build build --config $CONFIG $OPTS


