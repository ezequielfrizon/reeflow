#pragma once

#include <stddef.h>
#include <stdint.h>

#include "contracts/hardware_pins.h"

namespace reeflow::config {

struct PwmBringupParameters {
  uint32_t frequencyHz;
  uint8_t resolutionBits;
  uint16_t dutyZero;
  uint16_t dutyTestMin;
  uint16_t dutyTestMax;
  const char* note;
};

struct PwmBringupChannel {
  const contracts::HardwarePin* pin;
  uint8_t ledcChannel;
};

const PwmBringupParameters& pwmBringupParameters();
size_t pwmBringupChannelCount();
bool getPwmBringupChannel(size_t index, PwmBringupChannel& channel);

}  // namespace reeflow::config
