#pragma once
#ifndef BRIDGE_H
#define BRIDGE_H

#include "BluetoothSerial.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <Preferences.h>
#include <ArduinoQueue.h>

#include "THD7x.h"
#include "FiniteStateMachine.h"
#include "KISSInterceptor.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

const uint16_t API_VERSION = 0x0100; // Used to check compatibility between adapter and config app
const char PREFERENCES_NAMESPACE[] = "bb-link";

const uint16_t CAP_RIG_CTRL = 0x0010;
const uint16_t CAP_FIRMWARE_VERSION = 0x0800;

DECLARE_STATE(BLEState);
DECLARE_STATE(BTCState);

#define PAIR_MAX_DEVICES 10
#define MAX_BTC_DEVICE_NAME_LEN 248

class Bridge : public BLECharacteristicCallbacks, public BLEServerCallbacks
{
public:
  Bridge(String adapterName);
  bool init();
  void perform();
  bool isReady();
  bool btcConnected();
  bool btcDiscovery();
  bool isTx();
  bool isRx();
  void clearPairedDevices();
  void disconnect();
  void factoryReset();
  BLEServer * getBLEServer();

  BluetoothSerial btSerial;

private:
  String adapterName;
  BTScanResults *btDeviceList;
  BLEServer *pBLEServer;

  BLECharacteristic *pTx;
  BLECharacteristic *pRx;
  uint16_t mtuSize = 512;

  THD7x thd7x = THD7x(btSerial);
  vfo_t vfo = vfoUnknown;
  uint32_t previousFrequency = 0;
  tnc_mode_t previousTNCMode = tncUnknown;

  KISSInterceptor kissInterceptor = KISSInterceptor();

  BLEState bleDisconnectedState;
  BLEState bleConnectedState;
  FSMT<BLEState> bleStateMachine;
  BTCState btcDisconnectedState;
  BTCState btcConnectedState;
  BTCState btcDiscoveryState;
  FSMT<BTCState> btcStateMachine;
  unsigned int txLingerUntil = 0;
  unsigned int rxLingerUntil = 0;
  unsigned int lastDeviceLookup = 0;

  uint8_t pairedDeviceBtAddr[PAIR_MAX_DEVICES][ESP_BD_ADDR_LEN];
  uint8_t remoteAddress[ESP_BD_ADDR_LEN];
  char remoteName[MAX_BTC_DEVICE_NAME_LEN];
  bool connectToPairedDevice = false;
  bool useRigControl = true;

  Preferences preferences;
  ArduinoQueue<extended_hw_cmd_t> cmdQueue;
  bool processingCmdQueue = false;

  bool initBTC();
  bool initBLE();
  void startAdvertisingBLE();
  void stopAdvertisingBLE();
  void clearAllPendingBTCData();
  void setTxLinger(int linger);
  void setRxLinger(int linger);
  void lookUpLastPairedDevice();
  void processExtendedHardwareCommand(extended_hw_cmd_t *cmd);
  void clearStoredPairedDeviceInfo();
  void clearRemoteDeviceInfo();

  void reply8(uint8_t cmd, uint8_t data);
  void reply16(uint8_t cmd, uint16_t data);
  void reply(uint8_t cmd, uint8_t *data, size_t size);
  void reply(uint8_t *response, size_t size);

  void onRead(BLECharacteristic *pCharacteristic);
  void onWrite(BLECharacteristic *pCharacteristic);

  void onBTConfirmRequestCallback(uint32_t numVal);
  void onBTAuthCompleteCallback(bool success);

  void onMtuChanged(BLEServer *pServer, esp_ble_gatts_cb_param_t *param);
  void onConnect(BLEServer *pServer);
  void onDisconnect(BLEServer *pServer);  

  void bleDisconnectedEnter();
  void bleDisconnectedUpdate();
  void bleDisconnectedExit();
  void bleConnectedEnter();
  void bleConnectedUpdate();
  void bleConnectedExit();

  void btcDisconnectedEnter();
  void btcDisconnectedUpdate();
  void btcDisconnectedExit();
  void btcConnectedEnter();
  void btcConnectedUpdate();
  void btcConnectedExit();  
  void btcDiscoveryEnter();
  void btcDiscoveryUpdate();
  void btcDiscoveryExit();
};

#endif