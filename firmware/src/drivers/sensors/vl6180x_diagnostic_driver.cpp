#include "drivers/sensors/vl6180x_diagnostic_driver.h"

namespace reeflow::drivers {

Vl6180xDiagnosticReadResult runVl6180xDiagnosticRead(
    Vl6180xDiagnosticSensor& sensor, uint8_t i2cAddress) {
  Vl6180xDiagnosticReadResult result = {
      Vl6180xDiagnosticReadStatus::kReadError,
      i2cAddress,
      0,
      0,
  };

  uint8_t rawRange = 0;
  if (!sensor.readRangeRaw(i2cAddress, rawRange)) {
    return result;
  }

  result.status = Vl6180xDiagnosticReadStatus::kValid;
  result.rawRange = rawRange;
  result.rangeMm =
      static_cast<uint16_t>(rawRange) * kVl6180xRawRangeToMillimeters;
  return result;
}

}  // namespace reeflow::drivers
