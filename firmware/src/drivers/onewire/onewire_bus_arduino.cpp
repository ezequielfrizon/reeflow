#include "drivers/onewire/onewire_bus.h"

#include <Arduino.h>

namespace reeflow::drivers {
namespace {

class ArduinoOneWireBus final : public OneWireBus {
 public:
  bool reset(uint8_t gpio) override {
    activeGpio = gpio;

    pinMode(activeGpio, OUTPUT);
    digitalWrite(activeGpio, LOW);
    delayMicroseconds(480);
    pinMode(activeGpio, INPUT_PULLUP);
    delayMicroseconds(70);

    const bool presenceDetected = digitalRead(activeGpio) == LOW;
    delayMicroseconds(410);
    return presenceDetected;
  }

  void writeByte(uint8_t value) override {
    for (uint8_t bit = 0; bit < 8; ++bit) {
      writeBit((value & 0x01U) != 0);
      value >>= 1;
    }
  }

  uint8_t readByte() override {
    uint8_t value = 0;

    for (uint8_t bit = 0; bit < 8; ++bit) {
      if (readBit()) {
        value |= static_cast<uint8_t>(1U << bit);
      }
    }

    return value;
  }

 private:
  void writeBit(bool value) {
    pinMode(activeGpio, OUTPUT);
    digitalWrite(activeGpio, LOW);

    if (value) {
      delayMicroseconds(6);
      pinMode(activeGpio, INPUT_PULLUP);
      delayMicroseconds(64);
      return;
    }

    delayMicroseconds(60);
    pinMode(activeGpio, INPUT_PULLUP);
    delayMicroseconds(10);
  }

  bool readBit() {
    pinMode(activeGpio, OUTPUT);
    digitalWrite(activeGpio, LOW);
    delayMicroseconds(6);
    pinMode(activeGpio, INPUT_PULLUP);
    delayMicroseconds(9);
    const bool value = digitalRead(activeGpio) == HIGH;
    delayMicroseconds(55);
    return value;
  }

  uint8_t activeGpio = 0;
};

ArduinoOneWireBus oneWireBus;

}  // namespace

OneWireBus& arduinoOneWireBus() {
  return oneWireBus;
}

}  // namespace reeflow::drivers
