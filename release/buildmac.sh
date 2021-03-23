#!/bin/bash

cd ../Builds/MacOSX

# clean

# xcodebuild -configuration Release -derivedDataPath . -alltargets  clean
echo CLEANING ALL

xcodebuild -configuration Release -scheme "SonoBus - Standalone Plugin" -derivedDataPath . clean
xcodebuild -configuration Release -scheme "SonoBus - AU" -derivedDataPath . clean
xcodebuild -configuration Release -scheme "SonoBus - VST3" -derivedDataPath . clean
xcodebuild -configuration Release -scheme "SonoBus - VST" -derivedDataPath . clean
xcodebuild -configuration Release -scheme "SonoBus - AAX" -derivedDataPath . clean


echo BUILDING ALL

xcodebuild -configuration Release -scheme "SonoBus - Standalone Plugin" -derivedDataPath .

xcodebuild -configuration Release -scheme "SonoBus - AU" -derivedDataPath .
xcodebuild -configuration Release -scheme "SonoBus - VST3" -derivedDataPath .
xcodebuild -configuration Release -scheme "SonoBus - VST" -derivedDataPath .
xcodebuild -configuration Release -scheme "SonoBus - AAX" -derivedDataPath .



