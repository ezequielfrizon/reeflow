#pragma once

#include <stdint.h>

#include "core/platform/time_source.h"

namespace reeflow::test::fakes {

class FakeTimeSource final : public core::platform::TimeSource {
 public:
  uint32_t uptimeMillis() const override {
    return uptimeMillis_;
  }

  void setUptimeMillis(uint32_t uptimeMillis) {
    uptimeMillis_ = uptimeMillis;
  }

  void advanceMillis(uint32_t deltaMillis) {
    uptimeMillis_ += deltaMillis;
  }

 private:
  uint32_t uptimeMillis_ = 0;
};

}  // namespace reeflow::test::fakes
