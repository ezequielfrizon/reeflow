#include "drivers/io/arduino_hardware_io.h"

#include <Arduino.h>

namespace reeflow::drivers {
namespace {

int toArduinoMode(GpioMode mode) {
  switch (mode) {
    case GpioMode::kInput:
      return INPUT;
    case GpioMode::kOutput:
      return OUTPUT;
  }

  return INPUT;
}

int toArduinoLevel(GpioLevel level) {
  switch (level) {
    case GpioLevel::kLow:
      return LOW;
    case GpioLevel::kHigh:
      return HIGH;
  }

  return LOW;
}

class ArduinoGpioPort final : public GpioPort {
 public:
  void setMode(uint8_t gpio, GpioMode mode) override {
    pinMode(gpio, toArduinoMode(mode));
  }

  void write(uint8_t gpio, GpioLevel level) override {
    digitalWrite(gpio, toArduinoLevel(level));
  }
};

class ArduinoPwmLedcPort final : public PwmLedcPort {
 public:
  void setup(uint8_t ledcChannel, uint32_t frequencyHz,
             uint8_t resolutionBits) override {
    ledcSetup(ledcChannel, frequencyHz, resolutionBits);
  }

  void attachPin(uint8_t gpio, uint8_t ledcChannel) override {
    ledcAttachPin(gpio, ledcChannel);
  }

  void write(uint8_t ledcChannel, uint16_t duty) override {
    ledcWrite(ledcChannel, duty);
  }
};

class ArduinoBringupTimer final : public BringupTimer {
 public:
  void delayMs(unsigned long milliseconds) override {
    delay(milliseconds);
  }
};

ArduinoGpioPort gpioPort;
ArduinoPwmLedcPort pwmPort;
ArduinoBringupTimer bringupTimer;

}  // namespace

GpioPort& arduinoGpioPort() {
  return gpioPort;
}

PwmLedcPort& arduinoPwmLedcPort() {
  return pwmPort;
}

BringupTimer& arduinoBringupTimer() {
  return bringupTimer;
}

}  // namespace reeflow::drivers
