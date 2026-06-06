#pragma once

#include <stddef.h>

#include "drivers/io/gpio_interface.h"
#include "drivers/io/pwm_ledc_interface.h"

namespace reeflow::drivers {

struct InitialSafeStateResult {
  size_t relaysConfigured;
  size_t pwmChannelsConfigured;
  size_t sensorBusPinsPrepared;
};

InitialSafeStateResult applyInitialSafeHardwareState(GpioPort& gpio,
                                                     PwmLedcPort& pwm);
InitialSafeStateResult applyInitialSafeHardwareState();

}  // namespace reeflow::drivers
