#line 2 "THD7xTest.ino"

#include <AUnit.h>
#include <ArduinoLog.h>
#include "../../src/bb-link/THD7x.h"

using aunit::TestRunner;

test(setFrequency)
{
  BluetoothSerial mockBluetoothSerial;
  THD7x thd7x = THD7x(mockBluetoothSerial);
  thd7x.setFrequency(vfoA, 145000000);
  char buffer[32];
  int length = mockBluetoothSerial.getWriteBuffer(buffer, 32);
  assertEqual(16, length);
  buffer[length] = '\0';
  assertEqual("FQ 0,0145000000\r", buffer);
}

test(getFrequency)
{
  BluetoothSerial mockBluetoothSerial;
  THD7x thd7x = THD7x(mockBluetoothSerial);
  uint32_t frequency;
  mockBluetoothSerial.setMockReadValue("FQ 0,0145000000");
  assertTrue(thd7x.getFrequency(vfoA, &frequency));
  assertEqual((uint32_t)145000000L, frequency);
}

test(setBaudRate)
{
  BluetoothSerial mockBluetoothSerial;
  THD7x thd7x = THD7x(mockBluetoothSerial);
  thd7x.setBaudRate(baudRate9600);
  char buffer[32];
  int length = mockBluetoothSerial.getWriteBuffer(buffer, 32);
  assertEqual(5, length);
  buffer[length] = '\0'; 
  assertEqual("AS 1\r", buffer);
}

test(getBaudRate)
{
  BluetoothSerial mockBluetoothSerial;
  THD7x thd7x = THD7x(mockBluetoothSerial);
  baud_rate_t baud_rate;
  mockBluetoothSerial.setMockReadValue("AS 1");
  assertTrue(thd7x.getBaudRate(&baud_rate));
  assertEqual(baudRate9600, baud_rate);
}

test(setTNC)
{
  BluetoothSerial mockBluetoothSerial;
  THD7x thd7x = THD7x(mockBluetoothSerial);
  thd7x.setTNC(vfoA, tncKISS);
  char buffer[32];
  int length = mockBluetoothSerial.getWriteBuffer(buffer, 32);
  assertEqual(7, length);
  buffer[length] = '\0'; 
  assertEqual("TN 2,0\r", buffer);
}

test(getTNC)
{
  BluetoothSerial mockBluetoothSerial;
  THD7x thd7x = THD7x(mockBluetoothSerial);
  vfo_t vfo;
  tnc_mode_t mode;
  mockBluetoothSerial.setMockReadValue("TN 2,0");
  assertTrue(thd7x.getTNC(&vfo, &mode));
  assertEqual(vfoA, vfo);
  assertEqual(tncKISS, mode);
}

test(setMode)
{
  BluetoothSerial mockBluetoothSerial;
  THD7x thd7x = THD7x(mockBluetoothSerial);
  thd7x.setMode(vfoA, modeFM);
  char buffer[32];
  int length = mockBluetoothSerial.getWriteBuffer(buffer, 32);
  assertEqual(7, length);
  buffer[length] = '\0'; 
  assertEqual("MD 0,0\r", buffer);
}

test(getMode)
{
  BluetoothSerial mockBluetoothSerial;
  THD7x thd7x = THD7x(mockBluetoothSerial);
  vfo_mode_t mode;
  mockBluetoothSerial.setMockReadValue("MD 1,5");
  assertTrue(thd7x.getMode(vfoB, &mode));
  assertEqual(modeCW, mode);
}

test(getRadioId)
{
  BluetoothSerial mockBluetoothSerial;
  THD7x thd7x = THD7x(mockBluetoothSerial);
  char radioId[32];
  mockBluetoothSerial.setMockReadValue("ID TH-D74");
  assertTrue(thd7x.getRadioId(radioId, 32));
  assertEqual("TH-D74", radioId);
}

test(exitKISS)
{
  BluetoothSerial mockBluetoothSerial;
  THD7x thd7x = THD7x(mockBluetoothSerial);
  char buffer[32];
  thd7x.exitKISS();
  int length = mockBluetoothSerial.getWriteBuffer(buffer, 32);
  assertEqual(3, length);
  assertEqual(0xccffcc, buffer);
}

test(isKISSMode)
{
  BluetoothSerial mockBluetoothSerial;
  THD7x thd7x = THD7x(mockBluetoothSerial);
  mockBluetoothSerial.setMockReadValue("?");
  assertTrue(thd7x.isKISSMode());
}

test(sendCmdSuccess)
{
  BluetoothSerial mockBluetoothSerial;
  THD7x thd7x = THD7x(mockBluetoothSerial);
  char buffer[32];
  mockBluetoothSerial.setMockReadValue("TE 456");
  assertTrue(thd7x.sendCmd("TE 123", buffer, 32));
  assertEqual("TE 456", buffer);
}

test(sendCmdNoResponse)
{
  BluetoothSerial mockBluetoothSerial;
  THD7x thd7x = THD7x(mockBluetoothSerial);
  char buffer[32];
  assertFalse(thd7x.sendCmd("TE 123", buffer, 32));
}

test(sendCmdRetry)
{
  BluetoothSerial mockBluetoothSerial;
  THD7x thd7x = THD7x(mockBluetoothSerial);
  char buffer[32];
  mockBluetoothSerial.setMockReadValue("?");
  assertFalse(thd7x.sendCmd("TE 123", buffer, 32, 3));
  char wbuffer[256];
  int length = mockBluetoothSerial.getWriteBuffer(wbuffer, 256);
  wbuffer[length] = '\0';
  assertEqual(14, length);
  // The mock read value reset after being read, so after the first retry, the buffer is empty
  // and it's detected as a no respone timeout situation and no more retries are attempted
  assertEqual("TE 123\rTE 123\r", wbuffer);
}

void setup()
{
  Serial.begin(115200);
  while (!Serial);
  delay(1000);
  Log.begin(LOG_LEVEL_SILENT, &Serial);
}

void loop()
{
  TestRunner::run();
}