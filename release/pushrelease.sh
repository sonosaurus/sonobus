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
  
  for f in *.zip; do

    ## Check if the glob gets expanded to existing files.
    ## If not, f here will be exactly the pattern above
    ## and the exists test will evaluate to false.
    [ -e "$f" ] && havezip="1" || havezip=""

    ## This is all we needed to know, so we can break after the first iteration
    break
  done

  if [ -z "$havezip" ] ; then
  	echo "No Zip files!"
	exit 1
  fi
  
  FILES=`echo *.zip`
  
  echo "Making release $1 with assets: ${FILES}"
  gitrelease.sh essej/sonobus $1 -- *.zip < notes.md

  echo
  echo "Done!"

else
  echo "No version $1 directory exists"
  exit 1   
fi