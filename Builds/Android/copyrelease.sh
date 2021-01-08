#!/bin/bash

if [ x"$1" = "x" ] ; then
  echo "Usage: $0 version"
  exit 2
fi

VERSION=$1

FNAME=sonobus-$VERSION-android.apk

cp -v app/build/outputs/apk/release_/release/app-release_-release.apk ../../release/android/$FNAME
