#!/bin/bash

PREFIX=/usr/local

if [ -n "$1" ] ; then
  PREFIX="$1"
fi

mkdir -p deb${PREFIX}/bin
if ! cp build/SonoBus  deb${PREFIX}/bin/SonoBus ; then
  echo
  echo "Looks like you need to run this as 'sudo $0'"
  exit 2
fi

mkdir -p deb${PREFIX}/share/applications
cp sonobus.desktop deb${PREFIX}/share/applications/sonobus.desktop
chmod +x deb${PREFIX}/share/applications/sonobus.desktop

mkdir -p deb${PREFIX}/share/pixmaps
cp ../../images/sonobus_logo@2x.png deb${PREFIX}/share/pixmaps/sonobus.png

if [ -d build/SonoBus.vst3 ] ; then
  mkdir -p deb${PREFIX}/lib/vst3
  cp -a build/SonoBus.vst3 deb${PREFIX}/lib/vst3/
fi

dpkg-deb -b deb/ sonobus.deb
