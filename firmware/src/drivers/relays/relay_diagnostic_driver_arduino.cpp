#include "drivers/relays/relay_diagnostic_driver.h"

#include "drivers/io/arduino_hardware_io.h"

namespace reeflow::drivers {

void applyAllRelaysOff() {
  applyAllRelaysOff(arduinoGpioPort());
}

RelayDiagnosticResult runIndividualRelayDiagnostic(uint8_t physicalIndex,
                                                   unsigned long holdMs) {
  return runIndividualRelayDiagnostic(physicalIndex, holdMs, arduinoGpioPort(),
                                      arduinoBringupTimer());
}

}  // namespace reeflow::drivers
