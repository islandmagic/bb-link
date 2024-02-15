# See https://github.com/bxparks/EpoxyDuino for documentation about this
# Makefile to compile and run Arduino programs natively on Linux or MacOS.

APP_SRC_PATH := $(CURDIR)/../../src/bb-link
APP_CPP_FILES := $(wildcard $(APP_SRC_PATH)/THD7x.cpp)
OBJS += $(APP_CPP_FILES:%.cpp=%.o)

APP_NAME := THD7xTest
DEPS += $(APP_SRC_PATH)/MockBluetoothSerial.h
ARDUINO_LIBS := AUnit ArduinoLog EpoxyEsp32
EPOXY_CORE := EPOXY_CORE_ESP32
include ~/Documents/Arduino/libraries/EpoxyDuino/EpoxyDuino.mk
