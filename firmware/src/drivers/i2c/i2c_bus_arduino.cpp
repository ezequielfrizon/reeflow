#include "drivers/i2c/i2c_bus.h"

#include <Wire.h>

namespace reeflow::drivers {
namespace {

I2cOperationStatus mapTransmissionStatus(uint8_t status) {
  switch (status) {
    case 0:
      return I2cOperationStatus::kOk;
    case 2:
      return I2cOperationStatus::kAddressNotFound;
    case 5:
      return I2cOperationStatus::kTimeout;
    default:
      return I2cOperationStatus::kWriteError;
  }
}

void writeRegisterAddress(uint16_t reg) {
  Wire.write(static_cast<uint8_t>(reg >> 8));
  Wire.write(static_cast<uint8_t>(reg & 0xFF));
}

class ArduinoI2cBus final : public I2cBus {
 public:
  void begin(uint8_t sdaGpio, uint8_t sclGpio) override {
    Wire.begin(sdaGpio, sclGpio);
  }

  bool probe(uint8_t address) override {
    Wire.beginTransmission(address);
    return Wire.endTransmission() == 0;
  }

  I2cOperationStatus writeRegister(uint8_t address, uint16_t reg,
                                   uint8_t value) override {
    Wire.beginTransmission(address);
    writeRegisterAddress(reg);
    Wire.write(value);
    return mapTransmissionStatus(Wire.endTransmission());
  }

  I2cOperationStatus readRegister(uint8_t address, uint16_t reg,
                                  uint8_t& value) override {
    Wire.beginTransmission(address);
    writeRegisterAddress(reg);
    const I2cOperationStatus writeStatus =
        mapTransmissionStatus(Wire.endTransmission(false));
    if (writeStatus != I2cOperationStatus::kOk) {
      return writeStatus;
    }

    if (Wire.requestFrom(static_cast<int>(address), 1) != 1) {
      return I2cOperationStatus::kReadError;
    }

    value = static_cast<uint8_t>(Wire.read());
    return I2cOperationStatus::kOk;
  }

  I2cOperationStatus readBlock(uint8_t address, uint16_t startReg,
                               uint8_t* values, size_t length) override {
    if (values == nullptr && length > 0) {
      return I2cOperationStatus::kReadError;
    }

    Wire.beginTransmission(address);
    writeRegisterAddress(startReg);
    const I2cOperationStatus writeStatus =
        mapTransmissionStatus(Wire.endTransmission(false));
    if (writeStatus != I2cOperationStatus::kOk) {
      return writeStatus;
    }

    const size_t bytesRead =
        static_cast<size_t>(Wire.requestFrom(static_cast<int>(address),
                                            static_cast<int>(length)));
    if (bytesRead != length) {
      return I2cOperationStatus::kReadError;
    }

    for (size_t index = 0; index < length; ++index) {
      values[index] = static_cast<uint8_t>(Wire.read());
    }
    return I2cOperationStatus::kOk;
  }
};

ArduinoI2cBus i2cBus;

}  // namespace

I2cBus& arduinoI2cBus() {
  return i2cBus;
}

}  // namespace reeflow::drivers
