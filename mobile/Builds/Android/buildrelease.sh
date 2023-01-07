#!/bin/bash

./gradlew assembleRelease -Psonoupload_keystore_password=${sonoupload_keystore_password} -Psonoupload_key_password=${sonoupload_key_password} -Psonoupload_key=${sonoupload_key} 

