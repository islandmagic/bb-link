#include <ArduinoLog.h>
#include "Bridge.h"
#include "Adapter.h"
#include <map>

/*
  More info about those magic numbers
  https://github.com/hessu/aprs-specs/blob/master/BLE-KISS-API.md
*/
#define SERVICE_UUID "00000001-ba2a-46c9-ae49-01b0961f68bb"
#define TX_UUID "00000002-ba2a-46c9-ae49-01b0961f68bb" // From the perspective of the BLE app
#define RX_UUID "00000003-ba2a-46c9-ae49-01b0961f68bb" // From the perspective of the BLE app

#define BYTE_TRANSMIT_TIME 7             // Aprox time in ms to transmit a byte at 1200 baud
#define RETRY_BTC_CONNECT_INTERVAL 15000 // Try to connect to radio Bluetooth Classic interface every x ms
const size_t RX_BUF_SIZE = 1024;         // BLE 4.2 supports up to 512. MTU is negotiated by client.

const char PREF_RADIO_NAME[] = "radioName";
const char PREF_RADIO_ADDRESS[] = "radioAddress";
const char PREF_RIG_CTRL[] = "rigCtrl";

extern char _remote_name[ESP_BT_GAP_MAX_BDNAME_LEN + 1];
extern bool _isRemoteAddressSet;

extern Adapter adapter;

void connectToBluetooth(void *address)
{
  Log.traceln("BTC: connecting to Bluetooth Classic interface");
  adapter.bridge.btSerial.connect((uint8_t *)address, 0, ESP_SPP_SEC_NONE, ESP_SPP_ROLE_MASTER);
  vTaskDelete(NULL);
}

Bridge::Bridge(String adapterName) : bleDisconnectedState(
                                         [this]
                                         { this->bleDisconnectedEnter(); },
                                         [this]
                                         { this->bleDisconnectedUpdate(); },
                                         [this]
                                         { this->bleDisconnectedExit(); }),
                                     bleConnectedState(
                                         [this]
                                         { this->bleConnectedEnter(); },
                                         [this]
                                         { this->bleConnectedUpdate(); },
                                         [this]
                                         { this->bleConnectedExit(); }),
                                     bleStateMachine(bleDisconnectedState),
                                     btcDisconnectedState(
                                         [this]
                                         { this->btcDisconnectedEnter(); },
                                         [this]
                                         { this->btcDisconnectedUpdate(); },
                                         [this]
                                         { this->btcDisconnectedExit(); }),
                                     btcConnectedState(
                                         [this]
                                         { this->btcConnectedEnter(); },
                                         [this]
                                         { this->btcConnectedUpdate(); },
                                         [this]
                                         { this->btcConnectedExit(); }),
                                     btcDiscoveryState(
                                         [this]
                                         { this->btcDiscoveryEnter(); },
                                         [this]
                                         { this->btcDiscoveryUpdate(); },
                                         [this]
                                         { this->btcDiscoveryExit(); }),
                                     btcStateMachine(btcDisconnectedState),
                                     adapterName(adapterName),
                                     cmdQueue(10)
{
}

bool Bridge::init()
{
  Log.traceln("Bridge: init");
  rxLingerUntil = millis();
  txLingerUntil = millis();

  preferences.begin(PREFERENCES_NAMESPACE, false);
  useRigControl = preferences.getBool(PREF_RIG_CTRL, true);
  preferences.end();

  Log.infoln("Use rig control: %s", useRigControl ? "true" : "false");

  bool ok = initBTC();
  if (ok)
  {
    lookUpLastPairedDevice();
  }

  return (initBLE() && ok);
}

void Bridge::perform()
{
  bleStateMachine.update();
  btcStateMachine.update();

  uint8_t rxBuf[RX_BUF_SIZE];
  size_t rxLen = 0;

  // Process any command received from BLE
  while (!cmdQueue.isEmpty())
  {
    Log.traceln("BLE: dequeueing extended hardware command");
    processingCmdQueue = true;
    extended_hw_cmd_t cmd = cmdQueue.dequeue();
    processExtendedHardwareCommand(&cmd);
  }
  processingCmdQueue = false;

  if (isReady())
  {
    // Buffer data available from BTC
    while (btSerial.available() && rxLen < mtuSize)
    {
      rxBuf[rxLen++] = btSerial.read();
    }
    // Send data to BLE
    if (rxLen > 0)
    {
      Log.traceln("BLE < BTC: %i", rxLen);
      setRxLinger(BYTE_TRANSMIT_TIME * rxLen);
      pRx->setValue(rxBuf, rxLen);
      pRx->notify();
    }
  }
}

