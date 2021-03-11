#!/bin/bash

cd ../Builds/VisualStudio2017


#ARGS="-v:m  -m  -noWarn:C4244,C4267,C4996,C4305,C4189,C4100,C4127,C4458"
ARGS="-m  /clp:ErrorsOnly  -v:m"

# build everything in Release mode
echo "Building 64 bit"

MSBuild.exe SonoBus.sln ${ARGS} -t:Rebuild /p:Configuration=Release /p:Platform=x64

# build 32 bit
echo "Building 32 bit"

#MSBuild.exe SonoBus.sln ${ARGS} -t:Rebuild /p:Configuration=Release32 /p:Platform=Win32
MSBuild.exe SonoBus_SharedCode.vcxproj ${ARGS}  -t:Rebuild /p:Configuration=Release32 /p:Platform=Win32
MSBuild.exe SonoBus_StandalonePlugin.vcxproj ${ARGS} -t:Rebuild /p:Configuration=Release32 /p:Platform=Win32
MSBuild.exe SonoBus_VST.vcxproj ${ARGS} -t:Rebuild /p:Configuration=Release32 /p:Platform=Win32
MSBuild.exe SonoBus_VST3.vcxproj ${ARGS} -t:Rebuild /p:Configuration=Release32 /p:Platform=Win32


# build VST2 with ReleaseVST2
#MSBuild.exe SonoBus_SharedCode.vcxproj ${ARGS}  -t:Rebuild /p:Configuration=ReleaseVST2 /p:Platform=x64
#MSBuild.exe SonoBus_VST.vcxproj ${ARGS} -t:Rebuild /p:Configuration=ReleaseVST2 /p:Platform=x64



 