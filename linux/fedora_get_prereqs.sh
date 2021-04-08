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

