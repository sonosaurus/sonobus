#!/bin/bash

DEFS=""

if [ -n "${AAX_SDK_PATH}" ] ; then
  DEPS="$DEPS -DAAX_SDK_PATH=${AAX_SDK_PATH}"
fi

if [ -n "${VST2_SDK_PATH}" ] ; then
  DEPS="$DEPS -DVST2_SDK_PATH=${VST2_SDK_PATH}"
fi


cmake -G "Visual Studio 15 2017" $DEPS -B build32
cmake -G "Visual Studio 15 2017 Win64" $DEPS -B build




