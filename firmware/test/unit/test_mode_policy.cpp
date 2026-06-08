#include <assert.h>

#include "config/config_manager.h"
#include "modules/modes/mode_policy.h"

namespace {

using reeflow::config::makeDefaultReeflowConfig;
using reeflow::core::state::ModesState;
using reeflow::modules::modes::FEEDING;
using reeflow::modules::modes::MAINTENANCE;
using reeflow::modules::modes::NORMAL;
using reeflow::modules::modes::TPA;
using reeflow::modules::modes::ModeFinishReason;
using reeflow::modules::modes::ModePolicyDecision;
using reeflow::modules::modes::ModePolicyEffectPhase;
using reeflow::modules::modes::ModePolicyInput;
using reeflow::modules::modes::ModeReturnBehavior;
using reeflow::modules::modes::evaluateModePolicy;
using reeflow::modules::modes::makeLocalModeCommand;
using reeflow::modules::modes::makeModeBehaviorConfig;

constexpr uint32_t kNow = 10000;

ModesState modes(reeflow::modules::modes::OperationalMode mode,
                 uint32_t startedAt = 0, uint32_t remainingTime = 0) {
  ModesState state = {};
  state.currentMode = mode;
  state.startedAt = startedAt;
  state.remainingTime = remainingTime;
  return state;
}

ModePolicyInput inputFor(ModesState currentModes, uint32_t nowMillis = kNow) {
  ModePolicyInput input = {};
  input.currentModes = currentModes;
  input.config = makeModeBehaviorConfig(makeDefaultReeflowConfig().timers);
  input.nowMillis = nowMillis;
  return input;
}

ModePolicyInput commandInput(ModesState currentModes,
                             reeflow::modules::modes::OperationalMode mode) {
  ModePolicyInput input = inputFor(currentModes);
  input.command = makeLocalModeCommand(mode, kNow);
  input.hasCommand = true;
  return input;
}

void assertTransitionToNormalDoesNotTurnRelaysOn(
    const reeflow::modules::modes::ModePolicyPlan& plan) {
  assert(plan.nextModes.currentMode == NORMAL);
  assert(!plan.effects.turnOffRecalque);
  assert(!plan.effects.turnOffAtoPump);
  assert(plan.effects.releaseModeBlocks);
  assert(!plan.blockAtoAutomation);
  assert(plan.updateAtoAutomationGate);
}

void testTransitionNormalToFeeding() {
  const auto plan = evaluateModePolicy(commandInput(modes(NORMAL), FEEDING));

  assert(plan.decision == ModePolicyDecision::kAcceptTransition);
  assert(plan.nextModes.currentMode == FEEDING);
  assert(plan.nextModes.startedAt == kNow);
  assert(plan.nextModes.remainingTime == 600);
  assert(plan.effects.mode == FEEDING);
  assert(plan.effects.turnOffRecalque);
  assert(!plan.effects.turnOffAtoPump);
  assert(plan.blockAtoAutomation);
  assert(plan.updateAtoAutomationGate);
  assert(plan.effectPhase == ModePolicyEffectPhase::kEnter);
  assert(plan.saveMode);
  assert(plan.emitModeChanged);
  assert(plan.emitModeStarted);
  assert(!plan.emitModeFinished);
}

void testTransitionNormalToTpa() {
  const auto plan = evaluateModePolicy(commandInput(modes(NORMAL), TPA));

  assert(plan.decision == ModePolicyDecision::kAcceptTransition);
  assert(plan.nextModes.currentMode == TPA);
  assert(plan.nextModes.startedAt == kNow);
  assert(plan.nextModes.remainingTime == 0);
  assert(plan.effects.turnOffRecalque);
  assert(plan.effects.turnOffAtoPump);
  assert(plan.blockAtoAutomation);
  assert(plan.updateAtoAutomationGate);
  assert(plan.effectPhase == ModePolicyEffectPhase::kEnter);
  assert(plan.emitModeChanged);
  assert(plan.emitModeStarted);
}

void testTransitionNormalToMaintenance() {
  const auto plan =
      evaluateModePolicy(commandInput(modes(NORMAL), MAINTENANCE));

  assert(plan.decision == ModePolicyDecision::kAcceptTransition);
  assert(plan.nextModes.currentMode == MAINTENANCE);
  assert(plan.nextModes.startedAt == kNow);
  assert(plan.nextModes.remainingTime == 0);
  assert(!plan.effects.turnOffRecalque);
  assert(!plan.effects.turnOffAtoPump);
  assert(plan.effects.preserveManualRelayControl);
  assert(plan.blockAtoAutomation);
  assert(plan.updateAtoAutomationGate);
  assert(plan.effectPhase == ModePolicyEffectPhase::kEnter);
  assert(plan.emitModeChanged);
  assert(plan.emitModeStarted);
}

void testManualReturnToNormalFromFeedingTpaAndMaintenance() {
  const auto feedingPlan =
      evaluateModePolicy(commandInput(modes(FEEDING, 1000, 500), NORMAL));
  const auto tpaPlan = evaluateModePolicy(commandInput(modes(TPA), NORMAL));
  const auto maintenancePlan =
      evaluateModePolicy(commandInput(modes(MAINTENANCE), NORMAL));

  assert(feedingPlan.decision == ModePolicyDecision::kAcceptTransition);
  assertTransitionToNormalDoesNotTurnRelaysOn(feedingPlan);
  assert(feedingPlan.finishReason == ModeFinishReason::kManual);
  assert(feedingPlan.emitModeFinished);
  assert(!feedingPlan.emitModeStarted);

  assert(tpaPlan.decision == ModePolicyDecision::kAcceptTransition);
  assertTransitionToNormalDoesNotTurnRelaysOn(tpaPlan);
  assert(tpaPlan.emitModeFinished);

  assert(maintenancePlan.decision == ModePolicyDecision::kAcceptTransition);
  assertTransitionToNormalDoesNotTurnRelaysOn(maintenancePlan);
  assert(maintenancePlan.emitModeFinished);
}

void testIdempotentCommandForEachMode() {
  const ModesState states[] = {
      modes(NORMAL, 10, 0),
      modes(FEEDING, 20, 500),
      modes(TPA, 30, 0),
      modes(MAINTENANCE, 40, 0),
  };

  for (const ModesState& state : states) {
    const auto plan =
        evaluateModePolicy(commandInput(state, state.currentMode));

    assert(plan.decision == ModePolicyDecision::kNoChange);
    assert(plan.nextModes.currentMode == state.currentMode);
    assert(plan.nextModes.startedAt == state.startedAt);
    assert(plan.nextModes.remainingTime == state.remainingTime);
    assert(!plan.effects.turnOffRecalque);
    assert(!plan.effects.turnOffAtoPump);
    assert(!plan.updateAtoAutomationGate);
    assert(!plan.saveMode);
    assert(!plan.emitModeChanged);
    assert(!plan.emitModeStarted);
    assert(!plan.emitModeFinished);
  }
}

void testFeedingRemainingTimeBeforeExpiration() {
  const auto plan =
      evaluateModePolicy(inputFor(modes(FEEDING, 1000, 600), 301000));

  assert(plan.decision == ModePolicyDecision::kUpdateRemainingTime);
  assert(plan.nextModes.currentMode == FEEDING);
  assert(plan.nextModes.startedAt == 1000);
  assert(plan.nextModes.remainingTime == 300);
  assert(plan.effectPhase == ModePolicyEffectPhase::kStay);
  assert(!plan.effects.turnOffRecalque);
  assert(!plan.updateAtoAutomationGate);
  assert(!plan.saveMode);
  assert(!plan.emitModeChanged);
}

void testFeedingExactlyAtExpirationReturnsToNormal() {
  const auto plan =
      evaluateModePolicy(inputFor(modes(FEEDING, 1000, 1), 601000));

  assert(plan.decision == ModePolicyDecision::kAcceptTransition);
  assertTransitionToNormalDoesNotTurnRelaysOn(plan);
  assert(plan.finishReason == ModeFinishReason::kAutomaticTimeout);
  assert(plan.emitModeChanged);
  assert(plan.emitModeFinished);
  assert(!plan.emitModeStarted);
}

void testFeedingAfterExpirationReturnsToNormal() {
  const auto plan =
      evaluateModePolicy(inputFor(modes(FEEDING, 1000, 1), 700000));

  assert(plan.decision == ModePolicyDecision::kAcceptTransition);
  assertTransitionToNormalDoesNotTurnRelaysOn(plan);
  assert(plan.finishReason == ModeFinishReason::kAutomaticTimeout);
}

void testFeedingZeroDurationIsRejected() {
  ModePolicyInput input = commandInput(modes(NORMAL), FEEDING);
  input.config.feedingDurationSeconds = 0;

  const auto plan = evaluateModePolicy(input);

  assert(plan.decision == ModePolicyDecision::kRejectInvalidConfig);
  assert(plan.nextModes.currentMode == NORMAL);
  assert(!plan.saveMode);
  assert(!plan.emitModeChanged);
}

void testTpaAndMaintenanceDoNotReturnAutomatically() {
  const auto tpaPlan = evaluateModePolicy(inputFor(modes(TPA, 1000), 900000));
  const auto maintenancePlan =
      evaluateModePolicy(inputFor(modes(MAINTENANCE, 1000), 900000));

  assert(tpaPlan.decision == ModePolicyDecision::kNoChange);
  assert(tpaPlan.nextModes.currentMode == TPA);
  assert(!tpaPlan.emitModeFinished);

  assert(maintenancePlan.decision == ModePolicyDecision::kNoChange);
  assert(maintenancePlan.nextModes.currentMode == MAINTENANCE);
  assert(!maintenancePlan.emitModeFinished);
}

void testInvalidCommandsAndConfigAreRejectedDeterministically() {
  ModePolicyInput invalidCommand =
      commandInput(modes(NORMAL),
                   static_cast<reeflow::modules::modes::OperationalMode>(255));
  assert(evaluateModePolicy(invalidCommand).decision ==
         ModePolicyDecision::kRejectInvalidCommand);

  ModePolicyInput invalidCurrent =
      inputFor(modes(static_cast<reeflow::modules::modes::OperationalMode>(255)));
  assert(evaluateModePolicy(invalidCurrent).decision ==
         ModePolicyDecision::kRejectInvalidCommand);

  ModePolicyInput tpaInput = commandInput(modes(NORMAL), TPA);
  tpaInput.config.tpaReturn = ModeReturnBehavior::kAutomatic;
  assert(evaluateModePolicy(tpaInput).decision ==
         ModePolicyDecision::kRejectInvalidConfig);

  ModePolicyInput maintenanceInput = commandInput(modes(NORMAL), MAINTENANCE);
  maintenanceInput.config.maintenanceReturn = ModeReturnBehavior::kAutomatic;
  assert(evaluateModePolicy(maintenanceInput).decision ==
         ModePolicyDecision::kRejectInvalidConfig);
}

}  // namespace

int main() {
  testTransitionNormalToFeeding();
  testTransitionNormalToTpa();
  testTransitionNormalToMaintenance();
  testManualReturnToNormalFromFeedingTpaAndMaintenance();
  testIdempotentCommandForEachMode();
  testFeedingRemainingTimeBeforeExpiration();
  testFeedingExactlyAtExpirationReturnsToNormal();
  testFeedingAfterExpirationReturnsToNormal();
  testFeedingZeroDurationIsRejected();
  testTpaAndMaintenanceDoNotReturnAutomatically();
  testInvalidCommandsAndConfigAreRejectedDeterministically();
  return 0;
}
