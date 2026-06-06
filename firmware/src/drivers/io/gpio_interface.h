#pragma once

#include <stdint.h>

namespace reeflow::drivers {

enum class GpioMode {
  kInput,
  kOutput,
};

enum class GpioLevel {
  kLow,
  kHigh,
};

class GpioPort {
 public:
  virtual ~GpioPort() = default;

  virtual void setMode(uint8_t gpio, GpioMode mode) = 0;
  virtual void write(uint8_t gpio, GpioLevel level) = 0;
};

GpioLevel toGpioLevelLowHigh(bool high);

}  // namespace reeflow::drivers
