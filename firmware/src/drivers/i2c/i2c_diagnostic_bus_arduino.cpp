#include "drivers/i2c/i2c_diagnostic_bus.h"

#include <Wire.h>

namespace reeflow::drivers {
namespace {

class ArduinoI2cDiagnosticBus final : public I2cDiagnosticBus {
 public:
  void begin(uint8_t sdaGpio, uint8_t sclGpio) override {
    Wire.begin(sdaGpio, sclGpio);
  }

  bool probe(uint8_t address) override {
    Wire.beginTransmission(address);
    return Wire.endTransmission() == 0;
  }
};

ArduinoI2cDiagnosticBus i2cBus;

}  // namespace

I2cDiagnosticBus& arduinoI2cDiagnosticBus() {
  return i2cBus;
}

I2cScanResult runI2cDiagnosticScan(uint8_t sdaGpio, uint8_t sclGpio) {
  return runI2cDiagnosticScan(arduinoI2cDiagnosticBus(), sdaGpio, sclGpio);
}

}  // namespace reeflow::drivers