void Bridge::disconnect()
{
  clearAllPendingBTCData();
  connectToPairedDevice = false;
  btSerial.disconnect();
}

void Bridge::factoryReset()
{
  disconnect();
  clearPairedDevices();
  preferences.begin(PREFERENCES_NAMESPACE, false);
  preferences.clear();
  preferences.end();
  useRigControl = true;
  connectToPairedDevice = false;

  // Restart the device
  esp_restart();
}

String Bridge::getAdapterName()
{
  return adapterName;
}

bool Bridge::isReady()
{
  return (bleStateMachine.isInState(bleConnectedState) && btcStateMachine.isInState(btcConnectedState));
}

bool Bridge::btcConnected()
{
  return (btcStateMachine.isInState(btcConnectedState));
}

bool Bridge::btcDiscovery()
{
  return (btcStateMachine.isInState(btcDiscoveryState));
}

bool Bridge::initBTC()
{
  Log.traceln("Bridge: initBTC");
  btSerial.enableSSP();
  btSerial.setPin("0000");

  btSerial.onConfirmRequest([this](uint32_t num)
                            { this->onBTConfirmRequestCallback(num); });

  btSerial.onAuthComplete([this](bool success)
                          { this->onBTAuthCompleteCallback(success); });

  if (!btSerial.begin(adapterName, true))
  {
    Log.fatalln("FATAL: BTC init failed !!!!!");
    return false;
  }
  else
  {
    return true;
  }
}

bool Bridge::isTx()
{
  return (txLingerUntil > millis());
}

bool Bridge::isRx()
{
  return (rxLingerUntil > millis());
}

bool Bridge::initBLE()
{
  Log.traceln("Bridge: initBLE");

  BLEDevice::init(adapterName.c_str());
  pBLEServer = BLEDevice::createServer();
  pBLEServer->setCallbacks(this);

  BLEService *pService = pBLEServer->createService(SERVICE_UUID);

  pTx = pService->createCharacteristic(
      TX_UUID,
      BLECharacteristic::PROPERTY_WRITE_NR);

  pRx = pService->createCharacteristic(
      RX_UUID,
      BLECharacteristic::PROPERTY_NOTIFY);

  pRx->addDescriptor(new BLE2902());

  pRx->setAccessPermissions(ESP_GATT_PERM_READ);
  pTx->setAccessPermissions(ESP_GATT_PERM_WRITE);

  pTx->setCallbacks(this);
  pRx->setCallbacks(this);

  pService->start();

  return true;
}

BLEServer *Bridge::getBLEServer()
{
  return pBLEServer;
}

void Bridge::startAdvertisingBLE()
{
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06); // functions that help with iPhone connections issue
  BLEDevice::startAdvertising();
}

void Bridge::stopAdvertisingBLE()
{
  BLEDevice::stopAdvertising();
}

void Bridge::clearAllPendingBTCData()
{
  // Purge anything that may be pending
  if (btSerial.connected())
  {
    btSerial.flush();
    while (btSerial.available())
    {
      btSerial.read();
    }
  }
}

