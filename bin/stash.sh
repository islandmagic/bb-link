#!/bin/bash

# extract version from git tag
VERSION=$(git describe --tags --abbrev=0)

# make sure there is a version
if [ -z "$VERSION" ]; then
    echo "No version found"
    exit 1
fi

cp ../src/bb-link/build/esp32.esp32.pico32/bb-link.ino.bin ~/Downloads/bb-link-pico32-${VERSION}.bin
cp ../src/bb-link/build/esp32.esp32.tinypico/bb-link.ino.bin ~/Downloads/bb-link-tinypico-${VERSION}.bin