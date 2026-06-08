#pragma once

#include <stdint.h>

#include "modules/lighting/lighting_config.h"
#include "modules/lighting/lighting_types.h"

namespace reeflow::modules::lighting {

enum class LightingPwmResult {
  kSuccess,
  kUnknownChannel,
  kControllerFailure,
  kInvalidDuty,
  kInvalidConfiguration,
};

class LightingPwmController {
 public:
  virtual ~LightingPwmController() = default;

  virtual LightingPwmResult configureChannel(
      LightingChannel channel, const LightingPwmConfig& config) = 0;
  virtual LightingPwmResult writeDuty(LightingChannel channel,
                                      uint16_t duty) = 0;
  virtual LightingPwmResult allOff() = 0;
};

}  // namespace reeflow::modules::lighting
