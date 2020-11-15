#!/bin/bash


# codesign them with developer ID cert

POPTS="--strict  --force --options=runtime --sign C7AF15C3BCF2AD2E5C102B9DB6502CFAE2C8CF3B --timestamp"
AOPTS="--strict  --force --options=runtime --sign C7AF15C3BCF2AD2E5C102B9DB6502CFAE2C8CF3B --timestamp"

codesign ${AOPTS} --entitlements ../Builds/MacOSX/Standalone_Plugin.entitlements SonoBus/SonoBus.app
codesign ${POPTS} --entitlements ../Builds/MacOSX/AU.entitlements  SonoBus/SonoBus.component
codesign ${POPTS} --entitlements ../Builds/MacOSX/VST3.entitlements SonoBus/SonoBus.vst3
codesign ${POPTS} --entitlements ../Builds/MacOSX/VST.entitlements  SonoBus/SonoBus.vst

# AAX is special
if [ -n "${AAXSIGNCMD}" ]; then
  echo "Signing AAX plugin"
  ${AAXSIGNCMD}  --in SonoBus/Sonobus.aaxplugin --out SonoBus/Sonobus.aaxplugin
fi

# notarize them

if ! ./notarize-app.sh SonoBus/SonoBus.app ; then
  echo Notarization App failed
  exit 2
fi

if ! ./notarize-app.sh SonoBus/SonoBus.component ; then
  echo Notarization AU failed
  exit 2
fi

if ! ./notarize-app.sh SonoBus/SonoBus.vst3 ; then
  echo Notarization VST3 failed
  exit 2
fi
  
if ! ./notarize-app.sh SonoBus/SonoBus.vst ; then
  echo Notarization VST2 failed
  exit 2
fi

if ! ./notarize-app.sh SonoBus/SonoBus.aaxplugin ; then
  echo Notarization AAX failed
  exit 2
fi





