#!/bin/bash
#
# Install prerequisite packages for SonoBus build on Ubuntu

GITREPO="https://github.com/essej/sonobus.git"

PREREQS="libjack-jackd2-dev \
	libopus0 \
	libopus-dev \
	opus-tools \
	libasound2-dev \
	libx11-dev \
	libxext-dev \
	libxinerama-dev \
	libxrandr-dev \
	libxcursor-dev \
	libgl-dev \
	libfreetype6-dev \
	libcurl4-openssl-dev \
	"
echo ""
echo "Installing prerequisites - " $(date)
echo ""

sudo apt update

sudo apt-get -y install git build-essential $PREREQS



