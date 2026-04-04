#!/bin/bash

cd ..

rm -rf build build32

./setupcmakewin.sh
./setupcmakewin32.sh

./buildcmake.sh




 