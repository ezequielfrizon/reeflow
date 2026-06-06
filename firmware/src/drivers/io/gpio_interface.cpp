#include "drivers/io/gpio_interface.h"

namespace reeflow::drivers {

GpioLevel toGpioLevelLowHigh(bool high) {
  return high ? GpioLevel::kHigh : GpioLevel::kLow;
}

}  // namespace reeflow::drivers
