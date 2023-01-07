#!/bin/bash

if [ x"$1" = "x" ] ; then
  echo "Usage: $0 version"
  exit 2
fi

VERSION=$1

FNAME=sonobus-$VERSION-android.apk

cp -v app/build/outputs/apk/release_/release/app-release_-release.apk ../../../release/android/$FNAME


# now make a zip of the symbols
DFNAME=debug_${FNAME/apk/zip}
rm -f ../../../release/android/${DFNAME}

cd app/build/intermediates/cmake/release_Release/obj
zip -r ../../../../../../../../../release/android/${DFNAME} */*.so
