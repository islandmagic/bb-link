#!/bin/bash

# extract version from git tag
VERSION=$(git describe --tags --abbrev=0)

# remove the v prefix
VERSION=${VERSION:1}

# make sure there is a version
if [ -z "$VERSION" ]; then
    echo "No version found"
    exit 1
fi

cp ../src/bb-link/build/esp32.esp32.pico32/bb-link.ino.bin ~/Downloads/bb-link-pico32-${VERSION}.bin
cp ../src/bb-link/build/esp32.esp32.tinypico/bb-link.ino.bin ~/Downloads/bb-link-tinypico-${VERSION}.bin

# generate sha256sum and save only checksum to file
sha256sum ~/Downloads/bb-link-pico32-${VERSION}.bin | cut -d ' ' -f 1 > ~/Downloads/bb-link-pico32-${VERSION}.bin.sha256.txt
sha256sum ~/Downloads/bb-link-tinypico-${VERSION}.bin | cut -d ' ' -f 1 > ~/Downloads/bb-link-tinypico-${VERSION}.bin.sha256.txt

# generate .json file
echo "{
  \"version\": \"${VERSION}\",
  \"url\": \"https://github.com/islandmagic/bb-link/releases/download/v${VERSION}/bb-link-pico32-${VERSION}.bin\",
  \"sha256\": \"$(cat ~/Downloads/bb-link-pico32-${VERSION}.bin.sha256.txt)\",
}" > ~/Downloads/bb-link-pico32-${VERSION}.json

echo "{
  \"version\": \"${VERSION}\",
  \"url\": \"https://github.com/islandmagic/bb-link/releases/download/v${VERSION}/bb-link-tinypico-${VERSION}.bin\",
  \"sha256\": \"$(cat ~/Downloads/bb-link-tinypico-${VERSION}.bin.sha256.txt)\",
}" > ~/Downloads/bb-link-tinypico-${VERSION}.json
