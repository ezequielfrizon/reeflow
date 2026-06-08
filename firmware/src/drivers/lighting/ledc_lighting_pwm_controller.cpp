#include "drivers/lighting/ledc_lighting_pwm_controller.h"

#include <string.h>

#include "contracts/hardware_pins.h"

namespace reeflow::drivers::lighting {
namespace {

struct ChannelBinding {
  modules::lighting::LightingChannel channel;
  const char* hardwareCanonicalName;
  uint8_t ledcChannel;
};

constexpr ChannelBinding kChannelBindings[] = {
    {modules::lighting::LightingChannel::kWhite, "PWM Branco", 0},
    {modules::lighting::LightingChannel::kBlue, "PWM Azul", 1},
    {modules::lighting::LightingChannel::kRoyalBlue, "PWM Royal Blue", 2},
    {modules::lighting::LightingChannel::kUv, "PWM Moonlight/UV", 3},
};

constexpr size_t kChannelBindingCount =
    sizeof(kChannelBindings) / sizeof(kChannelBindings[0]);

const ChannelBinding* bindingForChannel(
    modules::lighting::LightingChannel channel) {
  for (size_t index = 0; index < kChannelBindingCount; ++index) {
    if (kChannelBindings[index].channel == channel) {
      return &kChannelBindings[index];
    }
  }

  return nullptr;
}

bool findHardwareGpio(const char* canonicalName, uint8_t& gpio) {
  for (size_t index = 0; index < contracts::kHardwareV1PinCount; ++index) {
    const contracts::HardwarePin& pin = contracts::kHardwareV1Pins[index];
    if (pin.role == contracts::HardwarePinRole::kPwmOutput &&
        strcmp(pin.canonicalName, canonicalName) == 0) {
      gpio = pin.gpio;
      return true;
    }
  }

  return false;
}

}  // namespace

LedcLightingPwmController::LedcLightingPwmController(PwmLedcPort& pwmPort)
    : pwmPort_(pwmPort),
      config_(modules::lighting::makeDefaultLightingModuleConfig().pwm),
      configured_{} {}

modules::lighting::LightingPwmResult LedcLightingPwmController::begin(
    const modules::lighting::LightingPwmConfig& config) {
  if (!modules::lighting::validLightingPwmConfig(config)) {
    return modules::lighting::LightingPwmResult::kInvalidConfiguration;
  }

  for (size_t index = 0; index < kChannelBindingCount; ++index) {
    const modules::lighting::LightingPwmResult result =
        configureChannel(kChannelBindings[index].channel, config);
    if (result != modules::lighting::LightingPwmResult::kSuccess) {
      return result;
    }
  }

  return modules::lighting::LightingPwmResult::kSuccess;
}

modules::lighting::LightingPwmResult
LedcLightingPwmController::configureChannel(
    modules::lighting::LightingChannel channel,
    const modules::lighting::LightingPwmConfig& config) {
  if (!modules::lighting::validLightingPwmConfig(config)) {
    return modules::lighting::LightingPwmResult::kInvalidConfiguration;
  }

  const ChannelBinding* binding = bindingForChannel(channel);
  if (binding == nullptr) {
    return modules::lighting::LightingPwmResult::kUnknownChannel;
  }

  uint8_t gpio = 0;
  if (!findHardwareGpio(binding->hardwareCanonicalName, gpio)) {
    return modules::lighting::LightingPwmResult::kInvalidConfiguration;
  }

  pwmPort_.setup(binding->ledcChannel, config.frequencyHz,
                 config.resolutionBits);
  pwmPort_.attachPin(gpio, binding->ledcChannel);
  pwmPort_.write(binding->ledcChannel, config.minDuty);

  config_ = config;
  configured_[modules::lighting::lightingChannelIndex(channel)] = true;
  return modules::lighting::LightingPwmResult::kSuccess;
}

modules::lighting::LightingPwmResult LedcLightingPwmController::writeDuty(
    modules::lighting::LightingChannel channel, uint16_t duty) {
  const ChannelBinding* binding = bindingForChannel(channel);
  if (binding == nullptr) {
    return modules::lighting::LightingPwmResult::kUnknownChannel;
  }

  if (duty < config_.minDuty || duty > config_.maxDuty) {
    return modules::lighting::LightingPwmResult::kInvalidDuty;
  }

  if (!configured_[modules::lighting::lightingChannelIndex(channel)]) {
    const modules::lighting::LightingPwmResult result =
        configureChannel(channel, config_);
    if (result != modules::lighting::LightingPwmResult::kSuccess) {
      return result;
    }
  }

  pwmPort_.write(binding->ledcChannel, duty);
  return modules::lighting::LightingPwmResult::kSuccess;
}

modules::lighting::LightingPwmResult LedcLightingPwmController::allOff() {
  for (size_t index = 0; index < kChannelBindingCount; ++index) {
    const modules::lighting::LightingPwmResult result =
        writeDuty(kChannelBindings[index].channel, config_.minDuty);
    if (result != modules::lighting::LightingPwmResult::kSuccess) {
      return result;
    }
  }

  return modules::lighting::LightingPwmResult::kSuccess;
}

}  // namespace reeflow::drivers::lighting
