#include "drivers/sensors/ds18b20_diagnostic_driver.h"

#include "drivers/io/arduino_hardware_io.h"

namespace reeflow::drivers {

Ds18b20DiagnosticResult runDs18b20DiagnosticRead(uint8_t gpio) {
  return runDs18b20DiagnosticRead(arduinoOneWireBus(), arduinoBringupTimer(),
                                  gpio);
}

}  // namespace reeflow::drivers
