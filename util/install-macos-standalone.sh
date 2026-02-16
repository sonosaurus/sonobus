#!/bin/sh

TARGET='/Applications'

if [ ! -d $TARGET ]; then
  exit 1
fi

if [ ! -d 'Standalone' ]; then
  echo "Standalone folder not found, exiting"
  exit 2
fi

cd Standalone && cp -av * $TARGET

codesign --force --deep --sign - $TARGET/SonoBusMendeni.app/Contents/MacOS/SonoBusMendeni || exit 3