void Bridge::lookUpLastPairedDevice()
{
  int count = esp_bt_gap_get_bond_device_num();

  clearRemoteDeviceInfo();

  if (!count)
  {
    Log.infoln("No paired device found");
    clearStoredPairedDeviceInfo();
  }
  else
  {
    Log.infoln("Found %i paired device", count);
    if (PAIR_MAX_DEVICES < count)
    {
      count = PAIR_MAX_DEVICES;
    }

    esp_err_t tError = esp_bt_gap_get_bond_device_list(&count, pairedDeviceBtAddr);

    if (ESP_OK == tError)
    {
      for (int i = 0; i < count; i++)
      {
        Log.infoln("Device %i, address: %s", i, BTAddress(pairedDeviceBtAddr[i]).toString().c_str());

        // Normally there should only be one paired device at a time. Check the address matches
        // what was stored in preferences.
        preferences.begin(PREFERENCES_NAMESPACE, false);
        String radioName = preferences.getString(PREF_RADIO_NAME);
        u_int8_t radioAddress[ESP_BD_ADDR_LEN];
        preferences.getBytes(PREF_RADIO_ADDRESS, radioAddress, ESP_BD_ADDR_LEN);
        preferences.end();

        // Check if address match
        if (BTAddress(radioAddress).equals(BTAddress(pairedDeviceBtAddr[i])))
        {
          Log.infoln("Found paired device name: %s", radioName);
          memcpy(remoteAddress, pairedDeviceBtAddr[i], ESP_BD_ADDR_LEN);
          strcpy(remoteName, radioName.c_str());
          connectToPairedDevice = true;
          break;
        }
        else
        {
          Log.infoln("Found paired device name does not match saved, name: %s address: %s", radioName, BTAddress(radioAddress).toString().c_str());
          clearPairedDevices();
          break;
        }
      }
    }
  }
}

void Bridge::clearStoredPairedDeviceInfo()
{
  preferences.begin(PREFERENCES_NAMESPACE, false);
  preferences.remove(PREF_RADIO_NAME);
  preferences.remove(PREF_RADIO_ADDRESS);
  preferences.end();
}

void Bridge::clearRemoteDeviceInfo()
{
  remoteName[0] = '\0';
  memset(remoteAddress, 0, ESP_BD_ADDR_LEN);
  connectToPairedDevice = false;
}

void Bridge::clearPairedDevices()
{
  int count = esp_bt_gap_get_bond_device_num();

  clearStoredPairedDeviceInfo();
  clearRemoteDeviceInfo();

  Log.traceln("Clear paired devices");

  if (!count)
  {
    Log.infoln("No paired device found");
  }
  else
  {
    Log.infoln("Found %i paired device(s)", count);
    if (PAIR_MAX_DEVICES < count)
    {
      count = PAIR_MAX_DEVICES;
    }

    esp_err_t tError = esp_bt_gap_get_bond_device_list(&count, pairedDeviceBtAddr);

    if (ESP_OK == tError)
    {
      for (int i = 0; i < count; i++)
      {
        Log.infoln("Device %i, address: %s", i, BTAddress(pairedDeviceBtAddr[i]).toString().c_str());

        esp_err_t tError = esp_bt_gap_remove_bond_device(pairedDeviceBtAddr[i]);
        if (ESP_OK == tError)
        {
          Log.traceln("Removed paired device");
        }
        else
        {
          Log.errorln("Failed to remove paired device");
        }
      }
    }
  }
}

void Bridge::setTxLinger(int linger)
{
  unsigned long now = millis();
  if (txLingerUntil < now)
  {
    txLingerUntil = now;
  }
  txLingerUntil += linger;
}

void Bridge::setRxLinger(int linger)
{
  unsigned long now = millis();
  if (rxLingerUntil < now)
  {
    rxLingerUntil = now;
  }
  rxLingerUntil += linger;
}

