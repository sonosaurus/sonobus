#!/bin/bash

if [ -z "$1" ] ; then
   echo "Usage: $0 <version>"
   exit 1
fi

VERSION=$1


BUILDDIR=../Builds/MacOSX/build/Release

rm -rf SonoBus

mkdir -p SonoBus


cp ../doc/README_MAC.txt SonoBus/

cp -pLRv ${BUILDDIR}/SonoBus.app  SonoBus/
cp -pLRv ${BUILDDIR}/SonoBus.component  SonoBus/
cp -pLRv ${BUILDDIR}/SonoBus.vst3 SonoBus/
cp -pLRv ${BUILDDIR}/SonoBus.vst  SonoBus/

ln -sf /Library/Audio/Plug-Ins/Components SonoBus/
ln -sf /Library/Audio/Plug-Ins/VST3 SonoBus/
ln -sf /Library/Audio/Plug-Ins/VST SonoBus/


# this codesigns and notarizes everything
if ! ./codesign.sh ; then
  echo
  echo Error codesign/notarizing, stopping
  echo
  exit 1
fi


if ./makedmg.sh $VERSION ; then

   ./notarizedmg.sh ${VERSION}/sonobus-${VERSION}-mac.dmg

   echo
   echo COMPLETED DMG READY === ${VERSION}/sonobus-${VERSION}-mac.dmg
   echo
   
fi
