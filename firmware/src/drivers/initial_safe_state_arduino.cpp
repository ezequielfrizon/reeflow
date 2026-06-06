#include "drivers/initial_safe_state.h"

#include "drivers/io/arduino_hardware_io.h"

namespace reeflow::drivers {

InitialSafeStateResult applyInitialSafeHardwareState() {
  return applyInitialSafeHardwareState(arduinoGpioPort(), arduinoPwmLedcPort());
}

}  // namespace reeflow::drivers
