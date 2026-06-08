#include "modules/modes/mode_service.h"

#include "core/state/system_state.h"

namespace reeflow::modules::modes {
namespace {

bool planRequiresEffects(const ModePolicyPlan& plan) {
  return plan.effectPhase == ModePolicyEffectPhase::kEnter ||
         plan.effectPhase == ModePolicyEffectPhase::kExit;
}

ModeServiceResult serviceResultForPolicyDecision(
    ModePolicyDecision decision) {
  switch (decision) {
    case ModePolicyDecision::kRejectInvalidCommand:
      return ModeServiceResult::kInvalidCommand;
    case ModePolicyDecision::kRejectInvalidConfig:
      return ModeServiceResult::kInvalidConfig;
    case ModePolicyDecision::kNoChange:
    case ModePolicyDecision::kUpdateRemainingTime:
    case ModePolicyDecision::kAcceptTransition:
      return ModeServiceResult::kSuccess;
  }

  return ModeServiceResult::kInvalidCommand;
}

ModeServiceResult serviceResultForEffectResult(ModeEffectResult result) {
  switch (result) {
    case ModeEffectResult::kSuccess:
      return ModeServiceResult::kSuccess;
    case ModeEffectResult::kRejected:
      return ModeServiceResult::kInvalidCommand;
    case ModeEffectResult::kFailed:
      return ModeServiceResult::kEffectsFailed;
  }

  return ModeServiceResult::kEffectsFailed;
}

bool modeWasChanged(const ModePolicyPlan& plan) {
  return plan.decision == ModePolicyDecision::kAcceptTransition;
}

}  // namespace

ModeService::ModeService(const config::ConfigManager& configManager,
                         const core::platform::TimeSource& timeSource,
                         core::events::EventBus& eventBus, ModeEffects& effects,
                         ModeStore& store)
    : configManager_(configManager),
      timeSource_(timeSource),
      eventBus_(eventBus),
      effects_(effects),
      store_(store) {}

ModeServiceResult ModeService::requestMode(const ModeCommand& command) {
  return applyPlan(evaluateModePolicy(makePolicyInput(true, command))).result;
}

ModeServiceTickResult ModeService::evaluateOnce() {
  return applyPlan(evaluateModePolicy(makePolicyInput(false, {})));
}

ModeServiceResult ModeService::restoreModeFromStore() {
  const ModeStoreLoadResult loadResult = store_.loadMode();
  switch (loadResult.status) {
    case ModeStoreLoadStatus::kNotFound:
      core::state::updateModesState({NORMAL, 0, 0});
      return ModeServiceResult::kSuccess;
    case ModeStoreLoadStatus::kLoaded:
      return applyRestoredMode(loadResult.mode);
    case ModeStoreLoadStatus::kInvalidMode:
    case ModeStoreLoadStatus::kFailure:
      core::state::updateModesState({NORMAL, 0, 0});
      return ModeServiceResult::kStoreFailed;
  }

  core::state::updateModesState({NORMAL, 0, 0});
  return ModeServiceResult::kStoreFailed;
}

ModeServiceTickResult ModeService::applyPlan(const ModePolicyPlan& plan) {
  ModeServiceTickResult result = {};
  result.result = serviceResultForPolicyDecision(plan.decision);
  result.modeChanged = false;

  if (result.result != ModeServiceResult::kSuccess ||
      plan.decision == ModePolicyDecision::kNoChange) {
    return result;
  }

  if (planRequiresEffects(plan)) {
    result.result =
        serviceResultForEffectResult(effects_.applyModeEffects(plan.effects));
    if (result.result != ModeServiceResult::kSuccess) {
      return result;
    }
  }

  core::state::updateModesState(plan.nextModes);
  result.modeChanged = modeWasChanged(plan);

  if (modeWasChanged(plan)) {
    publishModeEvents(plan);
  }

  result.result = saveModeIfNeeded(plan);
  return result;
}

ModeServiceResult ModeService::applyRestoredMode(OperationalMode mode) {
  if (!isCanonicalMode(mode)) {
    core::state::updateModesState({NORMAL, 0, 0});
    return ModeServiceResult::kInvalidCommand;
  }

  if (mode == NORMAL || mode == FEEDING) {
    core::state::updateModesState({NORMAL, 0, 0});
    return ModeServiceResult::kSuccess;
  }

  const ModeCommand command =
      makeLocalModeCommand(mode, timeSource_.uptimeMillis(),
                           ModeCommandReason::kBootRestore);
  ModePolicyPlan plan = evaluateModePolicy(makePolicyInput(true, command));
  if (plan.decision != ModePolicyDecision::kAcceptTransition) {
    core::state::updateModesState({NORMAL, 0, 0});
    return serviceResultForPolicyDecision(plan.decision);
  }

  if (planRequiresEffects(plan)) {
    const ModeServiceResult effectResult =
        serviceResultForEffectResult(effects_.applyModeEffects(plan.effects));
    if (effectResult != ModeServiceResult::kSuccess) {
      core::state::updateModesState({NORMAL, 0, 0});
      return effectResult;
    }
  }

  core::state::updateModesState(plan.nextModes);
  return ModeServiceResult::kSuccess;
}

ModeServiceResult ModeService::saveModeIfNeeded(const ModePolicyPlan& plan) {
  if (!plan.saveMode) {
    return ModeServiceResult::kSuccess;
  }

  const ModeStoreSaveResult saveResult =
      store_.saveMode(plan.nextModes.currentMode);
  if (saveResult == ModeStoreSaveResult::kSuccess) {
    return ModeServiceResult::kSuccess;
  }

  return ModeServiceResult::kStoreFailed;
}

void ModeService::publishModeEvents(const ModePolicyPlan& plan) {
  if (plan.emitModeFinished) {
    eventBus_.publish(makeModeCoreEvent(ModeEventType::kModeFinished));
  }

  if (plan.emitModeStarted) {
    eventBus_.publish(makeModeCoreEvent(ModeEventType::kModeStarted));
  }

  if (plan.emitModeChanged) {
    eventBus_.publish(makeModeCoreEvent(ModeEventType::kModeChanged));
  }
}

ModePolicyInput ModeService::makePolicyInput(
    bool hasCommand, const ModeCommand& command) const {
  ModePolicyInput input = {};
  input.currentModes = core::state::currentSystemState().modes;
  input.config = makeModeBehaviorConfig(configManager_);
  input.nowMillis = timeSource_.uptimeMillis();
  input.command = command;
  input.hasCommand = hasCommand;
  return input;
}

bool runModeServiceTask(void* context) {
  if (context == nullptr) {
    return false;
  }

  ModeService* service = static_cast<ModeService*>(context);
  return service->evaluateOnce().result == ModeServiceResult::kSuccess;
}

}  // namespace reeflow::modules::modes
