#include "drivers/sensors/ds18b20/ds18b20_temperature_sensor.h"

#include <stddef.h>
#include <string.h>

#include "contracts/hardware_pins.h"

namespace reeflow::drivers::sensors {
namespace {

constexpr uint8_t kOneWireSkipRom = 0xCC;
constexpr uint8_t kDs18b20ConvertTemperature = 0x44;
constexpr uint8_t kDs18b20ReadScratchpad = 0xBE;
constexpr size_t kScratchpadSize = 9;

uint8_t updateDallasCrc(uint8_t crc, uint8_t value) {
  for (uint8_t bit = 0; bit < 8; ++bit) {
    const uint8_t mix = (crc ^ value) & 0x01U;
    crc >>= 1;
    if (mix != 0) {
      crc ^= 0x8CU;
    }
    value >>= 1;
  }

  return crc;
}

bool hasValidDallasCrc(const uint8_t* scratchpad) {
  uint8_t crc = 0;

  for (size_t index = 0; index < kScratchpadSize - 1; ++index) {
    crc = updateDallasCrc(crc, scratchpad[index]);
  }

  return crc == scratchpad[kScratchpadSize - 1];
}

float scratchpadTemperatureCelsius(const uint8_t* scratchpad) {
  const int16_t rawTemperature =
      static_cast<int16_t>((static_cast<uint16_t>(scratchpad[1]) << 8) |
                           scratchpad[0]);
  return static_cast<float>(rawTemperature) / 16.0F;
}

bool elapsedAtLeast(uint32_t now, uint32_t startedAt, uint32_t interval) {
  return static_cast<uint32_t>(now - startedAt) >= interval;
}

bool pinNameIs(const reeflow::contracts::HardwarePin& pin, const char* name) {
  return strcmp(pin.canonicalName, name) == 0;
}

}  // namespace

uint8_t ds18b20TemperatureSensorGpio() {
  for (size_t index = 0; index < reeflow::contracts::kHardwareV1PinCount;
       ++index) {
    const reeflow::contracts::HardwarePin& pin =
        reeflow::contracts::kHardwareV1Pins[index];
    if (pin.role == reeflow::contracts::HardwarePinRole::kSensorBus &&
        pinNameIs(pin, "DS18B20")) {
      return pin.gpio;
    }
  }

  return 0;
}

Ds18b20TemperatureSensor::Ds18b20TemperatureSensor(
    OneWireBus& bus, const core::platform::TimeSource& timeSource,
    uint32_t conversionWaitMillis, uint8_t gpio)
    : bus_(bus),
      timeSource_(timeSource),
      conversionWaitMillis_(conversionWaitMillis),
      gpio_(gpio),
      conversionPending_(false),
      conversionStartedAt_(0) {}

modules::temperature::TemperatureSensorReading
Ds18b20TemperatureSensor::readTemperature() {
  if (!conversionPending_) {
    return startConversion();
  }

  const uint32_t now = timeSource_.uptimeMillis();
  if (!elapsedAtLeast(now, conversionStartedAt_, conversionWaitMillis_)) {
    return modules::temperature::makeTemperatureConversionPending();
  }

  modules::temperature::TemperatureSensorReading reading =
      readCompletedConversion();
  conversionPending_ = false;

  startNextConversionIfPresent();
  return reading;
}

uint8_t Ds18b20TemperatureSensor::gpio() const {
  return gpio_;
}

modules::temperature::TemperatureSensorReading
Ds18b20TemperatureSensor::startConversion() {
  if (!bus_.reset(gpio_)) {
    conversionPending_ = false;
    return modules::temperature::makeSensorNotFoundReading();
  }

  bus_.writeByte(kOneWireSkipRom);
  bus_.writeByte(kDs18b20ConvertTemperature);
  conversionStartedAt_ = timeSource_.uptimeMillis();
  conversionPending_ = true;
  return modules::temperature::makeTemperatureConversionPending();
}

modules::temperature::TemperatureSensorReading
Ds18b20TemperatureSensor::readCompletedConversion() {
  if (!bus_.reset(gpio_)) {
    return modules::temperature::makeTemperatureReadError();
  }

  bus_.writeByte(kOneWireSkipRom);
  bus_.writeByte(kDs18b20ReadScratchpad);

  uint8_t scratchpad[kScratchpadSize] = {};
  for (size_t index = 0; index < kScratchpadSize; ++index) {
    scratchpad[index] = bus_.readByte();
  }

  if (!hasValidDallasCrc(scratchpad)) {
    return modules::temperature::makeTemperatureReadError();
  }

  return modules::temperature::makeValidTemperatureReading(
      scratchpadTemperatureCelsius(scratchpad));
}

void Ds18b20TemperatureSensor::startNextConversionIfPresent() {
  if (!bus_.reset(gpio_)) {
    conversionPending_ = false;
    return;
  }

  bus_.writeByte(kOneWireSkipRom);
  bus_.writeByte(kDs18b20ConvertTemperature);
  conversionStartedAt_ = timeSource_.uptimeMillis();
  conversionPending_ = true;
}

}  // namespace reeflow::drivers::sensors
