#!/bin/bash

JOBS=2
if  nproc &> /dev/null ; then
  JOBS=$(nproc)
fi


# if on ARM we need -march=native
if uname -m | grep arm &> /dev/null ; then
  echo Building on ARM platform
  export CFLAGS="-march=native"
  JOBS=2
fi


CONFIG=Release make -j${JOBS} Standalone VST3