void Bridge::processExtendedHardwareCommand(extended_hw_cmd_t *cmd)
{
  switch (cmd->action)
  {
  case extended_hw_set_frequency:
  {
    if (useRigControl)
    {
      Log.traceln("BTC: extended_hw_set_frequency");
      previousFrequency = 0;

      if (vfo != vfoUnknown)
      {
        // At this point, we should always be in KISS mode. Exit KISS mode first so we don't have to wait for a timeout
        thd7x.exitKISS();

        // Just to be sure
        if (thd7x.isKISSMode())
        {
          Log.warningln("BTC: still in KISS mode?");
          thd7x.exitKISS();
        }

        Log.infoln("BTC: try to get baud rate");
        if (thd7x.getBaudRate(&previousBaudRate))
        {
          Log.infoln("BTC: previous baud rate: %d", previousBaudRate);
        }

        if (desiredBaudRate != baudRateUnknown && previousBaudRate != desiredBaudRate)
        {
          Log.infoln("BTC: set baud rate");
          thd7x.setBaudRate(desiredBaudRate);
        }

        Log.infoln("BTC: try to get mode");
        if (thd7x.getMode(vfo, &previousMode))
        {
          Log.infoln("BTC: previous mode: %d", previousMode);
          if (previousMode != modeFM)
          {
            thd7x.setMode(vfo, modeFM);
          }
        }
        else
        {
          previousMode = modeUnknown;
          Log.errorln("BTC: failed to get previous mode");
        }

        Log.infoln("BTC: try to set frequency: %d", cmd->data.uint32);

        if (thd7x.getFrequency(vfo, &previousFrequency))
        {
          Log.infoln("BTC: previousFrequency: %l", previousFrequency);
          thd7x.setFrequency(vfo, cmd->data.uint32);
        }
        else
        {
          previousFrequency = 0;
          Log.errorln("BTC: failed to get previous frequency");
        }

        // Show time
        thd7x.setTNC(vfo, tncKISS);
      }
      else
      {
        Log.warningln("BTC: VFO is unknown! Cowardly refusing to set frequency");
      }
    }
    break;
  }
  case extended_hw_restore_frequency:
  {
    if (useRigControl)
    {
      Log.traceln("BTC: extended_hw_restore_frequency");

      if (vfo != vfoUnknown && previousFrequency > 0)
      {
        // Always exit KISS mode first so we don't have to wait for a timeout
        thd7x.exitKISS();

        // Just to be sure
        if (thd7x.isKISSMode())
        {
          Log.warningln("BTC: still in KISS mode?");
          thd7x.exitKISS();
        }

        Log.infoln("BTC: try to restore frequency to %i", previousFrequency);
        thd7x.setFrequency(vfo, previousFrequency);
        previousFrequency = 0;

        if (previousMode != modeUnknown && previousMode != modeFM)
        {
          thd7x.setMode(vfo, previousMode);
        }

        if (desiredBaudRate != baudRateUnknown && previousBaudRate != desiredBaudRate)
        {
          Log.infoln("BTC: restore baud rate");
          thd7x.setBaudRate(previousBaudRate);
        }

        // As long as BLE is connected, we want the radio to be in KISS mode
        thd7x.setTNC(vfo, tncKISS);
      }
      else
      {
        Log.infoln("BTC: no previous frequency to restore");
      }
    }
    break;
  }
  case extended_hw_set_baud_rate:
  {
    Log.traceln("BTC: extended_hw_set_baud_rate");
    /*
      Save the desired baud rate for now. We could set the radio right away
      but that would require getting out of KISS mode which is an expensive operation
      and leads to the radio beeping. It's also expected to be seldome as there are not
      many 9600 stations.
      Defeer setting the baud rate when changing frequency. This assumes that the host 
      application sets the baud rate first.
    */
    desiredBaudRate = static_cast<baud_rate_t>(cmd->data.uint8);
    break;
  }
  case extended_hw_start_scan:
  {
    Log.traceln("BTC: extended_hw_start_scan");
    btcStateMachine.transitionTo(btcDiscoveryState);
    break;
  }
  case extended_hw_stop_scan:
  {
    Log.traceln("BTC: extended_hw_stop_scan");
    btcStateMachine.transitionTo(btcDisconnectedState);
    break;
  }
  case extended_hw_pair_with_device:
  {
    Log.traceln("BTC: extended_hw_pair_with_device");
    btSerial.disconnect();
    clearPairedDevices();

    // iterate thru the list of devices found during scan
    // check if the device address is in the list and get its name
    btDeviceList = btSerial.getScanResults();
    for (int i = 0; i < btDeviceList->getCount(); i++)
    {
      BTAdvertisedDevice *device = btDeviceList->getDevice(i);
      if (device->getAddress().equals(BTAddress(cmd->data.bytes)))
      {
        Log.infoln("BTC: Pairing with: %s %s", device->getName().c_str(), device->getAddress().toString().c_str());
        memcpy(remoteAddress, device->getAddress().getNative(), sizeof(esp_bd_addr_t));
        strcpy(remoteName, device->getName().c_str());
        connectToPairedDevice = true;
        // force connections
        btcStateMachine.immediateTransitionTo(btcDisconnectedState);
        break;
      }
    }
    break;
  }
  case extended_hw_clear_paired_device:
  {
    Log.traceln("BTC: extended_hw_clear_paired_device");
    btSerial.disconnect();
    clearPairedDevices();
    break;
  }
  case extended_hw_api_version:
  {
    Log.traceln("BTC: extended_hw_api_version");
    reply16(EXTENDED_HW_CMD_API_VERSION, API_VERSION);
    break;
  }
  case extended_hw_firmware_version:
  {
    Log.traceln("BTC: extended_hw_firmware_version");
    // create period delimited version string
    String version = String(FIRMWARE_VERSION_MAJOR) + "." + String(FIRMWARE_VERSION_MINOR) + "." + String(FIRMWARE_VERSION_PATCH);
    reply(EXTENDED_HW_CMD_FIRMWARE_VERSION, (uint8_t *)version.c_str(), strlen(version.c_str()));
    break;
  }
  case extended_hw_capabilities:
  {
    Log.traceln("BTC: extended_hw_capabilities");
    uint16_t caps;
    caps = useRigControl ? CAP_RIG_CTRL : 0 | CAP_FIRMWARE_VERSION;
    reply16(EXTENDED_HW_CMD_CAPABILITIES, caps);
    break;
  }
  case extended_hw_get_paired_device:
  {
    Log.traceln("BTC: extended_hw_get_paired_device");
    found_device_t paired;
    paired.connected = btcConnected() ? 0x01 : 0x00;
    memcpy(paired.address, remoteAddress, sizeof(esp_bd_addr_t));
    strcpy(paired.name, remoteName);
    reply(EXTENDED_HW_CMD_GET_PAIRED_DEVICE, reinterpret_cast<uint8_t *>(&paired), 1 + sizeof(esp_bd_addr_t) + strlen(paired.name));
    break;
  }
  case extended_hw_set_rig_ctrl:
  {
    Log.traceln("BTC: extended_hw_set_rig_ctrl");
    if (cmd->data.uint8 == 0x00)
    {
      Log.infoln("BTC: set rig control off");
      useRigControl = false;
    }
    else
    {
      Log.infoln("BTC: set rig control on");
      useRigControl = true;
    }
    preferences.begin(PREFERENCES_NAMESPACE, false);
    preferences.putBool(PREF_RIG_CTRL, useRigControl);
    preferences.end();
    break;
  }
  case extended_hw_factory_reset:
  {
    Log.traceln("BTC: extended_hw_factory_reset");
    factoryReset();
    break;
  }
  default:
    Log.errorln("BTC: unknown extended hardware command");
    break;
  }
}

