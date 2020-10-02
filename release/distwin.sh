#!/bin/bash

if [ -z "$1" ] ; then
   echo "Usage: $0 <version>"
   exit 1
fi

VERSION=$1

mkdir -p SonoBus/Plugins

cp -v ../doc/README_WINDOWS.txt SonoBus/README.txt
cp -v ../Builds/VisualStudio2017/x64/Release/Standalone\ Plugin/SonoBus.exe SonoBus/
cp -v ../Builds/VisualStudio2017/x64/Release/VST3/SonoBus.vst3 SonoBus/Plugins/
cp -v ../Builds/VisualStudio2017/x64/Release/VST/SonoBus.dll SonoBus/Plugins/


mkdir -p instoutput
rm -f instoutput/*


iscc /O"instoutput" /DSBVERSION="${VERSION}" wininstaller.iss

ZIPFILE=sonobus-${VERSION}-win.zip

cp -v ../doc/README_WINDOWS.txt instoutput/README.txt

rm -f ${ZIPFILE}

(cd instoutput; zip  ../${ZIPFILE} SonoBus\ Installer.exe README.txt )
