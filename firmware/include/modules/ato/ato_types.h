#pragma once

#include <stdint.h>

#include "core/state/system_state.h"

namespace reeflow::modules::ato {

enum class AtoDecision {
  kNoAction,
  kStartRefill,
  kStopAtMaximumLevel,
  kStopTimeout,
  kBlockCooldown,
  kBlockSensorOffline,
  kBlockInvalidConfig,
  kReconcilePumpDivergence,
  kDisable,
  kRecover,
};

enum class AtoDecisionReason {
  kNone,
  kLevelAtOrBelowMinimum,
  kLevelAtOrAboveMaximum,
  kTimeoutElapsed,
  kCooldownActive,
  kSensorOffline,
  kInvalidSensorReading,
  kInvalidConfig,
  kPumpStateDiverged,
  kConfigDisabled,
  kRecoveredFromTimeout,
  kRecoveredFromSensorOffline,
};

enum class AtoPumpCommand {
  kNone,
  kTurnOn,
  kTurnOffAutomation,
  kTurnOffFailsafe,
};

struct AtoPolicyMetadata {
  bool requiresRelayCommand;
  AtoPumpCommand pumpCommand;
  bool emitsEvent;
  bool updatesAtoState;
};

struct AtoEvaluationResult {
  AtoDecision decision;
  AtoDecisionReason reason;
  core::state::AtoStatus nextStatus;
  core::state::AtoState nextAto;
  AtoPolicyMetadata metadata;
};

constexpr AtoEvaluationResult makeNoActionAtoEvaluation(
    core::state::AtoStatus status) {
  return {
      AtoDecision::kNoAction,
      AtoDecisionReason::kNone,
      status,
      {false, status, false, 0, 0, 0},
      {false, AtoPumpCommand::kNone, false, false},
  };
}

}  // namespace reeflow::modules::ato
