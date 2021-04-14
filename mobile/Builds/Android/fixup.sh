#!/bin/bash

mkdir -p app/src/main/res
mkdir -p app/src/release/res
mkdir -p app/src/debug/res

cp -r ../../images/android/* app/src/main/res/
cp -r ../../images/android/* app/src/release/res/
cp -r ../../images/android/* app/src/debug/res/

