/*
  (c) 2024 Island Magic Co. All Rights Reserved.
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