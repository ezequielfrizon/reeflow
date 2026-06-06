#pragma once

#include <stdint.h>

#include "core/platform/watchdog_backend.h"

namespace reeflow::test::fakes {

class FakeWatchdogBackend final : public core::platform::WatchdogBackend {
 public:
  void configure(uint32_t timeoutMillis) override {
    configureCalls_ += 1;
    configuredTimeoutMillis_ = timeoutMillis;
  }

  void feed() override {
    feedCalls_ += 1;
  }

  uint32_t configuredTimeoutMillis() const {
    return configuredTimeoutMillis_;
  }

  uint32_t configureCalls() const {
    return configureCalls_;
  }

  uint32_t feedCalls() const {
    return feedCalls_;
  }

 private:
  uint32_t configuredTimeoutMillis_ = 0;
  uint32_t configureCalls_ = 0;
  uint32_t feedCalls_ = 0;
};

}  // namespace reeflow::test::fakes
