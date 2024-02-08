#pragma once
#ifndef ADAPTER_H
#define ADAPTER_H

#include <TinyPICO.h>
#include "Arduino.h"
#include "esp_ota_ops.h"

#include "Config.h"
#include "StatusIndicator.h"
#include "TouchButton.h"
#include "Bridge.h"
#include "FiniteStateMachine.h"

#define CAPACITIVE_TOUCH_INPUT_PIN T0 // Pin 4
#define ADAPTER_NAME "B.B. Link"      // Changing this will prevent RadioMail to know that QSY is supported.

#define FIRMWARE_VERSION_MAJOR 0
#define FIRMWARE_VERSION_MINOR 7
#define FIRMWARE_VERSION_PATCH 0

#define HARDWARE_VERSION_MAJOR 1
#define HARDWARE_VERSION_MINOR 0

enum hardware_board_t {
  hardware_board_unknown = 0,
  hardware_board_tinypico = 1,
  hardware_board_bblink_c = 2
};
#define HARDWARE_BOARD hardware_board_bblink_c

DECLARE_STATE(AdapterState);

enum shutdown_reason_t
{
  userInitiated = 0x00,
  idleTimeout = 0x01,
  lowBattery = 0x02
};

class Adapter : public BLECharacteristicCallbacks
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
  AdapterState otaFlashState;
  FSMT<AdapterState> adapterStateMachine;

  BLECharacteristic *pOtaFlash;
  BLECharacteristic *pOtaIdentity;
  esp_ota_handle_t otaHandle = 0;

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
  void otaFlashEnter();
  void otaFlashUpdate();
  void otaFlashExit();

  void initBLEOtaService();

  void onWrite(BLECharacteristic *pCharacteristic);
  void onRead(BLECharacteristic *pCharacteristic);
};

#endif