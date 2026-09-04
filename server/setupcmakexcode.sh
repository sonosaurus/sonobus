#!/bin/bash

# Warning is default
DEFS="-DAOO_LOG_LEVEL=Verbose"
#DEFS="-DAOO_LOG_LEVEL=Warning"

TEAMOPT=""
if [ x"$APPLE_TEAMID" != x ] ; then
 TEAMOPT=-DCMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM=$APPLE_TEAMID
fi

# xcode
cmake -GXcode ${DEPS} ${DEFS} -B buildXcode ${TEAMOPT}



