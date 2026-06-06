#pragma once

#include <stdint.h>

namespace reeflow::drivers {

enum class Vl6180xDiagnosticReadStatus {
  kValid,
  kReadError,
};

class Vl6180xDiagnosticSensor {
 public:
  virtual ~Vl6180xDiagnosticSensor() = default;

  virtual bool readRangeRaw(uint8_t i2cAddress, uint8_t& rawRange) = 0;
};

struct Vl6180xDiagnosticReadResult {
  Vl6180xDiagnosticReadStatus status;
  uint8_t i2cAddress;
  uint8_t rawRange;
  uint16_t rangeMm;
};

constexpr uint16_t kVl6180xRawRangeToMillimeters = 1;

Vl6180xDiagnosticReadResult runVl6180xDiagnosticRead(
    Vl6180xDiagnosticSensor& sensor, uint8_t i2cAddress);
Vl6180xDiagnosticReadResult runVl6180xDiagnosticRead(uint8_t i2cAddress);
Vl6180xDiagnosticSensor& arduinoVl6180xDiagnosticSensor();

}  // namespace reeflow::drivers
