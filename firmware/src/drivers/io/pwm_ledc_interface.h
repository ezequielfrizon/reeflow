#pragma once

#include <stdint.h>

namespace reeflow::drivers {

class PwmLedcPort {
 public:
  virtual ~PwmLedcPort() = default;

  virtual void setup(uint8_t ledcChannel, uint32_t frequencyHz,
                     uint8_t resolutionBits) = 0;
  virtual void attachPin(uint8_t gpio, uint8_t ledcChannel) = 0;
  virtual void write(uint8_t ledcChannel, uint16_t duty) = 0;
};

}  // namespace reeflow::drivers
