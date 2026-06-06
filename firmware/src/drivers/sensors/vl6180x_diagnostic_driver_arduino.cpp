#include "drivers/sensors/vl6180x_diagnostic_driver.h"

#include <Wire.h>

namespace reeflow::drivers {
namespace {

constexpr uint16_t kVl6180xResultRangeValueRegister = 0x0062;

class ArduinoVl6180xDiagnosticSensor final : public Vl6180xDiagnosticSensor {
 public:
  bool readRangeRaw(uint8_t i2cAddress, uint8_t& rawRange) override {
    Wire.beginTransmission(i2cAddress);
    Wire.write(static_cast<uint8_t>(kVl6180xResultRangeValueRegister >> 8));
    Wire.write(static_cast<uint8_t>(kVl6180xResultRangeValueRegister & 0xFF));
    if (Wire.endTransmission(false) != 0) {
      return false;
    }

    if (Wire.requestFrom(static_cast<int>(i2cAddress), 1) != 1) {
      return false;
    }

    rawRange = static_cast<uint8_t>(Wire.read());
    return true;
  }
};

ArduinoVl6180xDiagnosticSensor vl6180xSensor;

}  // namespace

Vl6180xDiagnosticSensor& arduinoVl6180xDiagnosticSensor() {
  return vl6180xSensor;
}

Vl6180xDiagnosticReadResult runVl6180xDiagnosticRead(uint8_t i2cAddress) {
  return runVl6180xDiagnosticRead(arduinoVl6180xDiagnosticSensor(), i2cAddress);
}

}  // namespace reeflow::drivers
