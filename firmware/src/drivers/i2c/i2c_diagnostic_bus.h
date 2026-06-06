#pragma once

#include <stddef.h>
#include <stdint.h>

namespace reeflow::drivers {

constexpr uint8_t kI2cScanFirstAddress = 0x03;
constexpr uint8_t kI2cScanLastAddress = 0x77;
constexpr size_t kI2cScanMaxDevices = 16;

class I2cDiagnosticBus {
 public:
  virtual ~I2cDiagnosticBus() = default;

  virtual void begin(uint8_t sdaGpio, uint8_t sclGpio) = 0;
  virtual bool probe(uint8_t address) = 0;
};

struct I2cScanResult {
  uint8_t sdaGpio;
  uint8_t sclGpio;
  size_t deviceCount;
  bool overflow;
  uint8_t addresses[kI2cScanMaxDevices];
};

I2cScanResult runI2cDiagnosticScan(I2cDiagnosticBus& bus, uint8_t sdaGpio,
                                   uint8_t sclGpio);
I2cScanResult runI2cDiagnosticScan(uint8_t sdaGpio, uint8_t sclGpio);
I2cDiagnosticBus& arduinoI2cDiagnosticBus();

}  // namespace reeflow::drivers
