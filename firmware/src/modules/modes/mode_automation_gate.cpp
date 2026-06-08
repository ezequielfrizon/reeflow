#include "modules/modes/mode_automation_gate.h"

namespace reeflow::modules::modes {

ModeAutomationGateResult LocalModeAutomationGate::setAutomationBlocked(
    ModeAutomation automation, bool blocked, OperationalMode mode) {
  if (automation != ModeAutomation::kAto || !isCanonicalMode(mode)) {
    return ModeAutomationGateResult::kUnknownAutomation;
  }

  atoBlocked_ = blocked;
  lastMode_ = mode;
  return ModeAutomationGateResult::kSuccess;
}

bool LocalModeAutomationGate::isAutomationAllowed(
    ModeAutomation automation) const {
  if (automation != ModeAutomation::kAto) {
    return false;
  }

  return !atoBlocked_;
}

OperationalMode LocalModeAutomationGate::lastMode() const {
  return lastMode_;
}

}  // namespace reeflow::modules::modes
