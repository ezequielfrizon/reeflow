#pragma once

#include <stdint.h>

#include "drivers/io/pwm_ledc_interface.h"
#include "modules/lighting/lighting_pwm_controller.h"

namespace reeflow::drivers::lighting {

class LedcLightingPwmController final
    : public modules::lighting::LightingPwmController {
 public:
  explicit LedcLightingPwmController(PwmLedcPort& pwmPort);

  modules::lighting::LightingPwmResult begin(
      const modules::lighting::LightingPwmConfig& config =
          modules::lighting::makeDefaultLightingModuleConfig().pwm);

  modules::lighting::LightingPwmResult configureChannel(
      modules::lighting::LightingChannel channel,
      const modules::lighting::LightingPwmConfig& config) override;
  modules::lighting::LightingPwmResult writeDuty(
      modules::lighting::LightingChannel channel, uint16_t duty) override;
  modules::lighting::LightingPwmResult allOff() override;

 private:
  PwmLedcPort& pwmPort_;
  modules::lighting::LightingPwmConfig config_;
  bool configured_[modules::lighting::kLightingFunctionalChannelCount];
};

}  // namespace reeflow::drivers::lighting
