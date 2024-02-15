#line 2 "KISSInterceptorTest.ino"

#include <AUnit.h>
#include <ArduinoLog.h>
#include "../../src/bb-link/KISSInterceptor.h"

using aunit::TestRunner;

test(extractExtendedHardwareCommandUnknown)
{
  KISSInterceptor kissInterceptor;
  uint8_t frame[] = {0xC0, 0x00, 0xC0};  
  extended_hw_cmd_t cmd;
  assertFalse(kissInterceptor.extractExtendedHardwareCommand(frame, sizeof(frame), &cmd));
}

test(extractExtendedHardwareCommandSetFrequency)
{
  KISSInterceptor kissInterceptor;
  uint8_t frame[] = {0xC0, 0x06, 0xEA, 0x01, 0x02, 0x03, 0x04, 0xC0};  
  extended_hw_cmd_t cmd;
  assertTrue(kissInterceptor.extractExtendedHardwareCommand(frame, sizeof(frame), &cmd));
  assertEqual(extended_hw_set_frequency, cmd.action);
  assertEqual((uint32_t)0x01020304, cmd.data.uint32);
}

test(extractExtendedHardwareCommandRestoreFrequency)
{
  KISSInterceptor kissInterceptor;
  uint8_t frame[] = {0xC0, 0x06, 0xEB, 0xC0};  
  extended_hw_cmd_t cmd;
  assertTrue(kissInterceptor.extractExtendedHardwareCommand(frame, sizeof(frame), &cmd));
  assertEqual(extended_hw_restore_frequency, cmd.action);
}

test(extractExtendedHardwareCommandStartScan)
{
  KISSInterceptor kissInterceptor;
  uint8_t frame[] = {0xC0, 0x06, 0xEC, 0xC0};  
  extended_hw_cmd_t cmd;
  assertTrue(kissInterceptor.extractExtendedHardwareCommand(frame, sizeof(frame), &cmd));
  assertEqual(extended_hw_start_scan, cmd.action);
}

test(extractExtendedHardwareCommandStopScan)
{
  KISSInterceptor kissInterceptor;
  uint8_t frame[] = {0xC0, 0x06, 0xED, 0xC0};  
  extended_hw_cmd_t cmd;
  assertTrue(kissInterceptor.extractExtendedHardwareCommand(frame, sizeof(frame), &cmd));
  assertEqual(extended_hw_stop_scan, cmd.action);
}

test(extractExtendedHardwareCommandPairWithDevice)
{
  KISSInterceptor kissInterceptor;
  uint8_t frame[] = {0xC0, 0x06, 0xEF, 0x01, 0x02, 0x03, 0x10, 0x20, 0x30, 0xC0};  
  extended_hw_cmd_t cmd;
  assertTrue(kissInterceptor.extractExtendedHardwareCommand(frame, sizeof(frame), &cmd));
  assertEqual(extended_hw_pair_with_device, cmd.action);
  assertEqual(0x01, cmd.data.bytes[0]);
  assertEqual(0x02, cmd.data.bytes[1]);
  assertEqual(0x03, cmd.data.bytes[2]);
  assertEqual(0x10, cmd.data.bytes[3]);
  assertEqual(0x20, cmd.data.bytes[4]);
  assertEqual(0x30, cmd.data.bytes[5]);
}

test(extractExtendedHardwareCommandClearPairedDevice)
{
  KISSInterceptor kissInterceptor;
  uint8_t frame[] = {0xC0, 0x06, 0xF0, 0xC0};  
  extended_hw_cmd_t cmd;
  assertTrue(kissInterceptor.extractExtendedHardwareCommand(frame, sizeof(frame), &cmd));
  assertEqual(extended_hw_clear_paired_device, cmd.action);
}

