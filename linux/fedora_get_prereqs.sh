#!/bin/bash
#
# Install prerequisite packages for SonoBus build on Fedora

GITREPO="https://github.com/essej/sonobus.git"

PREREQS="libopusenc \
	libopusenc-devel \
	jack-audio-connection-kit \
	jack-audio-connection-kit-devel \
	alsa-lib-devel \
	libX11-devel \
	libXext-devel \
	libXinerama-devel \
	libXrandr-devel \
	libXcursor-devel \
	freetype-devel \
	libcurl-devel \
	cmake
        "

echo ""
echo "Installing prerequisites - " $(date)
echo ""

sudo dnf update
sudo dnf -y groupinstall "Development Tools" "Development Libraries"
sudo dnf -y install git $PREREQS


function ver { printf "%03d%03d%03d%03d" $(echo "$1" | tr '.' ' '); }

cmakever=$(cmake --version | head -1 | cut -d" " -f3)

if [ $(ver $cmakever) -lt $(ver 3.16) ] ; then
  echo "Your CMake is too old! You need version 3.16 or higher. Try to get a newer version, or compile CMake from source."
  exit 1
fi

