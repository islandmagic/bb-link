#include "StatusIndicatorBase.h"

#include <TinyPICO.h>

#include "math.h"

#define STATUS_INDICATOR_DEFAULT_BRIGHTNESS 24

#define FLASH_LED_ON_PERIOD_MS 60
#define FLASH_LED_OFF_PERIOD_MS 600
#define FAST_BLINK_LED_ON_PERIOD_MS 60
#define FAST_BLINK_LED_OFF_PERIOD_MS 40

class StatusIndicator : public StatusIndicatorBase
{
public:
  StatusIndicator(TinyPICO tp) : tp(tp)
  {
  }

  void init()
  {
    tp.DotStar_Clear();
    tp.DotStar_SetPower(true);
    tp.DotStar_SetBrightness(STATUS_INDICATOR_DEFAULT_BRIGHTNESS);

    status = disconnected;
    lastUpdate = millis();
    ledOn = false;
    currentColor = 0;
    currentBrightness = 0;
  }

  void set(status_t new_status)
  {
    status = new_status;
  }

  void setBrightnessMaybe(int brightness)
  {
    if (currentBrightness != brightness)
    {
      currentBrightness = brightness;
      tp.DotStar_SetBrightness(currentBrightness);
      tp.DotStar_Show(); // Setting brightness does not trigger show
    }
  }

  void setColorMaybe(u_int32_t color)
  {
    if (currentColor != color)
    {
      currentColor = color;
      tp.DotStar_SetPixelColor(currentColor);
    }
  }

  void render()
  {
    setColorMaybe(colorForStatus());

    switch (modeForStatus())
    {
    case fixed:
      setBrightnessMaybe(STATUS_INDICATOR_DEFAULT_BRIGHTNESS);
      ledOn = true;
      break;

    case breathe:
    {
      // https://thingpulse.com/breathing-leds-cracking-the-algorithm-behind-our-breathing-pattern/
      float ledBreatheValue = (exp(sin(millis() / 2000.0 * PI)) - 0.36787944) * 108.0;
      setBrightnessMaybe(ledBreatheValue);
    }
    break;

    case flash:
    {
      unsigned long now = millis();
      unsigned long delta = now - lastUpdate;
      if (ledOn)
      {
        if (delta >= FLASH_LED_ON_PERIOD_MS)
        {
          setBrightnessMaybe(0);
          ledOn = false;
          lastUpdate = now;
        }
      }
      else
      {
        if (delta >= FLASH_LED_OFF_PERIOD_MS)
        {
          setBrightnessMaybe(STATUS_INDICATOR_DEFAULT_BRIGHTNESS);
          ledOn = true;
          lastUpdate = now;
        }
      }
    }
    break;

    case fastBlink:
    {
      unsigned long now = millis();
      unsigned long delta = now - lastUpdate;
      if (ledOn)
      {
        if (delta >= FAST_BLINK_LED_ON_PERIOD_MS)
        {
          setBrightnessMaybe(0);
          ledOn = false;
          lastUpdate = now;
        }
      }
      else
      {
        if (delta >= FAST_BLINK_LED_OFF_PERIOD_MS)
        {
          setBrightnessMaybe(STATUS_INDICATOR_DEFAULT_BRIGHTNESS);
          ledOn = true;
          lastUpdate = now;
        }
      }
    }
    break;

    case fadeOut:
    {
      unsigned long now = millis();
      unsigned long delta = now - lastUpdate;
      if (delta >= 80)
      {
        if (currentBrightness == 0)
        {
          return;
        }
        setBrightnessMaybe(currentBrightness - 1);
        lastUpdate = now;
      }
    }
    break;
    }
  }

  void sleep()
  {
    tp.DotStar_Clear();
    tp.DotStar_SetPower(false);
  }

  u_int32_t colorForStatus()
  {
    switch (status)
    {
    case disconnected:
    case shutdown:
    case actionRegistered:
      return 0xff8503; // Amber
    case batteryFull:
    case batteryLow:
      return 0x00FF00;
    case batteryShutdown:
      return 0xFF0000;
    case connected:
    case ready:
    case scanning:
      return 0x0000FF;
    case tx:
      return 0xFF0000;
    case rx:
      return 0x00FF00;
    case duplex:
    case otaFlash:
      return 0xA020F0; // Purple
    case error:
      return 0xFF0000;
    }
  }

  led_mode_t modeForStatus()
  {
    switch (status)
    {
    case connected:
      return breathe;
    case error:
    case scanning:
      return flash;
    case batteryLow:
    case batteryShutdown:
    case actionRegistered:
    case otaFlash:
      return fastBlink;
    case shutdown:
      return fadeOut;
    default:
      return fixed;
    }
  }

private:
  TinyPICO tp;
  status_t status;
  bool ledOn;
  unsigned long lastUpdate;
  u_int32_t currentColor;
  int currentBrightness;
};