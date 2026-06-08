#pragma once

#include <stdint.h>

#include "config/config_manager.h"
#include "core/state/system_state.h"
#include "modules/lighting/lighting_config.h"
#include "modules/lighting/lighting_profile.h"
#include "modules/lighting/lighting_types.h"

namespace reeflow::modules::lighting {

enum class LightingPolicyReason {
  kNone,
  kManualCommand,
  kManualProfile,
  kAutomaticCurve,
  kSunrise,
  kSunset,
  kMoonlight,
  kAcclimation,
  kChannelDisabled,
  kInvalidCommand,
  kInvalidConfig,
  kInvalidProfile,
  kFade,
  kIdempotent,
};

struct LightingPolicyChannelDecision {
  LightingChannel channel;
  uint16_t currentDuty;
  uint16_t targetDuty;
  uint16_t nextDuty;
  bool shouldWrite;
  LightingPolicyReason reason;
};

struct LightingPolicyInput {
  const core::state::LightingState& currentState;
  const config::LightingConfig& lightingConfig;
  const LightingProfile& activeProfile;
  uint16_t currentMinuteOfDay;
  uint16_t acclimationElapsedDays;
  const LightingCommand* command;
  LightingModuleConfig moduleConfig;
};

struct LightingPolicyDecision {
  LightingMode mode;
  bool accepted;
  LightingPolicyReason reason;
  LightingProfileValidationResult profileValidation;
  LightingPolicyChannelDecision channels[kLightingFunctionalChannelCount];
};

LightingPolicyDecision evaluateLightingPolicy(
    const LightingPolicyInput& input);

}  // namespace reeflow::modules::lighting
