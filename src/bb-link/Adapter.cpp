#include <ArduinoLog.h>
#include "Adapter.h"

#define IDLE_TIME_TO_SLEEP 5 * 60 * 1000          // 5 min, time before shutting down when neither devices are connected
#define ACTION_FEEDBACK_DURATION 2000             // Duration for action feedback
#define LINGER_TIME_BEFORE_SHUTDOWN 2000          // Grace time for user to untouch button before sleeping, since touch would wake up device again
#define BATTERY_FULL_VOLTAGE 3.750                // Voltage above which to consider LiPo battery charged
#define BATTERY_MIN_VOLTAGE 3.500                 // Voltage below which esp32 should go to sleep as battery is going to rapidely drop its charge
#define SHOW_BATTERY_DURATION 3000                // Duration for battery indicator to stay on
#define BATTERY_WATCHGUARD_INTERVAL 3 * 60 * 1000 // Interval between battery check

#define VBUS_SENSE_GPIO 9

#define SERVICE_UUID_OTA "1A68D2B0-C2E4-453F-A2BB-B659D66CF442"
#define CHARACTERISTIC_UUID_OTA_FLASH "1A68D2B1-C2E4-453F-A2BB-B659D66CF442"
#define CHARACTERISTIC_UUID_OTA_IDENTITY "1A68D2B2-C2E4-453F-A2BB-B659D66CF442"

extern const char *logLevels[];

// AdapterState
Adapter::Adapter() : idleState(
                         [this]
                         { this->idleEnter(); },
                         [this]
                         { this->idleUpdate(); },
                         [this]
                         { this->idleExit(); }),
                     inUseState(
                         [this]
                         { this->inUseEnter(); },
                         [this]
                         { this->inUseUpdate(); },
                         [this]
                         { this->inUseExit(); }),
                     shutdownState(
                         [this]
                         { this->shutdownEnter(); },
                         [this]
                         { this->shutdownUpdate(); },
                         [this]
                         { this->shutdownExit(); }),
                     showBatteryState(
                         [this]
                         { this->showBatteryEnter(); },
                         [this]
                         { this->showBatteryUpdate(); },
                         [this]
                         { this->showBatteryExit(); }),
                     otaFlashState(
                         [this]
                         { this->otaFlashEnter(); },
                         [this]
                         { this->otaFlashUpdate(); },
                         [this]
                         { this->otaFlashExit(); }),
                     adapterStateMachine(idleState)
{
}

void Adapter::init()
{
  Log.traceln("Adapter: init");
  pinMode(VBUS_SENSE_GPIO, INPUT);

  statusIndicator.init();
  touchButton.init();

  touchButton.setOnLongPressedCallback(std::bind(&Adapter::onLongPressed, this));
  touchButton.setOnShortPressedCallback(std::bind(&Adapter::onShortPressed, this));

#if defined(ARDUINO_TINYPICO)
  touchSleepWakeUpEnable(CAPACITIVE_TOUCH_INPUT_PIN, TOUCH_THRESHOLD);
#endif

  if (!bridge.init())
  {
    Log.fatalln("FATAL: Bridge init failed !!!!!");
    statusIndicator.set(error);
    while (true)
    {
      statusIndicator.render();
      delay(10);
    }
  }

  // Make sure bridge is initialized first
  initBLEOtaService();

  verifyFirmware();
}

void Adapter::verifyFirmware()
{
  Log.traceln("Checking firmware...");
  const esp_partition_t *running = esp_ota_get_running_partition();
  esp_ota_img_states_t ota_state;
  if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK)
  {
    const char *otaState = ota_state == ESP_OTA_IMG_NEW              ? "ESP_OTA_IMG_NEW"
                           : ota_state == ESP_OTA_IMG_PENDING_VERIFY ? "ESP_OTA_IMG_PENDING_VERIFY"
                           : ota_state == ESP_OTA_IMG_VALID          ? "ESP_OTA_IMG_VALID"
                           : ota_state == ESP_OTA_IMG_INVALID        ? "ESP_OTA_IMG_INVALID"
                           : ota_state == ESP_OTA_IMG_ABORTED        ? "ESP_OTA_IMG_ABORTED"
                                                                     : "ESP_OTA_IMG_UNDEFINED";
    Log.infoln("OTA state: %s", otaState);

    if (ota_state == ESP_OTA_IMG_PENDING_VERIFY)
    {
      if (esp_ota_mark_app_valid_cancel_rollback() == ESP_OK)
      {
        Log.infoln("App is valid, rollback cancelled successfully");
      }
      else
      {
        Log.errorln("Failed to cancel rollback");
      }
    }
  }
  else
  {
    Log.warningln("OTA partition has no record in OTA data");
  }
}

