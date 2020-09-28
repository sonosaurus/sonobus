#!/bin/bash

mkdir -p SonoBus/Plugins

cp -v ../doc/README_WINDOWS.txt SonoBus/
cp -v ../Builds/VisualStudio2017/x64/Release/Standalone\ Plugin/SonoBus.exe SonoBus/
cp -v ../Builds/VisualStudio2017/x64/Release/VST3/SonoBus.vst3 SonoBus/Plugins/
cp -v ../Builds/VisualStudio2017/x64/Release/VST/SonoBus.dll SonoBus/Plugins/
