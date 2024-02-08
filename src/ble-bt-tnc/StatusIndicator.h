#ifndef STATUS_INDICATOR_H
#define STATUS_INDICATOR_H

#include "Arduino.h"
#include <TinyPICO.h>

enum status_t
{
  disconnected = 0x00,
  connected = 0x01,
  ready = 0x02,
  tx = 0x03,
  rx = 0x04,
  duplex = 0x05,
  error = 0x06,
  shutdown = 0x07,
  batteryFull = 0x08,
  batteryLow = 0x09,
  batteryShutdown = 0x0A,
  actionRegistered = 0x0B,
  scanning = 0x0C,
  otaFlash = 0x0D
};

enum led_mode_t
{
  fixed = 0x00,
  breathe = 0x01,
  flash = 0x02,
  fastBlink = 0x03,
  fadeOut = 0x04
};

class StatusIndicator
{
public:
  StatusIndicator(TinyPICO tp);
  void init();
  void set(status_t status);
  void render();
  void sleep();

private:
  TinyPICO tp;
  status_t status;
  bool ledOn;
  unsigned long lastUpdate;
  u_int32_t currentColor;
  int currentBrightness;

  u_int32_t colorForStatus();
  led_mode_t modeForStatus();
  void setBrightnessMaybe(int brightness);
  void setColorMaybe(u_int32_t color);
};

#endif