test(extractExtendedHardwareCommandFirmwareVersion)
{
  KISSInterceptor kissInterceptor;
  uint8_t frame[] = {0xC0, 0x06, 0x28, 0xC0};  
  extended_hw_cmd_t cmd;
  assertTrue(kissInterceptor.extractExtendedHardwareCommand(frame, sizeof(frame), &cmd));
  assertEqual(extended_hw_firmware_version, cmd.action);
}

test(extractExtendedHardwareCommandCapabilities)
{
  KISSInterceptor kissInterceptor;
  uint8_t frame[] = {0xC0, 0x06, 0x7E, 0xC0};  
  extended_hw_cmd_t cmd;
  assertTrue(kissInterceptor.extractExtendedHardwareCommand(frame, sizeof(frame), &cmd));
  assertEqual(extended_hw_capabilities, cmd.action);
}

test(extractExtendedHardwareCommandApiVersion)
{
  KISSInterceptor kissInterceptor;
  uint8_t frame[] = {0xC0, 0x06, 0x7B, 0xC0};  
  extended_hw_cmd_t cmd;
  assertTrue(kissInterceptor.extractExtendedHardwareCommand(frame, sizeof(frame), &cmd));
  assertEqual(extended_hw_api_version, cmd.action);
}

test(extractExtendedHardwareCommandSetRigCtrl)
{
  KISSInterceptor kissInterceptor;
  uint8_t frame[] = {0xC0, 0x06, 0xF2, 0x01, 0xC0};  
  extended_hw_cmd_t cmd;
  assertTrue(kissInterceptor.extractExtendedHardwareCommand(frame, sizeof(frame), &cmd));
  assertEqual(extended_hw_set_rig_ctrl, cmd.action);
  assertEqual((uint8_t)0x01, cmd.data.uint8);

}

test(extractExtendedHardwareCommandFactoryReset)
{
  KISSInterceptor kissInterceptor;
  uint8_t frame[] = {0xC0, 0x06, 0xF3, 0xC0};  
  extended_hw_cmd_t cmd;
  assertTrue(kissInterceptor.extractExtendedHardwareCommand(frame, sizeof(frame), &cmd));
  assertEqual(extended_hw_factory_reset, cmd.action);
}

test(escape)
{
  KISSInterceptor kissInterceptor;
  uint8_t frame[] = {0x01, 0xC0, 0x03, 0xDB};
  uint8_t result[32];
  size_t resultSize = 32;
  assertTrue(kissInterceptor.escape(frame, sizeof(frame), result, &resultSize));
  assertEqual((size_t)8, resultSize);
  assertEqual(0xC0, result[0]);
  assertEqual(0x01, result[1]);
  assertEqual(0xDB, result[2]);
  assertEqual(0xDC, result[3]);
  assertEqual(0x03, result[4]);
  assertEqual(0xDB, result[5]);
  assertEqual(0xDD, result[6]);
  assertEqual(0xC0, result[7]);
}

test(unescape)
{
  KISSInterceptor kissInterceptor;
  uint8_t frame[] = {0xC0, 0x01, 0xDB, 0xDC, 0x03, 0xDB, 0xDD, 0xC0};
  uint8_t result[32];
  size_t resultSize = 32;
  assertTrue(kissInterceptor.unescape(frame, sizeof(frame), result, &resultSize));
  assertEqual((size_t)6, resultSize);
  assertEqual(0xC0, result[0]);
  assertEqual(0x01, result[1]);
  assertEqual(0xC0, result[2]);
  assertEqual(0x03, result[3]);
  assertEqual(0xDB, result[4]);
  assertEqual(0xC0, result[5]);
}

test(unescapeInvalidEscapeSequence)
{
  KISSInterceptor kissInterceptor;
  uint8_t frame[] = {0xC0, 0x01, 0xDB, 0xDC, 0x03, 0xDB, 0x00, 0xC0};
  uint8_t result[32];
  size_t resultSize = 32;
  assertFalse(kissInterceptor.unescape(frame, sizeof(frame), result, &resultSize));
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