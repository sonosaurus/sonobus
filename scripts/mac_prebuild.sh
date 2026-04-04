#!/bin/bash

#cp -v ../../scripts/SonoBus-mac-sandbox.entitlements SonoBus.entitlements

if grep sandbox SonoBus.entitlements &> /dev/null ; then
   cp -v ../../scripts/SonoBus-mac.entitlements SonoBus.entitlements
fi
