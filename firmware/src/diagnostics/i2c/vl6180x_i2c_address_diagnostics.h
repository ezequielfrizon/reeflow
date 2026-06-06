#pragma once

#include <stdint.h>

#include "drivers/i2c/i2c_diagnostic_bus.h"

namespace reeflow::diagnostics {

enum class Vl6180xI2cAddressStatus {
  kMatch,
  kNotFound,
  kMismatch,
};

struct Vl6180xI2cAddressComparison {
  Vl6180xI2cAddressStatus status;
  uint8_t expectedAddress;
  uint8_t foundAddress;
  bool hasFoundAddress;
};

Vl6180xI2cAddressComparison compareVl6180xI2cAddress(
    const drivers::I2cScanResult& scanResult);
const char* vl6180xI2cAddressStatusToText(Vl6180xI2cAddressStatus status);

}  // namespace reeflow::diagnostics
