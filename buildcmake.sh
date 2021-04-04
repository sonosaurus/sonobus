#!/bin/bash

OPTS="--verbose"

if [ "$1" -eq "debug" ]; then
  cmake --build build --config Debug $OPTS
else
  cmake --build build --config Release $OPTS
fi

