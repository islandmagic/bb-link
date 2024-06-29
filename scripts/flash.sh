#!/bin/bash
echo "*******************************************************"
echo "*** Flashing BB-Link to all connected ESP32 devices ***"
echo "*******************************************************"
devices=$(arduino-cli board list | grep 'wchusbserial' | awk '{print $1}')
for device in $devices; do
  echo "Flashing BB-Link to $device"
  arduino-cli board attach -b esp32:esp32:pico32 -p $device ../src/bb-link/bb-link.ino
  arduino-cli compile -u ../src/bb-link/bb-link.ino
  ruby post_flash.rb $device
done
echo "*******************************************************"
echo "*** Done                                            ***"
echo "*******************************************************"