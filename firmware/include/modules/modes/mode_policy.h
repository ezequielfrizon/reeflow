#pragma once

#include <stdint.h>

#include "core/state/system_state.h"
#include "modules/modes/mode_config.h"
#include "modules/modes/mode_effects.h"
#include "modules/modes/mode_events.h"
#include "modules/modes/mode_types.h"

namespace reeflow::modules::modes {

enum class ModePolicyDecision {
  kNoChange,
  kUpdateRemainingTime,
  kAcceptTransition,
  kRejectInvalidCommand,
  kRejectInvalidConfig,
};

enum class ModePolicyEffectPhase {
  kNone,
  kEnter,
  kStay,
  kExit,
};

struct ModePolicyInput {
  core::state::ModesState currentModes;
  ModeBehaviorConfig config;
  uint32_t nowMillis;
  ModeCommand command;
  bool hasCommand;
};

struct ModePolicyPlan {
  ModePolicyDecision decision;
  core::state::ModesState nextModes;
  ModeEffectsRequest effects;
  ModePolicyEffectPhase effectPhase;
  ModeFinishReason finishReason;
  bool blockAtoAutomation;
  bool updateAtoAutomationGate;
  bool saveMode;
  bool emitModeChanged;
  bool emitModeStarted;
  bool emitModeFinished;
};

ModePolicyPlan evaluateModePolicy(const ModePolicyInput& input);

}  // namespace reeflow::modules::modes
