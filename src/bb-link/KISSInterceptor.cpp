#include "KISSInterceptor.h"

static const char FEND = 0xC0;
static const char FESC = 0xDB;
static const char TFEND = 0xDC;
static const char TFESC = 0xDD;

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

      Serial.printf("Found SET HW KISS frame start at index %d\n", i);

      // Look for frame end
      for (int j = i + 1; j < size; j++)
      {
        if (buffer[j] == FEND)
        {
          Serial.printf("Found frame end at index %d\n", j);

          // Copy frame to new buffer
          uint8_t frame[j - i + 1];
          memcpy(frame, &buffer[i], j - i + 1);

          // Unescape frame
          uint8_t unescapedBuffer[size];
          size_t unescapedSize;
          if (!unescape(&buffer[i], size - i, unescapedBuffer, &unescapedSize))
          {
            Serial.printf("Failed to unescape frame\n");
            return false;
          }

          Serial.printf("Found valid hardware command\n");

          // Display hex content of buffer
          Serial.printf("Frame: ");
          for (int k = 0; k < unescapedSize; k++)
          {
            Serial.printf("%02X ", unescapedBuffer[k]);
          }
          Serial.printf("\n");

          switch (unescapedBuffer[i + 2])
          {
          case EXTENDED_HW_CMD_SET_FREQUENCY:
          {
            Serial.printf("Found set frequency command\n");
            uint32_t frequency = (unescapedBuffer[i + 3] << 24) | (unescapedBuffer[i + 4] << 16) |
                                 (unescapedBuffer[i + 5] << 8) | unescapedBuffer[i + 6];
            Serial.printf("Frequency: %d\n", frequency);
            cmd->action = extended_hw_set_frequency;
            cmd->data.uint32 = frequency;
            return true;
          }
          case EXTENDED_HW_CMD_RESTORE_FREQUENCY:
            Serial.printf("Found restore frequency command\n");
            cmd->action = extended_hw_restore_frequency;
            return true;

          case EXTENDED_HW_CMD_START_SCAN:
            Serial.printf("Found start scan command\n");
            cmd->action = extended_hw_start_scan;
            return true;

          case EXTENDED_HW_CMD_STOP_SCAN:
            Serial.printf("Found stop scan command\n");
            cmd->action = extended_hw_stop_scan;
            return true;

          case EXTENDED_HW_CMD_PAIR_WITH_DEVICE:
            Serial.printf("Found pair with device command\n");
            cmd->action = extended_hw_pair_with_device;
            memcpy(cmd->data.bytes, &unescapedBuffer[i + 3], ESP_BD_ADDR_LEN);
            return true;

          case EXTENDED_HW_CMD_CLEAR_PAIRED_DEVICE:
            Serial.printf("Found clear paired device command\n");
            cmd->action = extended_hw_clear_paired_device;
            return true;

          case EXTENDED_HW_CMD_FIRMWARE_VERSION:
            Serial.printf("Found firmware version command\n");
            cmd->action = extended_hw_firmware_version;
            return true;

          case EXTENDED_HW_CMD_CAPABILITIES:
            Serial.printf("Found capabilities command\n");
            cmd->action = extended_hw_capabilities;
            return true;

          case EXTENDED_HW_CMD_API_VERSION:
            Serial.printf("Found API version command\n");
            cmd->action = extended_hw_api_version;
            return true;

          case EXTENDED_HW_CMD_GET_PAIRED_DEVICE:
            Serial.printf("Found get paired device command\n");
            cmd->action = extended_hw_get_paired_device;
            return true;

          case EXTENDED_HW_CMD_SET_RIG_CTRL:
            Serial.printf("Found set rig control command\n");
            cmd->action = extended_hw_set_rig_ctrl;
            cmd->data.uint8 = unescapedBuffer[i + 3];
            return true;

          case EXTENDED_HW_CMD_FACTORY_RESET:
            Serial.printf("Found factory reset command\n");
            cmd->action = extended_hw_factory_reset;
            return true;

          default:
            Serial.printf("Found unknown hardware command\n");
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
  for (int i = 0; i < size; i++)
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