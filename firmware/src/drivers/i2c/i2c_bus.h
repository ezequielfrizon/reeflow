#pragma once

#include <stddef.h>
#include <stdint.h>

namespace reeflow::drivers {

enum class I2cOperationStatus {
  kOk,
  kAddressNotFound,
  kWriteError,
  kReadError,
  kTimeout,
};

class I2cBus {
 public:
  virtual ~I2cBus() = default;

  virtual void begin(uint8_t sdaGpio, uint8_t sclGpio) = 0;
  virtual bool probe(uint8_t address) = 0;
  virtual I2cOperationStatus writeRegister(uint8_t address, uint16_t reg,
                                           uint8_t value) = 0;
  virtual I2cOperationStatus readRegister(uint8_t address, uint16_t reg,
                                          uint8_t& value) = 0;
  virtual I2cOperationStatus readBlock(uint8_t address, uint16_t startReg,
                                       uint8_t* values, size_t length) = 0;
};

I2cBus& arduinoI2cBus();

}  // namespace reeflow::drivers
