#!/usr/bin/env bash
set -e

if [ ! -e "$1" ]; then
  echo "Usage: sign.sh FileToSign.apk"
  exit 1
fi

KEYSTORE="debug.keystore"
ALIAS="debug"

if [ ! $APK_PASSWORD ]; then
  echo "Set APK_PASSWORD environment variable"
  exit 2
fi

if [ -f "$KEYSTORE" ]; then
  echo "✔ Keystore already exists: $KEYSTORE"
  rm "$KEYSTORE"
fi

echo "Creating debug keystore..."

keytool -genkeypair -v \
  -keystore "$KEYSTORE" \
  -storepass "$APK_PASSWORD" \
  -alias "$ALIAS" \
  -keypass "$APK_PASSWORD" \
  -keyalg RSA \
  -keysize 2048 \
  -validity 10000 \
  -dname "CN=Android Debug,O=Android,C=US"

echo "✔ Keystore created: $KEYSTORE"

APK="$1"
KEYSTORE="debug.keystore"
ALIAS="debug"
# APK_PASSWORD="28f6c2050e3bae5ead"

if [ -z "$APK" ]; then
  echo "Usage: ./sign-apk.sh <apk-file>"
  exit 1
fi

if [ ! -f "$APK" ]; then
  echo "❌ APK not found: $APK"
  exit 1
fi

if [ ! -f "$KEYSTORE" ]; then
  echo "❌ Keystore not found: $KEYSTORE"
  echo "Run ./make-debug-keystore.sh first"
  exit 1
fi

# SIGNED_APK="${APK%.apk}-signed.apk"
SIGNED_APK="${APK%.apk}.apk"

echo "Signing APK..."
apksigner sign \
  --ks "$KEYSTORE" \
  --ks-key-alias "$ALIAS" \
  --ks-pass pass:"$APK_PASSWORD" \
  --key-pass pass:"$APK_PASSWORD" \
  --v4-signing-enabled false \
  --out "$SIGNED_APK" \
  "$APK"

echo "Verifying signature..."
apksigner verify --verbose "$SIGNED_APK"

echo "✔ Signed APK created:"
echo "  $SIGNED_APK"