void Bridge::reply8(uint8_t cmd, uint8_t data)
{
  uint8_t buffer[3] = {CMD_HARDWARE, cmd, data};
  reply(buffer, 3);
}

void Bridge::reply16(uint8_t cmd, uint16_t data)
{
  // Big endian
  uint8_t buffer[4] = {CMD_HARDWARE, cmd, uint8_t((data >> 8) & 0xFF), uint8_t(data & 0xFF)};
  reply(buffer, 4);
}

void Bridge::reply(uint8_t cmd, uint8_t *data, size_t size)
{
  uint8_t buffer[size + 2];
  buffer[0] = CMD_HARDWARE;
  buffer[1] = cmd;
  memcpy(buffer + 2, data, size);
  reply(buffer, size + 2);
}

void Bridge::reply(uint8_t *response, size_t size)
{
  uint8_t buffer[size * 2 + 2];
  size_t bufferSize = sizeof(buffer);

  if (kissInterceptor.escape(response, size, buffer, &bufferSize))
  {
    Log.infoln("BLE < (adapter): %i", bufferSize);
    pRx->setValue(buffer, bufferSize);
    pRx->notify();
  }
  else
  {
    Log.errorln("Failed to escape response");
  }
}

/*
  BLEServerCallbacks
*/
void Bridge::onConnect(BLEServer *pServer, esp_ble_gatts_cb_param_t *param)
{
  Log.traceln("BLE: onConnect");

  esp_ble_conn_update_params_t conn_params = {};
  memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
  conn_params.latency = 0;
  conn_params.max_int = 32;  // *1.25ms = 40ms
  conn_params.min_int = 16;  // *1.25ms = 20ms
  conn_params.timeout = 500; // *10ms = 5000ms
  // Start the update process
  esp_ble_gap_update_conn_params(&conn_params);

  bleStateMachine.transitionTo(bleConnectedState);
}

