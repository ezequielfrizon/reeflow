#include "drivers/sensors/ds18b20_diagnostic_driver.h"

#include <stddef.h>

namespace reeflow::drivers {
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

float scratchpadTemperatureC(const uint8_t* scratchpad) {
  const int16_t rawTemperature =
      static_cast<int16_t>((static_cast<uint16_t>(scratchpad[1]) << 8) |
                           scratchpad[0]);
  return static_cast<float>(rawTemperature) / 16.0F;
}

Ds18b20DiagnosticResult makeResult(Ds18b20DiagnosticStatus status, uint8_t gpio,
                                   float temperatureC = 0.0F) {
  return {status, gpio, temperatureC};
}

}  // namespace

Ds18b20DiagnosticResult runDs18b20DiagnosticRead(
    OneWireBus& bus, BringupTimer& timer, uint8_t gpio,
    unsigned long conversionWaitMs) {
  if (!bus.reset(gpio)) {
    return makeResult(Ds18b20DiagnosticStatus::kNotFound, gpio);
  }

  bus.writeByte(kOneWireSkipRom);
  bus.writeByte(kDs18b20ConvertTemperature);
  timer.delayMs(conversionWaitMs);

  if (!bus.reset(gpio)) {
    return makeResult(Ds18b20DiagnosticStatus::kReadError, gpio);
  }

  bus.writeByte(kOneWireSkipRom);
  bus.writeByte(kDs18b20ReadScratchpad);

  uint8_t scratchpad[kScratchpadSize] = {};
  for (size_t index = 0; index < kScratchpadSize; ++index) {
    scratchpad[index] = bus.readByte();
  }

  if (!hasValidDallasCrc(scratchpad)) {
    return makeResult(Ds18b20DiagnosticStatus::kReadError, gpio);
  }

  return makeResult(Ds18b20DiagnosticStatus::kFound, gpio,
                    scratchpadTemperatureC(scratchpad));
}

}  // namespace reeflow::drivers
