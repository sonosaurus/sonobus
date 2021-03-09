#!/bin/bash

if [ -z "$1" ] ; then
   echo "Usage: $0 <version> <certpassword>"
   exit 1
fi

VERSION=$1

CERTPASS=$2

if [ -z "$CERTFILE" ] ; then
  echo You need to define CERTFILE env variable to sign anything
  exit 2
fi

BUILDDIR='../Builds/VisualStudio2017/x64/Release'
BUILDDIR32='../Builds/VisualStudio2017/Win32/Release32'

mkdir -p SonoBus/Plugins

cp -v ../doc/README_WINDOWS.txt SonoBus/README.txt
cp -v ${BUILDDIR}/Standalone\ Plugin/SonoBus.exe SonoBus/
cp -v ${BUILDDIR}/VST3/SonoBus.vst3 SonoBus/Plugins/
cp -v ${BUILDDIR}/VST/SonoBus.dll SonoBus/Plugins/
cp -pHLRv ${BUILDDIR}/AAX/SonoBus.aaxplugin SonoBus/Plugins/

mkdir -p SonoBus/Plugins32

cp -v ${BUILDDIR32}/Standalone\ Plugin/SonoBus.exe SonoBus/SonoBus32.exe
cp -v ${BUILDDIR32}/VST3/SonoBus.vst3 SonoBus/Plugins32/
cp -v ${BUILDDIR32}/VST/SonoBus.dll SonoBus/Plugins32/
#cp -pHLRv ${BUILDDIR}/AAX/SonoBus.aaxplugin SonoBus/Plugins32/


# sign AAX
if [ -n "${AAXSIGNCMD}" ]; then
  echo "Signing AAX plugin"
  ${AAXSIGNCMD} --keypassword "${CERTPASS}"  --in 'SonoBus\Plugins\Sonobus.aaxplugin' --out 'SonoBus\Plugins\Sonobus.aaxplugin'
fi


# sign executable
#signtool.exe sign /v /t "http://timestamp.digicert.com" /f "$CERTFILE" /p "$CERTPASS" SonoBus/SonoBus.exe

mkdir -p instoutput
rm -f instoutput/*


iscc /O"instoutput" "/Ssigntool=signtool.exe sign /t http://timestamp.digicert.com /f ${CERTFILE} /p ${CERTPASS} \$f"  /DSBVERSION="${VERSION}" wininstaller.iss

#signtool.exe sign /v /t "http://timestamp.digicert.com" /f SonosaurusCodeSigningSectigoCert.p12 /p "$CERTPASS" instoutput/

ZIPFILE=sonobus-${VERSION}-win.zip

cp -v ../doc/README_WINDOWS.txt instoutput/README.txt

rm -f ${ZIPFILE}

(cd instoutput; zip  ../${ZIPFILE} SonoBus\ Installer.exe README.txt )
