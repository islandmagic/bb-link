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

#include "Adapter.h"

Adapter adapter = Adapter();

void setup()
{
  Serial.begin(115200);
  adapter.init();
}

void loop()
{
  adapter.perform();
}