void Adapter::initBLEOtaService()
{
  Log.traceln("Adapter: init BLE OTA service");

  // Create the BLE Service for OTA
  BLEService *pOtaService = bridge.getBLEServer()->createService(SERVICE_UUID_OTA);

  pOtaFlash = pOtaService->createCharacteristic(
      CHARACTERISTIC_UUID_OTA_FLASH,
      BLECharacteristic::PROPERTY_WRITE);

  pOtaIdentity = pOtaService->createCharacteristic(
      CHARACTERISTIC_UUID_OTA_IDENTITY,
      BLECharacteristic::PROPERTY_READ);

  pOtaFlash->addDescriptor(new BLE2902());

  pOtaFlash->setAccessPermissions(ESP_GATT_PERM_WRITE);
  pOtaIdentity->setAccessPermissions(ESP_GATT_PERM_READ);

  pOtaFlash->setCallbacks(this);

  uint8_t identity[6] = {HARDWARE_BOARD, HARDWARE_VERSION_MAJOR, HARDWARE_VERSION_MINOR, FIRMWARE_VERSION_MAJOR, FIRMWARE_VERSION_MINOR, FIRMWARE_VERSION_PATCH};
  pOtaIdentity->setValue(identity, 6);

  pOtaService->start();
}

void Adapter::perform()
{
  statusIndicator.render();
  if (!adapterStateMachine.isInState(otaFlashState))
  {
    touchButton.process();
    lowBatteryWatchguard();
  }
  adapterStateMachine.update();
}

void Adapter::updateSendReceiveStatus()
{
  if (bridge.isTx() && bridge.isRx())
  {
    // The adapter is full duplex and can receive and send at same time.
    statusIndicator.set(duplex);
  }
  else if (bridge.isRx())
  {
    statusIndicator.set(rx);
  }
  else if (bridge.isTx())
  {
    statusIndicator.set(tx);
  }
  else
  {
    statusIndicator.set(ready);
  }
}

void Adapter::lowBatteryWatchguard()
{
#if defined(ARDUINO_TINYPICO)
  unsigned long now = millis();
  if (now - lastBatteryCheck > BATTERY_WATCHGUARD_INTERVAL)
  {
    lastBatteryCheck = now;
    float level = tp.GetBatteryVoltage();
    Log.infoln("Battery voltage %F V", level);
    if (level < BATTERY_MIN_VOLTAGE)
    {
      Log.warningln("Voltage too low, initiating shutdown");
      shutdownReason = lowBattery;
      adapterStateMachine.transitionTo(shutdownState);
    }
  }
#endif
}

bool Adapter::isUSBPower()
{
#if defined(ARDUINO_TINYPICO)
  // TinyPICO has a resistor divider on USB power line
  return (digitalRead(VBUS_SENSE_GPIO) != 0);
#else
  return true;
#endif
}

void Adapter::doShutdown()
{
  Log.infoln("Going to deep sleep...");
  bridge.disconnect();
  statusIndicator.sleep();
  esp_deep_sleep_start();
}

/*
  Touch button Callbacks
*/
void Adapter::onLongPressed()
{
  Log.infoln("Long pressed button, shutdown");
  shutdownReason = userInitiated;
  statusIndicator.set(actionRegistered);
  unsigned long now = millis();
  while (millis() - now < ACTION_FEEDBACK_DURATION)
  {
    statusIndicator.render();
    delay(10);
  }
  adapterStateMachine.transitionTo(shutdownState);
}

void Adapter::onShortPressed()
{
  Log.infoln("Short pressed button");
  // Only show status when idle
  if (adapterStateMachine.isInState(idleState))
  {
    Log.infoln("Showing battery status");
    adapterStateMachine.transitionTo(showBatteryState);
  }
}

/*
  BLECharacteristicCallbacks
*/
void Adapter::onWrite(BLECharacteristic *pCharacteristic)
{
  std::string rxData = pCharacteristic->getValue();

  if (!adapterStateMachine.isInState(otaFlashState))
  {
    Log.infoln("OTA: begin flash");
    adapterStateMachine.immediateTransitionTo(otaFlashState);
    esp_ota_begin(esp_ota_get_next_update_partition(NULL), OTA_SIZE_UNKNOWN, &otaHandle);
  }

  if (rxData.length() > 0)
  {
    esp_ota_write(otaHandle, rxData.c_str(), rxData.length());
    Log.infoln("OTA: written %i bytes", rxData.length());
  }
  else
  {
    Log.infoln("OTA: end flash");
    esp_ota_end(otaHandle);
    if (esp_ota_set_boot_partition(esp_ota_get_next_update_partition(NULL)) == ESP_OK)
    {
      Log.infoln("OTA: success, rebooting");
      delay(2000);
      esp_restart();
    }
    else
    {
      Log.errorln("OTA: failed");
    }
    adapterStateMachine.transitionTo(idleState);
  }
}

