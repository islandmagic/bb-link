#include "Adapter.h"

#define IDLE_TIME_TO_SLEEP 5 * 60 * 1000          // 5 min, time before shutting down when neither devices are connected
#define ACTION_FEEDBACK_DURATION 2000             // Duration for action feedback
#define LINGER_TIME_BEFORE_SHUTDOWN 2000          // Grace time for user to untouch button before sleeping, since touch would wake up device again
#define BATTERY_FULL_VOLTAGE 3.750                // Voltage above which to consider LiPo battery charged
#define BATTERY_MIN_VOLTAGE 3.500                 // Voltage below which esp32 should go to sleep as battery is going to rapidely drop its charge
#define SHOW_BATTERY_DURATION 3000                // Duration for battery indicator to stay on
#define BATTERY_WATCHGUARD_INTERVAL 3 * 60 * 1000 // Interval between battery check

#define VBUS_SENSE_GPIO 9

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
                     adapterStateMachine(idleState)
{
}

void Adapter::init()
{
  pinMode(VBUS_SENSE_GPIO, INPUT);

  statusIndicator.init();
  touchButton.init();

  touchButton.setOnLongPressedCallback(std::bind(&Adapter::onLongPressed, this));
  touchButton.setOnShortPressedCallback(std::bind(&Adapter::onShortPressed, this));

  touchSleepWakeUpEnable(CAPACITIVE_TOUCH_INPUT_PIN, TOUCH_THRESHOLD);

  if (!bridge.init())
  {
    Serial.println("FATAL: Bridge init failed !!!!!");
    statusIndicator.set(error);
    while (true)
    {
      statusIndicator.render();
      delay(10);
    }
  }
}

void Adapter::perform()
{
  statusIndicator.render();
  touchButton.process();
  lowBatteryWatchguard();
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
  unsigned long now = millis();
  if (now - lastBatteryCheck > BATTERY_WATCHGUARD_INTERVAL)
  {
    lastBatteryCheck = now;
    float level = tp.GetBatteryVoltage();
    Serial.printf("Battery voltage %.3f V\n", level);
    if (level < BATTERY_MIN_VOLTAGE)
    {
      Serial.println("Voltage too low, initiating shutdown");
      shutdownReason = lowBattery;
      adapterStateMachine.transitionTo(shutdownState);
    }
  }
}

bool Adapter::isUSBPower()
{
  return (digitalRead(VBUS_SENSE_GPIO) != 0);
}

void Adapter::doShutdown()
{
  Serial.println("Going to deep sleep...");
  bridge.disconnect();
  statusIndicator.sleep();
  esp_deep_sleep_start();
}

/*
  Touchbutton Callbacks
*/
void Adapter::onLongPressed()
{
  Serial.println("Long pressed button, shutdown");
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
  Serial.println("Short pressed button");
  // Only show status when idle
  if (adapterStateMachine.isInState(idleState))
  {
    Serial.println("Showing battery status");
    adapterStateMachine.transitionTo(showBatteryState);
  }
}

/*
  Adapter state machine
*/
void Adapter::idleEnter()
{
  Serial.println("Adapter: idle");
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
    Serial.printf("Idle for %i s\n", idleTime / 1000);

    // Do not auto shutdown when on USB power
    if (isUSBPower())
    {
      Serial.println("On USB power, stay awake");
      // Reset the time in state
      adapterStateMachine.transitionTo(idleState);
    }
    else
    {
      Serial.println("On battery power, initiating shutdown");
      shutdownReason = idleTimeout;
      adapterStateMachine.transitionTo(shutdownState);
    }
  }

  if (Serial.available())
  {
    char ch = Serial.read();
    switch (ch)
    {
    case 'R':
      Serial.println("Perform factory reset");
      bridge.factoryReset();
      adapterStateMachine.transitionTo(idleState);
      break;
    }
  }
}

void Adapter::idleExit()
{
}

void Adapter::inUseEnter()
{
  Serial.println("Adapter: ready for use");
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
  Serial.println("Adapter: shutdown");
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
  float level = tp.GetBatteryVoltage();
  Serial.printf("Battery voltage %.3f V\n", level);
  if (level > BATTERY_FULL_VOLTAGE)
  {
    statusIndicator.set(batteryFull);
  }
  else
  {
    statusIndicator.set(batteryLow);
  }
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