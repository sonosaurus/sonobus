#!/usr/bin/env python3

# adjust version for packages

import os,sys
import plistlib

if len(sys.argv) < 4:
    print('Usage: %s <version> <infile.pkgproj> <outfile.pkgproj>', file=sys.stderr)
    sys.exit(1)

version = sys.argv[1]

with open(sys.argv[2], 'rb') as infile:

    dict = plistlib.load(infile)

    for n in range(0, len(dict['PACKAGES'])):
        dict['PACKAGES'][n]['PACKAGE_SETTINGS']['VERSION'] = version
        
    with open(sys.argv[3], 'wb') as fp:
        plistlib.dump(dict, fp)
    



