#!/bin/bash

PREFIX=/usr/local

if [ -n "$1" ] ; then
  PREFIX="$1"
fi

mkdir -p deb${PREFIX}/bin
cp build/sonobus deb${PREFIX}/bin/sonobus

mkdir -p deb${PREFIX}/share/applications
cp sonobus.desktop deb${PREFIX}/share/applications/sonobus.desktop
chmod +x deb${PREFIX}/share/applications/sonobus.desktop

mkdir -p deb${PREFIX}/share/pixmaps
cp ../../images/sonobus_logo@2x.png deb${PREFIX}/share/pixmaps/sonobus.png

if [ -d build/sonobus.vst3 ] ; then
  mkdir -p deb${PREFIX}/lib/vst3
  cp -a build/sonobus.vst3 deb${PREFIX}/lib/vst3/
fi

mkdir -p deb/etc/apt/trusted.gpg.d
wget -O deb/etc/apt/trusted.gpg.d/sonobus.gpg https://sonosaurus.github.io/sonobus/apt/keyring.gpg

dpkg-deb -b deb/ sonobus.deb