void Bridge::onDisconnect(BLEServer *pServer)
{
  Log.traceln("BLE: onDisconnect");
  bleStateMachine.transitionTo(bleDisconnectedState);
}

void Bridge::onMtuChanged(BLEServer *pServer, esp_ble_gatts_cb_param_t *param)
{
  Log.traceln("BLE: onMtuChanged");
  mtuSize = param->mtu.mtu;
  Log.infoln("New MTU size: %d", mtuSize);
}

/*
  BLECharacteristicCallbacks
*/
void Bridge::onWrite(BLECharacteristic *pCharacteristic)
{
  std::string txValue = pCharacteristic->getValue();

  if (txValue.length() > 0)
  {
    Log.traceln("BLE Rx: %i", txValue.length());

    extended_hw_cmd_t cmd;
    if (kissInterceptor.extractExtendedHardwareCommand((uint8_t *)pCharacteristic->getData(), txValue.length(), &cmd))
    {
      Log.traceln("BLE: queueing extended hardware command");
      cmdQueue.enqueue(cmd);
    }
    else if (btcStateMachine.isInState(btcConnectedState))
    {
      // Drop data if we're still processing cmds
      // This is to avoid sending data to the radio while it may not have changed frequency yet
      if (processingCmdQueue || !cmdQueue.isEmpty())
      {
        Log.traceln("BLE: dropping data while still processing hw commands");
        return;
      }

      Log.traceln("BLE > BTC: %i", txValue.length());
      btSerial.write(pCharacteristic->getData(), txValue.length());
      setTxLinger(BYTE_TRANSMIT_TIME * txValue.length());
    }
  }
}

void Bridge::onRead(BLECharacteristic *pCharacteristic)
{
  Log.errorln("BLE: onRead!!!!");
}

/*
  BTC callbacks
*/
void Bridge::onBTConfirmRequestCallback(uint32_t numVal)
{
  Log.infoln("Pairing pin: %l", numVal);
  btSerial.confirmReply(true);
}

void Bridge::onBTAuthCompleteCallback(boolean success)
{
  if (success)
  {
    Log.infoln("Pairing success");
    /*
      Save the name and address of the radio we just paired
      This so we can connect to it next time we start the device and
      display its name in the configuration app
    */
    preferences.begin(PREFERENCES_NAMESPACE, false);
    preferences.putString(PREF_RADIO_NAME, remoteName);
    preferences.putBytes(PREF_RADIO_ADDRESS, remoteAddress, ESP_BD_ADDR_LEN);
    preferences.end();
  }
  else
  {
    Log.warningln("Pairing failed, rejected by user");
  }
}

/*
  BTC states
*/
void Bridge::btcDisconnectedEnter()
{
  Log.infoln("BTC: disconnected");
  if (connectToPairedDevice)
  {
    // The connect method in BT serial is blocking. Use a task to connect
    Log.infoln("BTC: attempt to connect to %s at %s", remoteName, BTAddress(remoteAddress).toString().c_str());
    btSerial.disconnect(); // Just in case. If radio is already connected, reconnecting could lead to crash
    xTaskCreate(
        connectToBluetooth,    // Task function
        "connectBT",           // Task name
        4096,                  // Stack size
        (void *)remoteAddress, // Task input parameter
        1,                     // Priority of the task
        NULL                   // Task handle
    );
  }
}

void Bridge::btcDisconnectedUpdate()
{
  if (btSerial.connected())
  {
    btcStateMachine.transitionTo(btcConnectedState);
  }

  if (btcStateMachine.timeInCurrentState() > RETRY_BTC_CONNECT_INTERVAL)
  {
    // Retry connection
    btcStateMachine.immediateTransitionTo(btcDisconnectedState);
  }
}

void Bridge::btcDisconnectedExit()
{
}

void Bridge::btcConnectedEnter()
{
  Log.infoln("BTC: connected");
  clearAllPendingBTCData();
}

void Bridge::btcConnectedUpdate()
{
  if (!btSerial.connected())
  {
    btcStateMachine.transitionTo(btcDisconnectedState);
  }
}

void Bridge::btcConnectedExit()
{
}

