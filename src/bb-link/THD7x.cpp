#include <ArduinoLog.h>
#include "THD7x.h"

/*
  Commands: https://github.com/LA3QMA/TH-D74-Kenwood
*/
#define CMD_BUFFER_SIZE 32

THD7x::THD7x(BluetoothSerial &btSerial)
  : btSerial(btSerial) {
}

/*
  https://github.com/LA3QMA/TH-D74-Kenwood/blob/master/commands/FQ.md
*/

void THD7x::setFrequency(vfo_t vfo, uint32_t frequency) {
  char command[CMD_BUFFER_SIZE];
  sprintf(command, "FQ %d,%010d", vfo, frequency);
  char response[CMD_BUFFER_SIZE];
  sendCmd(command, response, CMD_BUFFER_SIZE);
}

bool THD7x::getFrequency(vfo_t vfo, uint32_t *frequency) {
  char command[CMD_BUFFER_SIZE];
  sprintf(command, "FQ %1d", vfo);
  char response[CMD_BUFFER_SIZE];

  if (sendCmd(command, response, CMD_BUFFER_SIZE)) {
    // convert 10 digit ascii string value to int
    *frequency = atoi(&response[5]);
    return true;
  }
  return false;
}

/*
  https://github.com/LA3QMA/TH-D74-Kenwood/blob/master/commands/TN.md
*/

void THD7x::setTNC(vfo_t vfo, tnc_mode_t mode) {
  char command[CMD_BUFFER_SIZE];
  sprintf(command, "TN %d,%d", mode, vfo);
  char response[CMD_BUFFER_SIZE];
  sendCmd(command, response, CMD_BUFFER_SIZE);
}

bool THD7x::getTNC(vfo_t *vfo, tnc_mode_t *mode) {
  char response[CMD_BUFFER_SIZE];
  if (sendCmd("TN", response, CMD_BUFFER_SIZE)) {
    // convert ascii char to int
    *vfo = static_cast<vfo_t>(response[5] - '0');
    *mode = static_cast<tnc_mode_t>(response[3] - '0');
    return true;
  }
  return false;
}

/*
  https://github.com/LA3QMA/TH-D74-Kenwood/blob/master/commands/ID.md
*/
bool THD7x::getRadioId(char *radioId, int len) {
  char response[CMD_BUFFER_SIZE];
  if (sendCmd("ID", response, CMD_BUFFER_SIZE)) {
    strncpy(radioId, (const char *)&response[3], len);
    return true;
  }
  return false;
}

void THD7x::exitKISS() {
  // https://www.ax25.net/kiss.aspx
  const unsigned char exitKISSSequence[] = { 0xC0, 0xFF, 0xC0 };
  Log.traceln("(adapter) > BTC: KISS exit sequence");
  for (unsigned char byte : exitKISSSequence) {
    btSerial.write(byte);
  }
  btSerial.flush();
}

bool THD7x::isKISSMode() {
  // Send a simple command to see if we get a response. BT queries the
  // bluetooth mode, which should always be on since we're connected.
  char response[CMD_BUFFER_SIZE];
  return !sendCmd("BT", response, CMD_BUFFER_SIZE);
}

bool THD7x::sendCmd(const char *cmd, char *response, size_t responseLen, int retry) {
  Log.traceln("(adapter) > BTC: %s", cmd);
  btSerial.flush();
  btSerial.print(cmd);
  btSerial.print("\r");
  btSerial.flush();
  btSerial.setTimeout(1000);
  size_t lenRead = btSerial.readBytesUntil('\r', response, responseLen - 1);
  if (lenRead == 0) {
    Log.infoln("No response from command %s", cmd);
    return false;
  }
  if (response[0] == '?') {
    Log.warningln("Error response from command %s", cmd);
    if (retry--) {
      Log.infoln("Retry attempt left %i", retry);
      delay(200);
      return sendCmd(cmd, response, responseLen, retry);
    } else {
      Log.warningln("No more retries");
      return false;
    }
  }
  response[lenRead] = '\0';
  Log.traceln("(adapter) < BTC: %s", response);
  return (response[0] == cmd[0] && response[1] == cmd[1]);
}
