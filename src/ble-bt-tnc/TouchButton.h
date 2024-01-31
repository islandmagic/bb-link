#ifndef TOUCH_BUTTON_H
#define TOUCH_BUTTON_H

#include "Arduino.h"

/*
  Arrived at this by experimentation. On battery power, the lack of ground plane
  require a higher threshold to register
*/
#define TOUCH_THRESHOLD 75

class TouchButton
{
public:
  TouchButton(int buttonPin);
  void init();
  void process();
  void setOnLongPressedCallback(std::function<void(void)> callback);
  void setOnShortPressedCallback(std::function<void(void)> callback);

private:
  int buttonPin;
  std::function<void(void)> onLongPressedCallback;
  std::function<void(void)> onShortPressedCallback;
  unsigned long firstTouchMillis;
};

#endif