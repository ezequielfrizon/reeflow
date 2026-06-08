#pragma once

#include <stdint.h>

#include "config/config_manager.h"

namespace reeflow::modules::modes {

enum class ModeReturnBehavior {
  kManual,
  kAutomatic,
};

struct ModeBehaviorConfig {
  uint32_t feedingDurationSeconds;
  ModeReturnBehavior feedingReturn;
  ModeReturnBehavior tpaReturn;
  ModeReturnBehavior maintenanceReturn;
  bool normalReleasesModeBlocks;
  bool feedingTurnsOffRecalque;
  bool feedingBlocksAtoAutomation;
  bool tpaTurnsOffRecalque;
  bool tpaTurnsOffAtoPump;
  bool tpaBlocksAtoAutomation;
  bool maintenanceBlocksNonCriticalAutomations;
  bool maintenanceBlocksAtoAutomation;
};

constexpr uint32_t kModeServiceTaskIntervalMillis = 1000;

inline ModeBehaviorConfig makeModeBehaviorConfig(
    const config::TimersConfig& timers) {
  ModeBehaviorConfig config = {};
  config.feedingDurationSeconds = timers.feedingDurationSeconds;
  config.feedingReturn = ModeReturnBehavior::kAutomatic;
  config.tpaReturn = ModeReturnBehavior::kManual;
  config.maintenanceReturn = ModeReturnBehavior::kManual;
  config.normalReleasesModeBlocks = true;
  config.feedingTurnsOffRecalque = true;
  config.feedingBlocksAtoAutomation = true;
  config.tpaTurnsOffRecalque = true;
  config.tpaTurnsOffAtoPump = true;
  config.tpaBlocksAtoAutomation = true;
  config.maintenanceBlocksNonCriticalAutomations = true;
  config.maintenanceBlocksAtoAutomation = true;
  return config;
}

inline ModeBehaviorConfig makeModeBehaviorConfig(
    const config::ConfigManager& configManager) {
  return makeModeBehaviorConfig(configManager.timers());
}

}  // namespace reeflow::modules::modes
