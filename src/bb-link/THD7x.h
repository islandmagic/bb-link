#pragma once
#ifndef THD7x_H
#define THD7x_H

#include "BluetoothSerial.h"

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

class THD7x
{
  public:
    THD7x(BluetoothSerial btSerial);

    void setFrequency(vfo_t vfo, uint32_t frequency);
    bool getFrequency(vfo_t vfo, uint32_t *frequency);

    void setTNC(vfo_t vfo, tnc_mode_t mode);
    bool getTNC(vfo_t *vfo, tnc_mode_t *mode);

    bool getRadioId(char *radioId, int len);

    void exitKISS();
    bool isKISSMode();

  private:
    BluetoothSerial btSerial;
    bool sendCmd(const char *command, char *response, size_t len, int retry=3);
};

#endif