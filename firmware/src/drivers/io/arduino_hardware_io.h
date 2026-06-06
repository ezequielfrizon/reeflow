#pragma once

#include "drivers/io/bringup_timer.h"
#include "drivers/io/gpio_interface.h"
#include "drivers/io/pwm_ledc_interface.h"

namespace reeflow::drivers {

GpioPort& arduinoGpioPort();
PwmLedcPort& arduinoPwmLedcPort();
BringupTimer& arduinoBringupTimer();

}  // namespace reeflow::drivers
