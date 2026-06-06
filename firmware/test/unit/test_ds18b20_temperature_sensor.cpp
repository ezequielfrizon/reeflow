#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include <vector>

#include "drivers/onewire/onewire_bus.h"
#include "drivers/sensors/ds18b20/ds18b20_temperature_sensor.h"
#include "fakes/fake_time_source.h"
#include "modules/temperature/temperature_sensor.h"

namespace {

using reeflow::drivers::OneWireBus;
using reeflow::drivers::sensors::Ds18b20TemperatureSensor;
using reeflow::drivers::sensors::ds18b20TemperatureSensorGpio;
using reeflow::drivers::sensors::kDs18b20TemperatureConversionWaitMillis;
using reeflow::modules::temperature::TemperatureSensorReadStatus;
using reeflow::test::fakes::FakeTimeSource;

struct FakeOneWireBus final : OneWireBus {
  bool reset(uint8_t gpio) override {
    resetGpios.push_back(gpio);
    if (resetIndex >= resetResults.size()) {
      return false;
    }

    return resetResults[resetIndex++];
  }

  void writeByte(uint8_t value) override {
    writtenBytes.push_back(value);
  }

  uint8_t readByte() override {
    if (readIndex >= readBytes.size()) {
      return 0;
    }

    return readBytes[readIndex++];
  }

  std::vector<bool> resetResults;
  std::vector<uint8_t> resetGpios;
  std::vector<uint8_t> writtenBytes;
  std::vector<uint8_t> readBytes;
  size_t resetIndex = 0;
  size_t readIndex = 0;
};

uint8_t dallasCrc(const uint8_t* values, size_t count) {
  uint8_t crc = 0;

  for (size_t index = 0; index < count; ++index) {
    uint8_t value = values[index];
    for (uint8_t bit = 0; bit < 8; ++bit) {
      const uint8_t mix = (crc ^ value) & 0x01U;
      crc >>= 1;
      if (mix != 0) {
        crc ^= 0x8CU;
      }
      value >>= 1;
    }
  }

  return crc;
}

void appendScratchpad(FakeOneWireBus& bus, float temperatureCelsius) {
  const int16_t rawTemperature =
      static_cast<int16_t>(temperatureCelsius * 16.0F);
  uint8_t scratchpad[9] = {
      static_cast<uint8_t>(rawTemperature & 0xFF),
      static_cast<uint8_t>((rawTemperature >> 8) & 0xFF),
      0,
      0,
      0,
      0,
      0,
      0,
      0,
  };
  scratchpad[8] = dallasCrc(scratchpad, 8);

  bus.readBytes.insert(bus.readBytes.end(), scratchpad, scratchpad + 9);
}

void appendInvalidScratchpad(FakeOneWireBus& bus) {
  for (size_t index = 0; index < 9; ++index) {
    bus.readBytes.push_back(0xFF);
  }
}

void testAdapterUsesOfficialDs18b20Gpio() {
  FakeOneWireBus bus;
  FakeTimeSource timeSource;
  Ds18b20TemperatureSensor sensor(bus, timeSource);

  assert(ds18b20TemperatureSensorGpio() == 4);
  assert(sensor.gpio() == ds18b20TemperatureSensorGpio());
}

void testAdapterStartsConversionWithoutBlocking() {
  FakeOneWireBus bus;
  FakeTimeSource timeSource;
  Ds18b20TemperatureSensor sensor(bus, timeSource);
  bus.resetResults.push_back(true);

  const auto reading = sensor.readTemperature();

  assert(reading.status == TemperatureSensorReadStatus::kConversionPending);
  assert(bus.resetGpios.size() == 1);
  assert(bus.resetGpios[0] == ds18b20TemperatureSensorGpio());
  assert(bus.writtenBytes.size() == 2);
  assert(bus.writtenBytes[0] == 0xCC);
  assert(bus.writtenBytes[1] == 0x44);
}

void testAdapterReturnsPendingBeforeConversionWait() {
  FakeOneWireBus bus;
  FakeTimeSource timeSource;
  Ds18b20TemperatureSensor sensor(bus, timeSource);
  bus.resetResults.push_back(true);

  assert(sensor.readTemperature().status ==
         TemperatureSensorReadStatus::kConversionPending);
  timeSource.advanceMillis(kDs18b20TemperatureConversionWaitMillis - 1);

  assert(sensor.readTemperature().status ==
         TemperatureSensorReadStatus::kConversionPending);
  assert(bus.resetGpios.size() == 1);
}

void testAdapterTranslatesValidScratchpadToCelsius() {
  FakeOneWireBus bus;
  FakeTimeSource timeSource;
  Ds18b20TemperatureSensor sensor(bus, timeSource);
  bus.resetResults.push_back(true);
  bus.resetResults.push_back(true);
  bus.resetResults.push_back(true);
  appendScratchpad(bus, 26.5F);

  assert(sensor.readTemperature().status ==
         TemperatureSensorReadStatus::kConversionPending);
  timeSource.advanceMillis(kDs18b20TemperatureConversionWaitMillis);
  const auto reading = sensor.readTemperature();

  assert(reading.status == TemperatureSensorReadStatus::kValid);
  assert(reading.temperatureCelsius == 26.5F);
  assert(bus.resetGpios[0] == ds18b20TemperatureSensorGpio());
  assert(bus.resetGpios[1] == ds18b20TemperatureSensorGpio());
  assert(bus.resetGpios[2] == ds18b20TemperatureSensorGpio());
}

void testAdapterTranslatesMissingSensor() {
  FakeOneWireBus bus;
  FakeTimeSource timeSource;
  Ds18b20TemperatureSensor sensor(bus, timeSource);
  bus.resetResults.push_back(false);

  const auto reading = sensor.readTemperature();

  assert(reading.status == TemperatureSensorReadStatus::kSensorNotFound);
  assert(reading.temperatureCelsius == 0.0F);
}

void testAdapterTranslatesReadResetFailureToReadError() {
  FakeOneWireBus bus;
  FakeTimeSource timeSource;
  Ds18b20TemperatureSensor sensor(bus, timeSource);
  bus.resetResults.push_back(true);
  bus.resetResults.push_back(false);

  assert(sensor.readTemperature().status ==
         TemperatureSensorReadStatus::kConversionPending);
  timeSource.advanceMillis(kDs18b20TemperatureConversionWaitMillis);
  const auto reading = sensor.readTemperature();

  assert(reading.status == TemperatureSensorReadStatus::kReadError);
}

void testAdapterTranslatesInvalidCrcToReadError() {
  FakeOneWireBus bus;
  FakeTimeSource timeSource;
  Ds18b20TemperatureSensor sensor(bus, timeSource);
  bus.resetResults.push_back(true);
  bus.resetResults.push_back(true);
  bus.resetResults.push_back(true);
  appendInvalidScratchpad(bus);

  assert(sensor.readTemperature().status ==
         TemperatureSensorReadStatus::kConversionPending);
  timeSource.advanceMillis(kDs18b20TemperatureConversionWaitMillis);
  const auto reading = sensor.readTemperature();

  assert(reading.status == TemperatureSensorReadStatus::kReadError);
}

}  // namespace

int main() {
  testAdapterUsesOfficialDs18b20Gpio();
  testAdapterStartsConversionWithoutBlocking();
  testAdapterReturnsPendingBeforeConversionWait();
  testAdapterTranslatesValidScratchpadToCelsius();
  testAdapterTranslatesMissingSensor();
  testAdapterTranslatesReadResetFailureToReadError();
  testAdapterTranslatesInvalidCrcToReadError();
  return 0;
}
