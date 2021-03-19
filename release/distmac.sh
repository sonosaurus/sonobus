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
cp -pRHv ${BUILDDIR}/SonoBus.aaxplugin  SonoBus/

ln -sf /Library/Audio/Plug-Ins/Components SonoBus/
ln -sf /Library/Audio/Plug-Ins/VST3 SonoBus/
ln -sf /Library/Audio/Plug-Ins/VST SonoBus/
ln -sf /Library/Application\ Support/Avid/Audio/Plug-Ins SonoBus/


# this codesigns and notarizes everything
if ! ./codesign.sh ; then
  echo
  echo Error codesign/notarizing, stopping
  echo
  exit 1
fi

# make installer package (and sign it)

if ! packagesbuild  macpkg/SonoBus.pkgproj ; then
  echo 
  echo Error building package
  echo
  exit 1
fi

mkdir -p SonoBusPkg
rm -f SonoBusPkg/*

if ! productsign --sign ${INSTSIGNID} --timestamp  macpkg/build/SonoBus\ Installer.pkg SonoBusPkg/SonoBus\ Installer.pkg ; then
  echo 
  echo Error signing package
  echo
  exit 1
fi

# make dmg with package inside it

if ./makepkgdmg.sh $VERSION ; then

   ./notarizedmg.sh ${VERSION}/sonobus-${VERSION}-mac.dmg

   echo
   echo COMPLETED DMG READY === ${VERSION}/sonobus-${VERSION}-mac.dmg
   echo
   
fi
