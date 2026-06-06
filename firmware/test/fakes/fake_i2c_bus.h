#pragma once

#include <stddef.h>
#include <stdint.h>

#include "drivers/i2c/i2c_bus.h"

namespace reeflow::test::fakes {

class FakeI2cBus final : public drivers::I2cBus {
 public:
  static constexpr size_t kMaxRegisters = 16;

  void begin(uint8_t sdaGpio, uint8_t sclGpio) override {
    ++beginCount_;
    lastSdaGpio_ = sdaGpio;
    lastSclGpio_ = sclGpio;
  }

  bool probe(uint8_t address) override {
    ++probeCount_;
    lastProbedAddress_ = address;
    return devicePresent_;
  }

  drivers::I2cOperationStatus writeRegister(uint8_t address, uint16_t reg,
                                            uint8_t value) override {
    ++writeCount_;
    lastWrittenAddress_ = address;
    lastWrittenRegister_ = reg;
    lastWrittenValue_ = value;

    const drivers::I2cOperationStatus status = consumeWriteStatus();
    if (status != drivers::I2cOperationStatus::kOk) {
      return status;
    }

    setRegister(reg, value);
    return drivers::I2cOperationStatus::kOk;
  }

  drivers::I2cOperationStatus readRegister(uint8_t address, uint16_t reg,
                                           uint8_t& value) override {
    ++readCount_;
    lastReadAddress_ = address;
    lastReadRegister_ = reg;

    const drivers::I2cOperationStatus status = consumeReadStatus();
    if (status != drivers::I2cOperationStatus::kOk) {
      return status;
    }

    value = registerValue(reg);
    return drivers::I2cOperationStatus::kOk;
  }

  drivers::I2cOperationStatus readBlock(uint8_t address, uint16_t startReg,
                                        uint8_t* values,
                                        size_t length) override {
    ++readBlockCount_;
    lastReadAddress_ = address;
    lastReadRegister_ = startReg;

    const drivers::I2cOperationStatus status = consumeReadStatus();
    if (status != drivers::I2cOperationStatus::kOk) {
      return status;
    }

    if (values == nullptr && length > 0) {
      return drivers::I2cOperationStatus::kReadError;
    }

    for (size_t index = 0; index < length; ++index) {
      values[index] =
          registerValue(static_cast<uint16_t>(startReg + index));
    }
    return drivers::I2cOperationStatus::kOk;
  }

  void setDevicePresent(bool present) {
    devicePresent_ = present;
  }

  void setNextWriteStatus(drivers::I2cOperationStatus status) {
    nextWriteStatus_ = status;
    hasNextWriteStatus_ = true;
  }

  void setNextReadStatus(drivers::I2cOperationStatus status) {
    nextReadStatus_ = status;
    hasNextReadStatus_ = true;
  }

  void setRegister(uint16_t reg, uint8_t value) {
    for (size_t index = 0; index < registerCount_; ++index) {
      if (registers_[index].reg == reg) {
        registers_[index].value = value;
        return;
      }
    }

    if (registerCount_ >= kMaxRegisters) {
      return;
    }

    registers_[registerCount_] = {reg, value};
    ++registerCount_;
  }

  size_t beginCount() const {
    return beginCount_;
  }

  size_t probeCount() const {
    return probeCount_;
  }

  size_t writeCount() const {
    return writeCount_;
  }

  size_t readCount() const {
    return readCount_;
  }

  size_t readBlockCount() const {
    return readBlockCount_;
  }

  uint8_t lastSdaGpio() const {
    return lastSdaGpio_;
  }

  uint8_t lastSclGpio() const {
    return lastSclGpio_;
  }

  uint8_t lastProbedAddress() const {
    return lastProbedAddress_;
  }

  uint8_t lastWrittenAddress() const {
    return lastWrittenAddress_;
  }

  uint16_t lastWrittenRegister() const {
    return lastWrittenRegister_;
  }

  uint8_t lastWrittenValue() const {
    return lastWrittenValue_;
  }

  uint8_t lastReadAddress() const {
    return lastReadAddress_;
  }

  uint16_t lastReadRegister() const {
    return lastReadRegister_;
  }

 private:
  struct RegisterValue {
    uint16_t reg;
    uint8_t value;
  };

  drivers::I2cOperationStatus consumeWriteStatus() {
    if (!hasNextWriteStatus_) {
      return drivers::I2cOperationStatus::kOk;
    }

    hasNextWriteStatus_ = false;
    return nextWriteStatus_;
  }

  drivers::I2cOperationStatus consumeReadStatus() {
    if (!hasNextReadStatus_) {
      return drivers::I2cOperationStatus::kOk;
    }

    hasNextReadStatus_ = false;
    return nextReadStatus_;
  }

  uint8_t registerValue(uint16_t reg) const {
    for (size_t index = 0; index < registerCount_; ++index) {
      if (registers_[index].reg == reg) {
        return registers_[index].value;
      }
    }

    return 0;
  }

  bool devicePresent_ = true;
  RegisterValue registers_[kMaxRegisters] = {};
  size_t registerCount_ = 0;
  bool hasNextWriteStatus_ = false;
  bool hasNextReadStatus_ = false;
  drivers::I2cOperationStatus nextWriteStatus_ =
      drivers::I2cOperationStatus::kOk;
  drivers::I2cOperationStatus nextReadStatus_ =
      drivers::I2cOperationStatus::kOk;
  size_t beginCount_ = 0;
  size_t probeCount_ = 0;
  size_t writeCount_ = 0;
  size_t readCount_ = 0;
  size_t readBlockCount_ = 0;
  uint8_t lastSdaGpio_ = 0;
  uint8_t lastSclGpio_ = 0;
  uint8_t lastProbedAddress_ = 0;
  uint8_t lastWrittenAddress_ = 0;
  uint16_t lastWrittenRegister_ = 0;
  uint8_t lastWrittenValue_ = 0;
  uint8_t lastReadAddress_ = 0;
  uint16_t lastReadRegister_ = 0;
};

}  // namespace reeflow::test::fakes
