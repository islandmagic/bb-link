/*
  (c) 2024 Island Magic Co. All Rights Reserved.

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
*/

#include <ArduinoLog.h>
const char *logLevels[] = {"OFF", "FATAL", "ERROR", "WARNING", "INFO", "TRACE", "VERBOSE"};

#include "Adapter.h"
Adapter adapter = Adapter();

void setup()
{
  // Slow down to save power
  setCpuFrequencyMhz(80); // 240, 160, 80

  Serial.begin(115200);
  delay(1000);

  // LOG_LEVEL_FATAL, LOG_LEVEL_ERROR, LOG_LEVEL_WARNING, LOG_LEVEL_INFO, LOG_LEVEL_TRACE, LOG_LEVEL_VERBOSE
  Log.begin(LOG_LEVEL_INFO, &Serial);

  Serial.println("\n ___   ___     _    _      _");
  Serial.println("| _ ) | _ )   | |  (_)_ _ | |__");
  Serial.println("| _ \\_| _ \\_  | |__| | ' \\| / /");
  Serial.println("|___(_)___(_) |____|_|_||_|_\\_\\\n");

  Serial.printf("Booting up %s v%d.%d.%d on %s v%d.%d\n\n", ADAPTER_NAME, FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR, FIRMWARE_VERSION_PATCH, HARDWARE_BOARD == hardware_board_tinypico ? "TinyPICO" : "Pico32", HARDWARE_VERSION_MAJOR, HARDWARE_VERSION_MINOR);

  Serial.printf("Heap free: %d, usage: %d %%\n", ESP.getFreeHeap(), 100 - (ESP.getFreeHeap() * 100) / ESP.getHeapSize());
  // ESP.getFreeSketchSpace() returns the total sketch space
  Serial.printf("Flash size: %d, total: %d, usage: %d %%\n", ESP.getSketchSize(), ESP.getFreeSketchSpace(), (ESP.getSketchSize() * 100) / ESP.getFreeSketchSpace());
  Serial.printf("CPU clock: %d Mhz\n", getCpuFrequencyMhz());
  Serial.printf("Log level: %s\n", logLevels[Log.getLevel()]);

  adapter.init();
}

void loop()
{
  adapter.perform();
}