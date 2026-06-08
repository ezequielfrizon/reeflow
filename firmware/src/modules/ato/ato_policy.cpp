#include "modules/ato/ato_policy.h"

namespace reeflow::modules::ato {
namespace {

bool atoStatusIsCanonical(core::state::AtoStatus status) {
  return status == core::state::AtoStatus::kNormal ||
         status == core::state::AtoStatus::kRefilling ||
         status == core::state::AtoStatus::kTimeout ||
         status == core::state::AtoStatus::kSensorOffline ||
         status == core::state::AtoStatus::kDisabled;
}

bool pumpMayBeRunning(const core::state::AtoState& ato,
                      const core::state::RelayEntryState& relay) {
  return ato.pumpRunning || relay.enabled;
}

bool elapsedAtLeast(uint32_t nowMillis, uint32_t startedAtMillis,
                    uint32_t intervalMillis) {
  return static_cast<uint32_t>(nowMillis - startedAtMillis) >= intervalMillis;
}

bool cooldownActive(const AtoPolicyInput& input) {
  return input.ato.lastCompletion > 0 &&
         !elapsedAtLeast(input.nowMillis, input.ato.lastCompletion,
                         input.config.cooldownMillis);
}

bool sensorOffline(const AtoPolicyInput& input,
                   const AtoModuleConfig& moduleConfig) {
  return input.waterLevel.status == core::state::WaterLevelStatus::kSensorOffline ||
         input.waterLevel.currentLevel > moduleConfig.canonicalMaximumLevel;
}

bool levelWithinAcceptableRange(const AtoPolicyInput& input) {
  return input.waterLevel.currentLevel >= input.config.minimumLevel &&
         input.waterLevel.currentLevel <= input.config.maximumLevel;
}

bool refillInProgress(const AtoPolicyInput& input) {
  return input.ato.status == core::state::AtoStatus::kRefilling &&
         input.ato.pumpRunning && input.atoPumpRelay.enabled;
}

bool statusCanRecover(core::state::AtoStatus status) {
  return status == core::state::AtoStatus::kTimeout ||
         status == core::state::AtoStatus::kSensorOffline;
}

AtoEvaluationResult makeDecision(
    const AtoPolicyInput& input, AtoDecision decision,
    AtoDecisionReason reason, core::state::AtoStatus nextStatus,
    bool nextPumpRunning, AtoPumpCommand pumpCommand, bool emitsEvent,
    uint32_t nextLastActivation, uint32_t nextLastCompletion,
    uint32_t nextTimeoutCounter) {
  AtoEvaluationResult result = {};
  result.decision = decision;
  result.reason = reason;
  result.nextStatus = nextStatus;
  result.nextAto = input.ato;
  result.nextAto.enabled = input.config.enabled;
  result.nextAto.status = nextStatus;
  result.nextAto.pumpRunning = nextPumpRunning;
  result.nextAto.lastActivation = nextLastActivation;
  result.nextAto.lastCompletion = nextLastCompletion;
  result.nextAto.timeoutCounter = nextTimeoutCounter;
  result.metadata.pumpCommand = pumpCommand;
  result.metadata.requiresRelayCommand = pumpCommand != AtoPumpCommand::kNone;
  result.metadata.emitsEvent = emitsEvent;
  result.metadata.updatesAtoState =
      result.nextAto.enabled != input.ato.enabled ||
      result.nextAto.status != input.ato.status ||
      result.nextAto.pumpRunning != input.ato.pumpRunning ||
      result.nextAto.lastActivation != input.ato.lastActivation ||
      result.nextAto.lastCompletion != input.ato.lastCompletion ||
      result.nextAto.timeoutCounter != input.ato.timeoutCounter;
  return result;
}

AtoEvaluationResult makeNoAction(const AtoPolicyInput& input) {
  const core::state::AtoStatus status =
      input.config.enabled &&
              input.ato.status == core::state::AtoStatus::kDisabled
          ? core::state::AtoStatus::kNormal
          : input.ato.status;
  return makeDecision(input, AtoDecision::kNoAction, AtoDecisionReason::kNone,
                      status, input.ato.pumpRunning,
                      AtoPumpCommand::kNone, false, input.ato.lastActivation,
                      input.ato.lastCompletion, input.ato.timeoutCounter);
}

AtoEvaluationResult makeFailsafeOffDecision(
    const AtoPolicyInput& input, AtoDecision decision,
    AtoDecisionReason reason, core::state::AtoStatus nextStatus,
    bool emitsEvent, bool incrementTimeoutCounter) {
  const uint32_t timeoutCounter =
      incrementTimeoutCounter ? input.ato.timeoutCounter + 1
                              : input.ato.timeoutCounter;
  return makeDecision(input, decision, reason, nextStatus, false,
                      pumpMayBeRunning(input.ato, input.atoPumpRelay)
                          ? AtoPumpCommand::kTurnOffFailsafe
                          : AtoPumpCommand::kNone,
                      emitsEvent, input.ato.lastActivation, input.nowMillis,
                      timeoutCounter);
}

}  // namespace

bool atoConfigIsValid(const config::AtoConfig& config,
                      const AtoModuleConfig& moduleConfig) {
  return config.minimumLevel >= moduleConfig.canonicalMinimumLevel &&
         config.maximumLevel <= moduleConfig.canonicalMaximumLevel &&
         config.minimumLevel < config.maximumLevel &&
         config.timeoutMillis > 0 && config.cooldownMillis > 0;
}

AtoEvaluationResult evaluateAtoConfiguration(
    const AtoPolicyInput& input, const AtoModuleConfig& moduleConfig) {
  if (atoConfigIsValid(input.config, moduleConfig)) {
    return makeNoAction(input);
  }

  const core::state::AtoStatus nextStatus =
      atoStatusIsCanonical(input.ato.status) ? input.ato.status
                                             : core::state::AtoStatus::kNormal;
  return makeDecision(
      input, AtoDecision::kBlockInvalidConfig,
      AtoDecisionReason::kInvalidConfig, nextStatus, false,
      pumpMayBeRunning(input.ato, input.atoPumpRelay)
          ? AtoPumpCommand::kTurnOffFailsafe
          : AtoPumpCommand::kNone,
      false, input.ato.lastActivation, input.ato.lastCompletion,
      input.ato.timeoutCounter);
}

AtoEvaluationResult evaluateAtoPolicy(
    const AtoPolicyInput& input, const AtoModuleConfig& moduleConfig) {
  if (!input.config.enabled) {
    return makeDecision(
        input, AtoDecision::kDisable, AtoDecisionReason::kConfigDisabled,
        core::state::AtoStatus::kDisabled, false,
        pumpMayBeRunning(input.ato, input.atoPumpRelay)
            ? AtoPumpCommand::kTurnOffFailsafe
            : AtoPumpCommand::kNone,
        false, input.ato.lastActivation, input.ato.lastCompletion,
        input.ato.timeoutCounter);
  }

  if (!atoConfigIsValid(input.config, moduleConfig)) {
    return evaluateAtoConfiguration(input, moduleConfig);
  }

  if (input.ato.pumpRunning != input.atoPumpRelay.enabled) {
    return makeFailsafeOffDecision(
        input, AtoDecision::kReconcilePumpDivergence,
        AtoDecisionReason::kPumpStateDiverged, input.ato.status, false,
        false);
  }

  if (sensorOffline(input, moduleConfig)) {
    return makeFailsafeOffDecision(
        input, AtoDecision::kBlockSensorOffline,
        input.waterLevel.status == core::state::WaterLevelStatus::kSensorOffline
            ? AtoDecisionReason::kSensorOffline
            : AtoDecisionReason::kInvalidSensorReading,
        core::state::AtoStatus::kSensorOffline,
        input.ato.status != core::state::AtoStatus::kSensorOffline, false);
  }

  if (statusCanRecover(input.ato.status) &&
      !input.ato.pumpRunning && !input.atoPumpRelay.enabled &&
      levelWithinAcceptableRange(input)) {
    const AtoDecisionReason reason =
        input.ato.status == core::state::AtoStatus::kTimeout
            ? AtoDecisionReason::kRecoveredFromTimeout
            : AtoDecisionReason::kRecoveredFromSensorOffline;
    return makeDecision(input, AtoDecision::kRecover, reason,
                        core::state::AtoStatus::kNormal, false,
                        AtoPumpCommand::kNone, true,
                        input.ato.lastActivation, input.ato.lastCompletion,
                        input.ato.timeoutCounter);
  }

  if (refillInProgress(input) &&
      elapsedAtLeast(input.nowMillis, input.ato.lastActivation,
                     input.config.timeoutMillis)) {
    return makeFailsafeOffDecision(
        input, AtoDecision::kStopTimeout, AtoDecisionReason::kTimeoutElapsed,
        core::state::AtoStatus::kTimeout, true,
        input.ato.status != core::state::AtoStatus::kTimeout);
  }

  if (refillInProgress(input) &&
      input.waterLevel.currentLevel >= input.config.maximumLevel) {
    return makeDecision(input, AtoDecision::kStopAtMaximumLevel,
                        AtoDecisionReason::kLevelAtOrAboveMaximum,
                        core::state::AtoStatus::kNormal, false,
                        AtoPumpCommand::kTurnOffAutomation, true,
                        input.ato.lastActivation, input.nowMillis,
                        input.ato.timeoutCounter);
  }

  if (!input.ato.pumpRunning && !input.atoPumpRelay.enabled &&
      input.waterLevel.currentLevel <= input.config.minimumLevel) {
    if (cooldownActive(input)) {
      return makeDecision(input, AtoDecision::kBlockCooldown,
                          AtoDecisionReason::kCooldownActive,
                          input.ato.status, false, AtoPumpCommand::kNone,
                          false, input.ato.lastActivation,
                          input.ato.lastCompletion, input.ato.timeoutCounter);
    }

    if (statusCanRecover(input.ato.status)) {
      return makeNoAction(input);
    }

    return makeDecision(input, AtoDecision::kStartRefill,
                        AtoDecisionReason::kLevelAtOrBelowMinimum,
                        core::state::AtoStatus::kRefilling, true,
                        AtoPumpCommand::kTurnOn, true, input.nowMillis,
                        input.ato.lastCompletion, input.ato.timeoutCounter);
  }

  return makeNoAction(input);
}

}  // namespace reeflow::modules::ato