void Bridge::btcDiscoveryEnter()
{
  Log.traceln("BTC: discovery");
  btSerial.disconnect();

  btDeviceList = btSerial.getScanResults();
  if (btSerial.discoverAsync([this](BTAdvertisedDevice *pDevice)
                             {
    Log.infoln("Found device: %s", pDevice->toString().c_str());
    /*
    Name: TH-D74, Address: 04:ee:03:61:2d:b0, cod: 0x620204, rssi: -55
    Name: TNC4 Mobilinkd, Address: 34:81:f4:aa:b2:dd, cod: 0x4c0300, rssi: -24
    Name: PicoAPRS, Address: 4c:75:25:65:29:82, cod: 0x001f00, rssi: -64
    Name: TH-D75, Address: 40:79:12:e4:65:44, cod: 0x620204, rssi: -62
    */
    // Filter list to known Kenwood handsets capabilities signature
    // https://www.ampedrftech.com/cod.htm?result=620204
    if (pDevice->getCOD() == 0x620204)
    {
      found_device_t found;
      found.connected = 0x00;
      memcpy(found.address, pDevice->getAddress().getNative(), sizeof(esp_bd_addr_t));
      strcpy(found.name, pDevice->getName().c_str());
      reply(EXTENDED_HW_CMD_FOUND_DEVICE, reinterpret_cast<uint8_t *>(&found), 1 + sizeof(esp_bd_addr_t) + strlen(found.name));
    } }))
  {
    Log.traceln("BTC: started scan");
  }
  else
  {
    Log.errorln("BTC: failed to start scan");
  }
}

void Bridge::btcDiscoveryUpdate()
{
}

void Bridge::btcDiscoveryExit()
{
  btSerial.discoverAsyncStop();
  // Give time for the scan to stop and seems that sometimes the device
  // names don't come thru
  delay(500);
}

/*
  BLE states
*/

void Bridge::bleDisconnectedEnter()
{
  Log.infoln("BLE: disconnected");
  startAdvertisingBLE();
}

void Bridge::bleDisconnectedUpdate()
{
}

void Bridge::bleDisconnectedExit()
{
  stopAdvertisingBLE();
}

void Bridge::bleConnectedEnter()
{
  Log.infoln("BLE: connected");
  clearAllPendingBTCData();

  if (useRigControl)
  {
    vfo = vfoUnknown;
    previousTNCMode = tncUnknown;

    // Make sure there is a radio to talk to
    if (btcStateMachine.isInState(btcConnectedState))
    {
      /*
      * We don't know what TNC mode the radio is in. If the KISS TNC is already on,
      * we can't send any commands to the radio, so we have to exit it first.
      */
      if (thd7x.isKISSMode())
      {
        Log.traceln("BLE: already in KISS mode");
        previousTNCMode = tncKISS;
        thd7x.exitKISS();
      }

      // Figure out which VFO is active for KISS mode
      tnc_mode_t mode;
      if (thd7x.getTNC(&vfo, &mode))
      {
        Log.infoln("BLE: vfo: %d", vfo);
        Log.infoln("BLE: mode: %d", mode);

        if (previousTNCMode == tncUnknown)
        {
          previousTNCMode = mode;
        }

        // As long as BLE is connected, we want the radio to be in KISS mode so application
        // can send data to the radio
        thd7x.setTNC(vfo, tncKISS);
      }
      else
      {
        Log.errorln("BLE: failed to get TNC mode and VFO!!!");
        vfo = vfoUnknown;
        previousTNCMode = tncUnknown;
      }
    }
  }
}

void Bridge::bleConnectedUpdate()
{
}

void Bridge::bleConnectedExit()
{
  if (useRigControl)
  {
    // Make sure there is a radio to talk to
    if (btcStateMachine.isInState(btcConnectedState))
    {
      if (previousTNCMode != tncKISS && previousTNCMode != tncUnknown && vfo != vfoUnknown)
      {
        Log.traceln("BLE: restoring initial KISS mode");

        // Always exit KISS mode first so we don't have to wait for a timeout
        thd7x.exitKISS();

        // Just to be sure
        if (thd7x.isKISSMode())
        {
          Log.warningln("BLE: still in KISS mode?");
          thd7x.exitKISS();
        }

        thd7x.setTNC(vfo, previousTNCMode);
      }
    }
  }
}