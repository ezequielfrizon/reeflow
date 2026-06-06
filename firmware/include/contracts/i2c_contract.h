#pragma once

#include <stdint.h>

namespace reeflow::contracts {

struct I2cDeviceContract {
  const char* canonicalName;
  uint8_t expectedAddress;
  const char* observation;
};

constexpr I2cDeviceContract kVl6180xI2cContract = {
    "VL6180X",
    0x29,
    "Expected address for Phase 1 bring-up comparison; physical confirmation "
    "is pending.",
};

}  // namespace reeflow::contracts
