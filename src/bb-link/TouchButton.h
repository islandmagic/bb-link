#include "TouchButtonBase.h"
#include "Arduino.h"

#define LONG_PRESS_MS 2000
#define SHORT_PRESS_MS 500
class TouchButton : public TouchButtonBase
{
public:
  TouchButton(int buttonPin) : buttonPin(buttonPin), onLongPressedCallback(nullptr), onShortPressedCallback(nullptr), firstTouchMillis(0) {}

  void init() override
  {
    firstTouchMillis = 0;
  }

  void process() override
  {
    int touchValue = touchRead(buttonPin);
    // Log.infoln("Touch value %i", touchValue);
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

  void setOnLongPressedCallback(std::function<void(void)> callback) override
  {
    onLongPressedCallback = callback;
  }

  void setOnShortPressedCallback(std::function<void(void)> callback) override
  {
    onShortPressedCallback = callback;
  }

private:
  int buttonPin;
  std::function<void(void)> onLongPressedCallback;
  std::function<void(void)> onShortPressedCallback;
  unsigned long firstTouchMillis;
};