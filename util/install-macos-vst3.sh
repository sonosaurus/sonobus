#!/bin/sh

if [ $USER != 'root' ]; then
  echo "/************************************/"
  echo "/*                                  */"
  echo "/* This script must be run as root: */"
  echo "/*                                  */"
  echo "/*   sudo ./install-macos-vst3.sh   */"
  echo "/*                                  */"
  echo "/************************************/"
  exit 1
fi

TARGET='/Library/Audio/Plug-Ins/VST3/SonoBusMendeni'

if [ ! -d $TARGET ]; then
  mkdir $TARGET || exit 1
fi

if [ ! -d 'VST3' ]; then
  echo "VST3 folder not found, exiting"
  exit 2
fi

cd VST3 && cp -av * $TARGET

codesign --force --deep --sign - $TARGET/SonoBusMendeni.vst3/Contents/MacOS/SonoBusMendeni || exit 3

spctl --master-disable
spctl --status

echo "/****************************************/"
echo "/*                                      */"
echo "/* Don't forget to re-enable Gatekeeper */"
echo "/*                                      */"
echo "/*      sudo spctl --master-enable      */"
echo "/*                                      */"
echo "/****************************************/"
