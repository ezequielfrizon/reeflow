#pragma once

#include <stdint.h>

#include "drivers/io/bringup_timer.h"
#include "drivers/onewire/onewire_bus.h"

namespace reeflow::drivers {

enum class Ds18b20DiagnosticStatus {
  kFound,
  kNotFound,
  kReadError,
};

struct Ds18b20DiagnosticResult {
  Ds18b20DiagnosticStatus status;
  uint8_t gpio;
  float temperatureC;
};

constexpr unsigned long kDs18b20DiagnosticConversionWaitMs = 750;

Ds18b20DiagnosticResult runDs18b20DiagnosticRead(
    OneWireBus& bus, BringupTimer& timer, uint8_t gpio,
    unsigned long conversionWaitMs = kDs18b20DiagnosticConversionWaitMs);
Ds18b20DiagnosticResult runDs18b20DiagnosticRead(uint8_t gpio);

}  // namespace reeflow::drivers
