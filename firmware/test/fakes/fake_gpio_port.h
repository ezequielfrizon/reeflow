#pragma once

#include <stddef.h>
#include <stdint.h>

#include "drivers/io/gpio_interface.h"

namespace reeflow::test::fakes {

class FakeGpioPort final : public drivers::GpioPort {
 public:
  struct ModeOperation {
    uint8_t gpio;
    drivers::GpioMode mode;
  };

  struct WriteOperation {
    uint8_t gpio;
    drivers::GpioLevel level;
  };

  static constexpr size_t kMaxOperations = 32;

  void setMode(uint8_t gpio, drivers::GpioMode mode) override {
    if (modeOperationCount_ < kMaxOperations) {
      modeOperations_[modeOperationCount_++] = {gpio, mode};
    }
  }

  void write(uint8_t gpio, drivers::GpioLevel level) override {
    if (writeOperationCount_ < kMaxOperations) {
      writeOperations_[writeOperationCount_++] = {gpio, level};
    }
  }

  size_t modeOperationCount() const {
    return modeOperationCount_;
  }

  size_t writeOperationCount() const {
    return writeOperationCount_;
  }

  const ModeOperation& modeOperation(size_t index) const {
    return modeOperations_[index];
  }

  const WriteOperation& writeOperation(size_t index) const {
    return writeOperations_[index];
  }

 private:
  ModeOperation modeOperations_[kMaxOperations] = {};
  WriteOperation writeOperations_[kMaxOperations] = {};
  size_t modeOperationCount_ = 0;
  size_t writeOperationCount_ = 0;
};

}  // namespace reeflow::test::fakes
