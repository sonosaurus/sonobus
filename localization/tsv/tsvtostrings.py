#!/usr/bin/env python

import os,sys

if len(sys.argv) < 2:
    print('Usage: ' + sys.argv[0] + ' filename')
    sys.exit(1)

f = open(sys.argv[1])

lines = f.readlines()

# special case the first 3 lines

print lines[0].split('\t')[0]
print lines[1].split('\t')[0]
print

for line in lines[3:]:
    parts = line.split('\t')
    print('"' + parts[0] + '"'),
    print('='),
    if len(parts) > 1:
        print('"' + parts[1] + '"')
    else:
        print('""')
