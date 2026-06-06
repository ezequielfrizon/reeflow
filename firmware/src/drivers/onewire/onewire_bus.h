#pragma once

#include <stdint.h>

namespace reeflow::drivers {

class OneWireBus {
 public:
  virtual ~OneWireBus() = default;

  virtual bool reset(uint8_t gpio) = 0;
  virtual void writeByte(uint8_t value) = 0;
  virtual uint8_t readByte() = 0;
};

OneWireBus& arduinoOneWireBus();

}  // namespace reeflow::drivers
