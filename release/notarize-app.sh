#!/bin/bash

set -u # forbid use of uninitialised variables
set -e # exit on error

function die {
    echo "ERROR: $1";
    exit 1
}

# this is a specific password for the notarization service -- I'm storing  it the Keychain under the name
# 'Notarization-PASSWORD' -- the password is something like azcy-sdjd-defe-nufj
#account_pwd=$(security find-generic-password -a ${USER} -s Notarization-PASSWORD -w)
account_pwd="@keychain:Notarization-PASSWORD"
account_name="${APPLEID}" # obvsiouly you need to replace this with your developer account..

account_options="-u ${account_name} -p ${account_pwd} --asc-provider ${APPLEASC}"

RED="\033[1;31m"
GREEN="\033[1;32m"
YELLOW="\033[1;33m"
BLUE="\033[1;34m"
MAGENTA="\033[1;35m"
CYAN="\033[1;36m"
WHITE="\033[1;37m"
RESET="\033[0m"

do_submit=1
do_getresult=1

uuidfile=""

apps=()
main_app=""
while test "$#" -gt 0; do
  case "$1" in
 -*=*) optarg=`echo "$1" | sed 's/[-_a-zA-Z0-9]*=//'` ;;
  *) optarg= ;;
  esac
  case "$1" in
      --submit)
          do_getresult=0
          ;;
      --resume)
          do_submit=0
          ;;
      --submit=*)
          do_getresult=0
          uuidfile="$optarg"
          ;;
      --resume=*)
          do_submit=0
          uuidfile="$optarg"          
          ;;
      --primary-bundle-id=*)
          bundle_id="$optarg"
          ;;
      --help)
          echo "Usage: notarize-app MyBundle.app or notarize-app MyInstaller.pkg or notarize-app MyImage.dmg"
          echo "    will submit and wait for the result, and then will staple the app bundle."
          echo "    Your (specific) password for the notarization service must be stored in the Keychain"
          echo "    under the name 'Notarization-PASSWORD'"
          echo "options:"
          echo "    --submit[=uuidfile]  : submit the bundle for notarization, saves the uuid in a /tmp file and returns."
          echo "    --resume[=uuidfile]     : retrieve the uuid from the /tmp file, and check its status on apple servers"
          echo "                     if the notarization succeed, staple the app bundle."
          echo "    --primary-bundle-id : if submitting a .pkg or a .dmg you must specify a primary bundle id."
          exit 0
          ;;
      --trace)
          set -x
          ;;
      --*)
          die "wrong option: $1";
          ;;
      *)
          if [ -z "$main_app" ]; then main_app="$1"; fi
          apps+=("$1");
          ;;
  esac
  shift
done

# kill child process on ctrl-C..
trap "trap - SIGTERM && kill -- -$$" SIGINT SIGTERM

nb_apps=${#apps[*]}
if (( "$nb_apps" == 0 )); then
    die "Missing argument for .app bundle.."
fi
is_pkg=0
is_dmg=0
is_app=1
for app in "${apps[@]}"; do
    extension="${app##*.}"
    if [ "$extension" == "pkg" ]; then
        is_pkg=1;
    elif [ "$extension" == "dmg" ]; then
        is_dmg=1;
    else
        if [ ! -d "$app" ]; then
            die "$app is not a bundle folder!!";
        fi
        is_app=1
    fi
done

if (( $is_dmg )) || (( $is_pkg )); then
    is_app=0;
fi

if (( ${!apps[*]} > 1 )) && (( $is_app == 0 )); then
    die "Only one pkg or dmg at a time."
fi

if (( $is_app )); then
    bundle_id=$(plutil -extract CFBundleIdentifier xml1 -o - "${main_app}/Contents/Info.plist" | sed -n "s/.*<string>\(.*\)<\/string>.*/\1/p")
fi

echo "Bundle ID: $bundle_id"
if [ -z "${bundle_id}" ]; then
    die "Bundle id not found...";
fi

if [ -z "$uuidfile" ] ; then
  uuidfile="/tmp/notarize-app-${bundle_id}.uuid"
fi

tmpfile="/tmp/notarize-app-${bundle_id}"

ok=0

if (( $do_submit )); then
    if (( $is_app )); then
        tmpzip="/tmp/notarize-app-${bundle_id}.zip"
	rm -f "$tmpzip"
        zip -r "$tmpzip" "${apps[@]}"
    else
        tmpzip="${apps[0]}"
    fi

    #set -x # trace on
    tmpzip_basename="$(basename "$tmpzip")"
    echo "Submitting ${tmpzip_basename}, please be patient it must be uploaded, and JAVA is slow.."
    # some versions of altool write the uuid to stderr, others write to stdout, so we redirect *both* in the file...

    xcrun altool --notarize-app --primary-bundle-id "$bundle_id" $account_options  -f "$tmpzip" >& "$tmpfile"
    altool_status=$?

    if [[ "$altool_status" == "0" ]]; then
        uuid=$(grep "RequestUUID = " "$tmpfile" | sed -e 's/.* = //')
    else
        if [[ -f "$tmpfile" ]]; then
            # second chance, if the file has already been uploaded, try to find the uuid in the error log
            # "The software asset has already been uploaded. The update ID is xxxxxxx-xxxx-xxxx-xxxxxxxx"
            uuid=$(grep "The upload ID is " | sed -e 's/.* //')
            altool_status=0
        fi
    fi

    if [[ "$altool_status" != "0" ]]; then
        (cat "$tmpfile"; die "xcrun altool failed with exit code $ret")
    fi

    uuid=$(grep "RequestUUID = " "$tmpfile" | sed -e 's/.* = //')
    echo "uuid for submission: $uuid"
    echo "$uuid" > "$uuidfile"
    if [[ -z "$uuid" ]]; then
        [ -f "$tmpfile" ] && cat "$tmpfile"
        die "Something is wrong, the uuid is empty -- did Apple change again the output of altool ?"
    fi
else
    if [ ! -f "$uuidfile" ]; then
        die "File $uuidfile not found, can't resume..";
    fi
    uuid=$(cat "$uuidfile")
    echo "Checking uuid $uuid"
fi

if (( $do_getresult )); then
    numtry=0
    ok=0
    if [ "$uuid" != "" ]; then
        while true; do
            numtry=$((numtry+1))
            if (( $numtry > 1 )); then
                sleep 30;
            fi
            if (( $numtry > 50 )); then
                die "two many attempts, notarization seem to take foreever..."
            fi
            xcrun altool --notarization-info "$uuid" $account_options >> "$tmpfile" 2>&1 || continue;
            status=$(tail -3 "$tmpfile" | grep "Status Message" | sed -e 's/Status Message.*: *//')
            sec=$(($(date +%s) - $(stat -t %s -f %m -- "$uuidfile"))); # number of seconds since the creation of the file
            echo -e "$tmpfile, submitted $sec seconds ago, attempt #$numtry: $YELLOW$status$RESET";
            if [[ "$status" == *"Package Approved"* ]]; then
                ok=1
                break;
            fi
            if [ "$status" != "" ]; then
                ok=0;
                tail -5 "$tmpfile";
                break;
            fi
        done
    fi
fi

if (( $ok )); then
    for app in "${apps[@]}"; do
        xcrun stapler staple "$app"
    done
    echo -e "Application $GREEN${apps[*]}$RESET stapled!!"
else
    exit 42
fi
