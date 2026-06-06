#include "drivers/pwm/pwm_diagnostic_driver.h"

#include "drivers/io/arduino_hardware_io.h"

namespace reeflow::drivers {

void configurePwmBringupChannels() {
  configurePwmBringupChannels(arduinoPwmLedcPort());
}

void applyAllPwmDutyZero() {
  applyAllPwmDutyZero(arduinoPwmLedcPort());
}

void applyPwmDuty(const PwmDiagnosticTarget& target, uint16_t duty) {
  applyPwmDuty(arduinoPwmLedcPort(), target, duty);
}

PwmDiagnosticResult runPwmBringupDiagnosticSequence(unsigned long holdMs) {
  return runPwmBringupDiagnosticSequence(arduinoPwmLedcPort(),
                                         arduinoBringupTimer(), holdMs);
}

}  // namespace reeflow::drivers
