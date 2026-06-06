#include "config/pwm_bringup_config.h"

namespace reeflow::config {
namespace {

constexpr PwmBringupParameters kParameters = {
    5000,
    8,
    0,
    64,
    128,
    "Bring-up diagnostic values only; official lighting parameters belong to Phase 8.",
};

}  // namespace

const PwmBringupParameters& pwmBringupParameters() {
  return kParameters;
}

size_t pwmBringupChannelCount() {
  size_t count = 0;

  for (size_t index = 0; index < contracts::kHardwareV1PinCount; ++index) {
    if (contracts::kHardwareV1Pins[index].role ==
        contracts::HardwarePinRole::kPwmOutput) {
      ++count;
    }
  }

  return count;
}

bool getPwmBringupChannel(size_t index, PwmBringupChannel& channel) {
  size_t currentPwm = 0;

  for (size_t pinIndex = 0; pinIndex < contracts::kHardwareV1PinCount; ++pinIndex) {
    const contracts::HardwarePin& pin = contracts::kHardwareV1Pins[pinIndex];

    if (pin.role != contracts::HardwarePinRole::kPwmOutput) {
      continue;
    }

    if (currentPwm == index) {
      channel = {&pin, static_cast<uint8_t>(currentPwm)};
      return true;
    }

    ++currentPwm;
  }

  channel = {};
  return false;
}

}  // namespace reeflow::config
