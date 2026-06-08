#include "modules/modes/mode_effects.h"

namespace reeflow::modules::modes {

RelayModeEffects::RelayModeEffects(relays::RelayService& relayService,
                                   ModeAutomationGate& automationGate)
    : relayService_(relayService), automationGate_(automationGate) {}

ModeEffectResult RelayModeEffects::applyModeEffects(
    const ModeEffectsRequest& request) {
  if (!isCanonicalMode(request.mode)) {
    return ModeEffectResult::kRejected;
  }

  if (request.turnOffRecalque) {
    const ModeEffectResult result =
        applyRelayOff(relays::RECALQUE, relays::RelayCommandSource::kAutomation);
    if (result != ModeEffectResult::kSuccess) {
      return result;
    }
  }

  if (request.turnOffAtoPump) {
    const ModeEffectResult result =
        applyRelayOff(relays::ATO_PUMP, relays::RelayCommandSource::kFailsafe);
    if (result != ModeEffectResult::kSuccess) {
      return result;
    }
  }

  const ModeEffectResult gateResult = applyAtoGate(request);
  if (gateResult != ModeEffectResult::kSuccess) {
    return gateResult;
  }

  return ModeEffectResult::kSuccess;
}

ModeEffectResult RelayModeEffects::applyRelayOff(
    relays::RelayId relay, relays::RelayCommandSource source) {
  const relays::RelayCommand command = {relay, relays::RelayDesiredState::kOff,
                                        source};
  const relays::RelayCommandResult result = relayService_.applyCommand(command);
  if (result == relays::RelayCommandResult::kSuccess) {
    return ModeEffectResult::kSuccess;
  }

  if (result == relays::RelayCommandResult::kInvalidContract ||
      result == relays::RelayCommandResult::kUnknownRelay) {
    return ModeEffectResult::kRejected;
  }

  return ModeEffectResult::kFailed;
}

ModeEffectResult RelayModeEffects::applyAtoGate(
    const ModeEffectsRequest& request) {
  if (!request.updateAtoAutomationGate && !request.releaseModeBlocks) {
    return ModeEffectResult::kSuccess;
  }

  const bool blocked =
      request.releaseModeBlocks ? false : request.blockAtoAutomation;
  const ModeAutomationGateResult result = automationGate_.setAutomationBlocked(
      ModeAutomation::kAto, blocked, request.mode);
  if (result == ModeAutomationGateResult::kSuccess) {
    return ModeEffectResult::kSuccess;
  }

  if (result == ModeAutomationGateResult::kUnknownAutomation) {
    return ModeEffectResult::kRejected;
  }

  return ModeEffectResult::kFailed;
}

}  // namespace reeflow::modules::modes
