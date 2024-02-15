#ifdef EPOXY_DUINO

class MockBluetoothSerial
{
private:
    char readBuffer[256];
    size_t readBufferLength = 0;
    char writeBuffer[256];
    size_t writeBufferLength = 0;

public:
    MockBluetoothSerial()
    {
    }

    void begin(const char * /* name */)
    {
    }

    int available()
    {
        return 0;
    }

    size_t write(uint8_t byte)
    {
        writeBuffer[writeBufferLength] = byte;
        writeBufferLength++;
        return 1;
    }

    int read()
    {
        return -1; // Example value, -1 indicates no data
    }

    int readBytesUntil(char /*terminator*/, char *buffer, size_t length)
    {
        if (readBufferLength > 0 && length > readBufferLength)
        {
            memcpy(buffer, readBuffer, readBufferLength);
            int result = readBufferLength;
            readBufferLength = 0;
            return result;
        }
        else
        {
            return 0;
        }
    }

    void flush()
    {
    }

    void setTimeout(unsigned long /*timeout*/)
    {
    }

    void print(const char *message)
    {
        memcpy(writeBuffer+writeBufferLength, message, strlen(message));
        writeBufferLength += strlen(message);
    }

    size_t getWriteBuffer(char *buffer, size_t length)
    {
        if (writeBufferLength > length)
        {
            return -1;
        }
        memcpy(buffer, writeBuffer, writeBufferLength);
        int result = writeBufferLength;
        writeBufferLength = 0;
        return result;
    }

    void setMockReadValue(const char *value, size_t length)
    {
        memcpy(readBuffer, value, length);
        readBufferLength = length;
    }

    void setMockReadValue(const char *value)
    {
        setMockReadValue(value, strlen(value));
    }
};

#define BluetoothSerial MockBluetoothSerial

#endif