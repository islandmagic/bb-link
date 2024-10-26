#pragma once
#ifndef THD7x_H
#define THD7x_H

#ifndef EPOXY_DUINO
#include "BluetoothSerial.h"
#else
#include "MockBluetoothSerial.h"
#endif

enum vfo_t : int
{
  vfoA = 0x00,
  vfoB = 0x01,
  vfoUnknown = 0xFF
};

enum tnc_mode_t : int
{
  tncOff = 0x00,
  tncAPRS = 0x01,
  tncKISS = 0x02,
  tncUnknown = 0xFF
};

enum baud_rate_t : int
{
  baudRate1200 = 0x00,
  baudRate9600 = 0x01,
  baudRateUnknown = 0xFF
};

enum vfo_mode_t : int
{
  modeFM = 0x00,
  modeDV = 0x01,
  modeAM = 0x02,
  modeLSB = 0x03,
  modeUSB = 0x04,
  modeCW = 0x05,
  modeNFM = 0x06,
  modeDR = 0x07,
  modeWFM = 0x08,
  modeRCW = 0x09,
  modeUnknown = 0xFF
};

class THD7x
{
  public:
    THD7x(BluetoothSerial &btSerial);

    void setFrequency(vfo_t vfo, uint32_t frequency);
    bool getFrequency(vfo_t vfo, uint32_t *frequency);

    void setBaudRate(baud_rate_t baud_rate);
    bool getBaudRate(baud_rate_t *baud_rate);

    void setMode(vfo_t vfo, vfo_mode_t mode);
    bool getMode(vfo_t vfo, vfo_mode_t *mode);

    void setTNC(vfo_t vfo, tnc_mode_t mode);
    bool getTNC(vfo_t *vfo, tnc_mode_t *mode);

    bool getRadioId(char *radioId, int len);

    void exitKISS();
    bool isKISSMode();

    bool sendCmd(const char *command, char *response, size_t len, int retry=3);

  private:
    BluetoothSerial &btSerial;
};

#endif