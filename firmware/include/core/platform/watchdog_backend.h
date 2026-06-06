#pragma once

#include <stdint.h>

namespace reeflow::core::platform {

class WatchdogBackend {
 public:
  virtual ~WatchdogBackend() = default;

  virtual void configure(uint32_t timeoutMillis) = 0;
  virtual void feed() = 0;
};

}  // namespace reeflow::core::platform
