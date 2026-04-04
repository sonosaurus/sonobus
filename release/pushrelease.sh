#/usr/bin/env bash

if [ -z "$1" ] ; then
  echo "Usage: $0 <version>"
  exit 1
fi

if [ -d "$1" ] ; then
  cd $1
  if [ ! -f notes.md ] ; then
  	echo "No notes.md exists!"
  	exit 1
  fi
  
  for f in *.exe *.dmg; do

    ## Check if the glob gets expanded to existing files.
    ## If not, f here will be exactly the pattern above
    ## and the exists test will evaluate to false.
    [ -e "$f" ] && havezip="1" || havezip=""

    ## This is all we needed to know, so we can break after the first iteration
    break
  done

  if [ -z "$havezip" ] ; then
  	echo "No archive files!"
	exit 1
  fi
  
  FILES=`echo *.exe *.dmg`
  
  echo "Making release $1 with assets: ${FILES}"
  gitrelease.sh sonosaurus/sonobus $1 -- *.exe *.dmg < notes.md

  echo
  echo "Done!"

else
  echo "No version $1 directory exists"
  exit 1   
fi