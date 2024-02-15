#pragma once
#ifndef KISSINTERCEPTOR_H
#define KISSINTERCEPTOR_H

#include "Arduino.h"
#ifndef EPOXY_DUINO
#include "BLEDevice.h"
#else
#include "MockBLEDevice.h"
#endif

static const uint8_t CMD_HARDWARE = 0x06;

static const uint8_t EXTENDED_HW_CMD_FIRMWARE_VERSION = 0x28;
static const uint8_t EXTENDED_HW_CMD_CAPABILITIES = 0x7E;
static const uint8_t EXTENDED_HW_CMD_API_VERSION = 0x7B;

static const uint8_t EXTENDED_HW_CMD_SET_FREQUENCY = 0xEA;
static const uint8_t EXTENDED_HW_CMD_RESTORE_FREQUENCY = 0xEB;

static const uint8_t EXTENDED_HW_CMD_START_SCAN = 0xEC;
static const uint8_t EXTENDED_HW_CMD_STOP_SCAN = 0xED;
static const uint8_t EXTENDED_HW_CMD_FOUND_DEVICE = 0xEE;
static const uint8_t EXTENDED_HW_CMD_PAIR_WITH_DEVICE = 0xEF;
static const uint8_t EXTENDED_HW_CMD_CLEAR_PAIRED_DEVICE = 0xF0;
static const uint8_t EXTENDED_HW_CMD_GET_PAIRED_DEVICE = 0xF1;
static const uint8_t EXTENDED_HW_CMD_SET_RIG_CTRL = 0xF2;
static const uint8_t EXTENDED_HW_CMD_FACTORY_RESET = 0xF3;

enum extended_hw_action_t : uint8_t
{
  extended_hw_set_frequency = 0x00,
  extended_hw_restore_frequency = 0x01,
  extended_hw_start_scan = 0x02,
  extended_hw_stop_scan = 0x03,
  extended_hw_pair_with_device = 0x04,
  extended_hw_clear_paired_device = 0x05,
  extended_hw_found_device = 0x06,
  extended_hw_firmware_version = 0x07,
  extended_hw_capabilities = 0x08,
  extended_hw_api_version = 0x09,
  extended_hw_get_paired_device = 0x0A,
  extended_hw_set_rig_ctrl = 0x0B,
  extended_hw_factory_reset = 0x0C,
  extended_hw_unknown = 0xFF
};

typedef union {
  uint32_t uint32;  // 32-bit access
  uint8_t bytes[8];   // array access
  uint8_t uint8;
} data_union_t;

struct extended_hw_cmd_t {
  extended_hw_action_t action;
  data_union_t data;
};

struct found_device_t
{
  uint8_t connected;
  esp_bd_addr_t address;
  char name[32];
};
class KISSInterceptor
{
public:
  KISSInterceptor();
  bool extractExtendedHardwareCommand(uint8_t *buffer, size_t size, extended_hw_cmd_t *cmd);
  bool escape(uint8_t *buffer, size_t size, uint8_t *result, size_t *resultSize);
  bool unescape(uint8_t *buffer, size_t size, uint8_t *unescapedBuffer, size_t *unescapedSize);

private:
};

#endif