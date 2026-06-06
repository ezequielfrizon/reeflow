#pragma once

#include <stdint.h>

namespace reeflow::core::platform {

class TimeSource {
 public:
  virtual ~TimeSource() = default;

  virtual uint32_t uptimeMillis() const = 0;
};

}  // namespace reeflow::core::platform