void Adapter::onRead(BLECharacteristic *pCharacteristic)
{
  Log.errorln("OTA: onRead!!!!");
}

/*
  Adapter state machine
*/
void Adapter::idleEnter()
{
  Log.traceln("Adapter: idle");
  statusIndicator.set(disconnected);
}

void Adapter::idleUpdate()
{

  bridge.perform();

  if (bridge.isReady())
  {
    adapterStateMachine.transitionTo(inUseState);
  }
  else if (bridge.btcConnected())
  {
    statusIndicator.set(connected);
  }
  else if (bridge.btcDiscovery())
  {
    statusIndicator.set(scanning);
  }
  else
  {
    statusIndicator.set(disconnected);
  }

  unsigned int idleTime = adapterStateMachine.timeInCurrentState();
  if (idleTime > IDLE_TIME_TO_SLEEP)
  {
    Log.infoln("Idle for %i s", idleTime / 1000);

    // Do not auto shutdown when on USB power
    if (isUSBPower())
    {
      Log.infoln("On USB power, stay awake");
      // Reset the time in state
      adapterStateMachine.transitionTo(idleState);
    }
    else
    {
      Log.infoln("On battery power, initiating shutdown");
      shutdownReason = idleTimeout;
      adapterStateMachine.transitionTo(shutdownState);
    }
  }

  if (Serial.available())
  {
    char ch = Serial.read();
    switch (ch)
    {
    case 'r':
      Log.warningln("Reboot");
      delay(2000);
      esp_restart();
      break;
    case 'R':
      Log.warningln("Perform factory reset");
      bridge.factoryReset();
      adapterStateMachine.transitionTo(idleState);
      break;
    case 'v':
      // Decrease log verbosity by one level
      if (Log.getLevel() > LOG_LEVEL_SILENT)
      {
        Log.setLevel(Log.getLevel() - 1);
      }
      Serial.printf("Log level: %s\n", logLevels[Log.getLevel()]);
      break;
    case 'V':
      // Increase log verbosity by one level
      if (Log.getLevel() < LOG_LEVEL_VERBOSE)
      {
        Log.setLevel(Log.getLevel() + 1);
      }
      Serial.printf("Log level: %s\n", logLevels[Log.getLevel()]);
      break;
    }
  }
}

void Adapter::idleExit()
{
}

void Adapter::inUseEnter()
{
  Log.traceln("Adapter: ready for use");
  statusIndicator.set(ready);
}

void Adapter::inUseUpdate()
{
  bridge.perform();

  if (!bridge.isReady())
  {
    adapterStateMachine.transitionTo(idleState);
  }
  else
  {
    updateSendReceiveStatus();
  }
}

void Adapter::inUseExit()
{
}

void Adapter::shutdownEnter()
{
  Log.traceln("Adapter: shutdown");
  switch (shutdownReason)
  {
  case userInitiated:
  case idleTimeout:
    statusIndicator.set(shutdown);
    break;

  case lowBattery:
    statusIndicator.set(batteryShutdown);
    break;
  }
}

void Adapter::shutdownUpdate()
{
  if (adapterStateMachine.timeInCurrentState() > LINGER_TIME_BEFORE_SHUTDOWN)
  {
    doShutdown();
  }
}

void Adapter::shutdownExit()
{
}

void Adapter::showBatteryEnter()
{
#if defined(ARDUINO_TINYPICO)
  float level = tp.GetBatteryVoltage();
  Log.infoln("Battery voltage %F V", level);
  if (level > BATTERY_FULL_VOLTAGE)
  {
    statusIndicator.set(batteryFull);
  }
  else
  {
    statusIndicator.set(batteryLow);
  }
#endif
}

void Adapter::showBatteryUpdate()
{
  if (adapterStateMachine.timeInCurrentState() >= SHOW_BATTERY_DURATION)
  {
    adapterStateMachine.transitionTo(idleState);
  }
}

void Adapter::showBatteryExit()
{
}

void Adapter::otaFlashEnter()
{
  Log.traceln("Adapter: OTA flash");
  statusIndicator.set(otaFlash);
  bridge.disconnect();
}

void Adapter::otaFlashUpdate()
{
  // Do nothing, OTA is handled by BLECharacteristicCallbacks
}

void Adapter::otaFlashExit()
{
  // If OTA flash was successful, the device will reboot
}
