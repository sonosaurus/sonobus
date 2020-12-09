#!/bin/bash

PREFIX=/usr/local

if [ -n "$1" ] ; then
  PREFIX="$1"
fi

echo "Installing SonoBus to ${PREFIX} ... (specify destination as command line argument if you want it elsewhere)"

mkdir -p ${PREFIX}/bin
if ! cp build/SonoBus  ${PREFIX}/bin/SonoBus ; then
  echo
  echo "Looks like you need to run this as 'sudo $0'"
  exit 2
fi

mkdir -p ${PREFIX}/share/applications
cp  sonobus.desktop ${PREFIX}/share/applications/sonobus.desktop
chmod +x ${PREFIX}/share/applications/sonobus.desktop

mkdir -p ${PREFIX}/pixmaps
cp  ../../images/sonobus_logo@2x.png ${PREFIX}/pixmaps/sonobus.png

if [ -d build/SonoBus.vst3 ] ; then
  mkdir -p ${PREFIX}/lib/lxvst
  cp -a build/SonoBus.vst3 ${PREFIX}/lib/lxvst/

  echo "SonoBus VST3 plugin installed"
fi

echo "SonoBus application installed"

