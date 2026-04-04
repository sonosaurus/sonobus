#!/bin/bash

if [ -z "$1" ] ; then
  echo "Usage: $0 <version>"
  exit 1
fi

VERSION=$1

rm -f SonoBusPkg.dmg

cp SonoBus/README_MAC.txt SonoBusPkg/

if dropdmg --config-name=SonoBusPkg --layout-folder SonoBusPkgLayout --volume-name="SonoBus v${VERSION}"  --APP_VERSION=v${VERSION}  --signing-identity=C7AF15C3BCF2AD2E5C102B9DB6502CFAE2C8CF3B SonoBusPkg
then
  mkdir -p ${VERSION}
  mv -v SonoBusPkg.dmg ${VERSION}/sonobus-${VERSION}-mac.dmg  	
else
  echo "Error making package DMG"
  exit 2
fi

