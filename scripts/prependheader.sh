#!/bin/bash

IFS= read -r -d '' HEADER << EOF
// SPDX-License-Identifier: GPLv3-or-later
// Copyright (C) 2020 Jesse Chappell
EOF

if [ -z "$1" ] ; then
  echo "Usage: $0 file [files...]"
fi

# echo "$HEADER"
for f in $* ; do
  echo -e "${HEADER}\n$(cat ${f})" > "${f}"
  echo "Prepended license header for $f"
done
