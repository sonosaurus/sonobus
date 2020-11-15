#!/bin/bash

cd ../Builds/MacOSX

# clean

xcodebuild -alltargets clean


xcodebuild -configuration Release -scheme "SonoBus - Standalone Plugin" -derivedDataPath .

xcodebuild -configuration Release -scheme "SonoBus - AU" -derivedDataPath .
xcodebuild -configuration Release -scheme "SonoBus - VST3" -derivedDataPath .
xcodebuild -configuration Release -scheme "SonoBus - VST" -derivedDataPath .
xcodebuild -configuration Release -scheme "SonoBus - AAX" -derivedDataPath .



