#include "TouchButton.h"

#define LONG_PRESS_MS 2000
#define SHORT_PRESS_MS 500

TouchButton::TouchButton(int buttonPin) : buttonPin(buttonPin)
{
}

void TouchButton::init()
{
  firstTouchMillis = 0;
}

void TouchButton::process()
{
  int touchValue = touchRead(buttonPin);
  // Serial.printf("Touch value %i\n", touchValue);
  if (touchValue < TOUCH_THRESHOLD)
  {
    // Button touched
    if (firstTouchMillis == 0)
    {
      firstTouchMillis = millis();
    }
    else if (millis() - firstTouchMillis >= LONG_PRESS_MS)
    {
      firstTouchMillis = 0;

      // Fire long press
      if (onLongPressedCallback != nullptr)
      {
        onLongPressedCallback();
      }
    }
  }
  else
  {
    if (firstTouchMillis > 0)
    {
      if (millis() - firstTouchMillis >= SHORT_PRESS_MS)
        if (onShortPressedCallback != nullptr)
        {
          onShortPressedCallback();
        }
    }

    firstTouchMillis = 0;
  }
}

void TouchButton::setOnLongPressedCallback(std::function<void(void)> callback)
{
  onLongPressedCallback = callback;
}

void TouchButton::setOnShortPressedCallback(std::function<void(void)> callback)
{
  onShortPressedCallback = callback;
}
