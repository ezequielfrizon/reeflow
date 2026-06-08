#pragma once

#include <stdint.h>

#include "config/config_manager.h"
#include "core/state/system_state.h"
#include "modules/ato/ato_config.h"
#include "modules/ato/ato_types.h"

namespace reeflow::modules::ato {

struct AtoPolicyInput {
  core::state::WaterLevelState waterLevel;
  core::state::AtoState ato;
  core::state::RelayEntryState atoPumpRelay;
  config::AtoConfig config;
  uint32_t nowMillis;
};

bool atoConfigIsValid(
    const config::AtoConfig& config,
    const AtoModuleConfig& moduleConfig = makeDefaultAtoModuleConfig());

AtoEvaluationResult evaluateAtoConfiguration(
    const AtoPolicyInput& input,
    const AtoModuleConfig& moduleConfig = makeDefaultAtoModuleConfig());

AtoEvaluationResult evaluateAtoPolicy(
    const AtoPolicyInput& input,
    const AtoModuleConfig& moduleConfig = makeDefaultAtoModuleConfig());

}  // namespace reeflow::modules::ato
