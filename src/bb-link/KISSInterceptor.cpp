#include <ArduinoLog.h>
#include "KISSInterceptor.h"

static const uint8_t FEND = 0xC0;
static const uint8_t FESC = 0xDB;
static const uint8_t TFEND = 0xDC;
static const uint8_t TFESC = 0xDD;

KISSInterceptor::KISSInterceptor()
{
}

bool KISSInterceptor::extractExtendedHardwareCommand(uint8_t *buffer, size_t size, extended_hw_cmd_t *cmd)
{
  // Look for frame start
  for (int i = 0; i < size; i++)
  {
    if (buffer[i] == FEND)
    {
      // Fast check for hardware command
      if (buffer[i + 1] != CMD_HARDWARE)
      {
        break;
      }

      Log.traceln("Found SET HW KISS frame start at index %d", i);
      // Look for frame end
      for (int j = i + 1; j < size; j++)
      {
        if (buffer[j] == FEND)
        {
          Log.traceln("Found frame end at index %d", j);

          // Copy frame to new buffer
          uint8_t frame[j - i + 1];
          memcpy(frame, &buffer[i], j - i + 1);

          // Unescape frame
          uint8_t unescapedBuffer[size];
          size_t unescapedSize;
          if (!unescape(&buffer[i], size - i, unescapedBuffer, &unescapedSize))
          {
            Log.errorln("Failed to unescape frame");
            return false;
          }

          Log.traceln("Found valid hardware cmd");

          char hexString[3 * unescapedSize + 1];
          for (int k = 0; k < unescapedSize; k++)
          {
            sprintf(&hexString[3 * k], "%02X ", unescapedBuffer[k]);
          }

          // Display hex content of buffer
          Log.traceln("Frame: %s", hexString);

          switch (unescapedBuffer[i + 2])
          {
          case EXTENDED_HW_CMD_SET_FREQUENCY:
          {
            uint32_t frequency = (unescapedBuffer[i + 3] << 24) | (unescapedBuffer[i + 4] << 16) |
                                 (unescapedBuffer[i + 5] << 8) | unescapedBuffer[i + 6];
            Log.infoln("Set frequency cmd: %d", frequency);
            cmd->action = extended_hw_set_frequency;
            cmd->data.uint32 = frequency;
            return true;
          }

          case EXTENDED_HW_CMD_RESTORE_FREQUENCY:
            Log.infoln("Restore frequency cmd");
            cmd->action = extended_hw_restore_frequency;
            return true;

          case EXTENDED_HW_CMD_SET_BAUD_RATE:
          {
            uint8_t baud_rate = unescapedBuffer[i + 3];
            Log.infoln("Set baud rate cmd: %d", baud_rate);
            cmd->action = extended_hw_set_baud_rate;
            cmd->data.uint8 = baud_rate;
            return true;
          }

          case EXTENDED_HW_CMD_START_SCAN:
            Log.infoln("Start scan cmd");
            cmd->action = extended_hw_start_scan;
            return true;

          case EXTENDED_HW_CMD_STOP_SCAN:
            Log.infoln("Stop scan cmd");
            cmd->action = extended_hw_stop_scan;
            return true;

          case EXTENDED_HW_CMD_PAIR_WITH_DEVICE:
            Log.infoln("Pair with device cmd");
            cmd->action = extended_hw_pair_with_device;
            memcpy(cmd->data.bytes, &unescapedBuffer[i + 3], ESP_BD_ADDR_LEN);
            return true;

          case EXTENDED_HW_CMD_CLEAR_PAIRED_DEVICE:
            Log.infoln("Clear paired device cmd");
            cmd->action = extended_hw_clear_paired_device;
            return true;

          case EXTENDED_HW_CMD_FIRMWARE_VERSION:
            Log.infoln("Firmware version cmd");
            cmd->action = extended_hw_firmware_version;
            return true;

          case EXTENDED_HW_CMD_CAPABILITIES:
            Log.infoln("Capabilities cmd");
            cmd->action = extended_hw_capabilities;
            return true;

          case EXTENDED_HW_CMD_API_VERSION:
            Log.infoln("API version cmd");
            cmd->action = extended_hw_api_version;
            return true;

          case EXTENDED_HW_CMD_GET_PAIRED_DEVICE:
            Log.infoln("Get paired device cmd");
            cmd->action = extended_hw_get_paired_device;
            return true;

          case EXTENDED_HW_CMD_SET_RIG_CTRL:
            Log.infoln("Set rig control cmd");
            cmd->action = extended_hw_set_rig_ctrl;
            cmd->data.uint8 = unescapedBuffer[i + 3];
            return true;

          case EXTENDED_HW_CMD_FACTORY_RESET:
            Log.infoln("Factory reset cmd");
            cmd->action = extended_hw_factory_reset;
            return true;

          default:
            Log.errorln("Unknown hardware cmd");
            return false;
          }
        }
      }
    }
  }

  return false;
}

bool KISSInterceptor::unescape(uint8_t *buffer, size_t size, uint8_t *result, size_t *resultSize)
{
  uint8_t *src = buffer;
  uint8_t *dst = result;
  while(src < buffer + size)
  {
    if (*src == FESC)
    {
      src++;
      if (*src == TFEND)
      {
        *dst = FEND;
      }
      else if (*src == TFESC)
      {
        *dst = FESC;
      }
      else
      {
        // Invalid escape sequence
        return false;
      }
    }
    else
    {
      *dst = *src;
    }
    src++;
    dst++;
  }
  *resultSize = dst - result;
  return true;
}

// Output buffer is assumed to be large enough to hold the escaped data
bool KISSInterceptor::escape(uint8_t *buffer, size_t size, uint8_t *result, size_t *resultSize)
{
  uint8_t *src = buffer;
  uint8_t *dst = result;
  size_t dstSize = *resultSize;

  if (dstSize < size * 2 + 2)
  {
    return false;
  }

  *dst = FEND;
  dst++;

  for (int i = 0; i < size; i++)
  {
    if (*src == FEND)
    {
      *dst = FESC;
      dst++;
      *dst = TFEND;
    }
    else if (*src == FESC)
    {
      *dst = FESC;
      dst++;
      *dst = TFESC;
    }
    else
    {
      *dst = *src;
    }
    src++;
    dst++;
  }

  *dst = FEND;
  dst++;

  *resultSize = dst - result;
  return true;
}