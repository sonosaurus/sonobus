#!/bin/bash

PREFIX=/usr/local

if [ -n "$1" ] ; then
  PREFIX="$1"
fi

echo "Un-Installing SonoBus from ${PREFIX} ... (specify destination as command line argument if you have it elsewhere)"

if ! rm -f ${PREFIX}/bin/SonoBus ; then
  echo
  echo "Looks like you need to run this with 'sudo $0'"
  exit 2
fi

rm -f ${PREFIX}/share/applications/sonobus.desktop
rm -f ${PREFIX}/pixmaps/sonobus.png
rm -rf ${PREFIX}/lib/vst3/SonoBus.vst3

echo "SonoBus uninstalled"
