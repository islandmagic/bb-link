#pragma once
#ifndef ADAPTER_H
#define ADAPTER_H

#include <TinyPICO.h>
#include "Arduino.h"

#include "Config.h"
#include "StatusIndicator.h"
#include "TouchButton.h"
#include "Bridge.h"
#include "FiniteStateMachine.h"

#define CAPACITIVE_TOUCH_INPUT_PIN T0 // Pin 4
#define ADAPTER_NAME "B.B. Link"      // Changing this will prevent RadioMail to know that QSY is supported.

DECLARE_STATE(AdapterState);

enum shutdown_reason_t
{
  userInitiated = 0x00,
  idleTimeout = 0x01,
  lowBattery = 0x02
};

class Adapter
{
public:
  Adapter();
  void init();
  void perform();

  Bridge bridge = Bridge(ADAPTER_NAME);

private:
  TinyPICO tp = TinyPICO();
  StatusIndicator statusIndicator = StatusIndicator(tp);
  TouchButton touchButton = TouchButton(CAPACITIVE_TOUCH_INPUT_PIN);

  unsigned long lastBatteryCheck = 0;
  shutdown_reason_t shutdownReason;

  AdapterState idleState;
  AdapterState inUseState;
  AdapterState shutdownState;
  AdapterState showBatteryState;
  FSMT<AdapterState> adapterStateMachine;

  void onLongPressed();
  void onShortPressed();
  void lowBatteryWatchguard();
  void updateSendReceiveStatus();
  bool isUSBPower();
  void doShutdown();

  void shutdownEnter();
  void shutdownUpdate();
  void shutdownExit();
  void idleEnter();
  void idleUpdate();
  void idleExit();
  void inUseEnter();
  void inUseUpdate();
  void inUseExit();
  void showBatteryEnter();
  void showBatteryUpdate();
  void showBatteryExit();
};

#endif