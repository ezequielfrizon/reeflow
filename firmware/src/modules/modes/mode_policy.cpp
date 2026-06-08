#include "modules/modes/mode_policy.h"

namespace reeflow::modules::modes {
namespace {

constexpr uint32_t kMillisPerSecond = 1000;

ModePolicyPlan basePlan(const ModePolicyInput& input) {
  ModePolicyPlan plan = {};
  plan.decision = ModePolicyDecision::kNoChange;
  plan.nextModes = input.currentModes;
  plan.effects =
      makeNoOpModeEffectsRequest(input.currentModes.currentMode,
                                 input.nowMillis);
  plan.effectPhase = ModePolicyEffectPhase::kNone;
  plan.finishReason = ModeFinishReason::kNone;
  return plan;
}

uint32_t elapsedSeconds(uint32_t startedAtMillis, uint32_t nowMillis) {
  if (nowMillis <= startedAtMillis) {
    return 0;
  }

  return (nowMillis - startedAtMillis) / kMillisPerSecond;
}

core::state::ModesState modeState(OperationalMode mode,
                                  uint32_t startedAtMillis,
                                  uint32_t remainingTimeSeconds) {
  core::state::ModesState state = {};
  state.currentMode = mode;
  state.startedAt = startedAtMillis;
  state.remainingTime = remainingTimeSeconds;
  return state;
}

bool commandIsValid(const ModeCommand& command) {
  return command.source == ModeCommandSource::kLocal &&
         isCanonicalMode(command.requestedMode);
}

void markEffectiveTransition(ModePolicyPlan& plan, OperationalMode previousMode,
                             OperationalMode nextMode) {
  plan.decision = ModePolicyDecision::kAcceptTransition;
  plan.saveMode = true;
  plan.emitModeChanged = true;
  plan.emitModeStarted = nextMode != NORMAL;
  plan.emitModeFinished = previousMode != NORMAL;
}

ModePolicyPlan enterNormal(const ModePolicyInput& input,
                           ModeFinishReason finishReason) {
  ModePolicyPlan plan = basePlan(input);
  const OperationalMode previousMode = input.currentModes.currentMode;
  plan.nextModes = modeState(NORMAL, input.nowMillis, 0);
  plan.effects = makeNoOpModeEffectsRequest(NORMAL, input.nowMillis);
  plan.effects.releaseModeBlocks = input.config.normalReleasesModeBlocks;
  plan.effects.updateAtoAutomationGate = input.config.normalReleasesModeBlocks;
  plan.effects.blockAtoAutomation = false;
  plan.effectPhase = previousMode == NORMAL ? ModePolicyEffectPhase::kEnter
                                            : ModePolicyEffectPhase::kExit;
  plan.finishReason = finishReason;
  plan.blockAtoAutomation = false;
  plan.updateAtoAutomationGate = input.config.normalReleasesModeBlocks;
  markEffectiveTransition(plan, previousMode, NORMAL);
  plan.emitModeStarted = false;
  return plan;
}

ModePolicyPlan enterFeeding(const ModePolicyInput& input) {
  ModePolicyPlan plan = basePlan(input);
  if (input.config.feedingDurationSeconds == 0 ||
      input.config.feedingReturn != ModeReturnBehavior::kAutomatic) {
    plan.decision = ModePolicyDecision::kRejectInvalidConfig;
    return plan;
  }

  plan.nextModes =
      modeState(FEEDING, input.nowMillis, input.config.feedingDurationSeconds);
  plan.effects = makeNoOpModeEffectsRequest(FEEDING, input.nowMillis);
  plan.effects.turnOffRecalque = input.config.feedingTurnsOffRecalque;
  plan.effects.updateAtoAutomationGate = input.config.feedingBlocksAtoAutomation;
  plan.effects.blockAtoAutomation = input.config.feedingBlocksAtoAutomation;
  plan.effectPhase = ModePolicyEffectPhase::kEnter;
  plan.blockAtoAutomation = input.config.feedingBlocksAtoAutomation;
  plan.updateAtoAutomationGate = input.config.feedingBlocksAtoAutomation;
  markEffectiveTransition(plan, input.currentModes.currentMode, FEEDING);
  return plan;
}

ModePolicyPlan enterTpa(const ModePolicyInput& input) {
  ModePolicyPlan plan = basePlan(input);
  if (input.config.tpaReturn != ModeReturnBehavior::kManual) {
    plan.decision = ModePolicyDecision::kRejectInvalidConfig;
    return plan;
  }

  plan.nextModes = modeState(TPA, input.nowMillis, 0);
  plan.effects = makeNoOpModeEffectsRequest(TPA, input.nowMillis);
  plan.effects.turnOffRecalque = input.config.tpaTurnsOffRecalque;
  plan.effects.turnOffAtoPump = input.config.tpaTurnsOffAtoPump;
  plan.effects.updateAtoAutomationGate = input.config.tpaBlocksAtoAutomation;
  plan.effects.blockAtoAutomation = input.config.tpaBlocksAtoAutomation;
  plan.effectPhase = ModePolicyEffectPhase::kEnter;
  plan.blockAtoAutomation = input.config.tpaBlocksAtoAutomation;
  plan.updateAtoAutomationGate = input.config.tpaBlocksAtoAutomation;
  markEffectiveTransition(plan, input.currentModes.currentMode, TPA);
  return plan;
}

ModePolicyPlan enterMaintenance(const ModePolicyInput& input) {
  ModePolicyPlan plan = basePlan(input);
  if (input.config.maintenanceReturn != ModeReturnBehavior::kManual) {
    plan.decision = ModePolicyDecision::kRejectInvalidConfig;
    return plan;
  }

  plan.nextModes = modeState(MAINTENANCE, input.nowMillis, 0);
  plan.effects = makeNoOpModeEffectsRequest(MAINTENANCE, input.nowMillis);
  plan.effects.preserveManualRelayControl = true;
  plan.effects.updateAtoAutomationGate =
      input.config.maintenanceBlocksNonCriticalAutomations;
  plan.effects.blockAtoAutomation =
      input.config.maintenanceBlocksNonCriticalAutomations &&
      input.config.maintenanceBlocksAtoAutomation;
  plan.effectPhase = ModePolicyEffectPhase::kEnter;
  plan.blockAtoAutomation =
      input.config.maintenanceBlocksNonCriticalAutomations &&
      input.config.maintenanceBlocksAtoAutomation;
  plan.updateAtoAutomationGate =
      input.config.maintenanceBlocksNonCriticalAutomations;
  markEffectiveTransition(plan, input.currentModes.currentMode, MAINTENANCE);
  return plan;
}

ModePolicyPlan evaluateFeedingTimer(const ModePolicyInput& input) {
  ModePolicyPlan plan = basePlan(input);
  if (input.config.feedingDurationSeconds == 0) {
    plan.decision = ModePolicyDecision::kRejectInvalidConfig;
    return plan;
  }

  const uint32_t elapsed =
      elapsedSeconds(input.currentModes.startedAt, input.nowMillis);
  if (elapsed >= input.config.feedingDurationSeconds) {
    return enterNormal(input, ModeFinishReason::kAutomaticTimeout);
  }

  const uint32_t remaining = input.config.feedingDurationSeconds - elapsed;
  if (remaining != input.currentModes.remainingTime) {
    plan.decision = ModePolicyDecision::kUpdateRemainingTime;
    plan.nextModes.remainingTime = remaining;
    plan.effectPhase = ModePolicyEffectPhase::kStay;
  }

  return plan;
}

ModePolicyPlan transitionTo(const ModePolicyInput& input,
                            OperationalMode requestedMode,
                            ModeFinishReason finishReason) {
  switch (requestedMode) {
    case OperationalMode::kNormal:
      return enterNormal(input, finishReason);
    case OperationalMode::kFeeding:
      return enterFeeding(input);
    case OperationalMode::kTpa:
      return enterTpa(input);
    case OperationalMode::kMaintenance:
      return enterMaintenance(input);
  }

  ModePolicyPlan plan = basePlan(input);
  plan.decision = ModePolicyDecision::kRejectInvalidCommand;
  return plan;
}

}  // namespace

ModePolicyPlan evaluateModePolicy(const ModePolicyInput& input) {
  ModePolicyPlan plan = basePlan(input);
  if (!isCanonicalMode(input.currentModes.currentMode)) {
    plan.decision = ModePolicyDecision::kRejectInvalidCommand;
    return plan;
  }

  if (input.hasCommand) {
    if (!commandIsValid(input.command)) {
      plan.decision = ModePolicyDecision::kRejectInvalidCommand;
      return plan;
    }

    if (input.command.requestedMode == input.currentModes.currentMode) {
      return plan;
    }

    const ModeFinishReason finishReason =
        input.command.requestedMode == NORMAL ? ModeFinishReason::kManual
                                              : ModeFinishReason::kNone;
    return transitionTo(input, input.command.requestedMode, finishReason);
  }

  if (input.currentModes.currentMode == FEEDING) {
    return evaluateFeedingTimer(input);
  }

  return plan;
}

}  // namespace reeflow::modules::modes
