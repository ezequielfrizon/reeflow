#pragma once

#include "modules/modes/mode_types.h"

namespace reeflow::modules::modes {

enum class ModeAutomation {
  kAto,
};

enum class ModeAutomationGateResult {
  kSuccess,
  kUnknownAutomation,
  kFailed,
};

class ModeAutomationGate {
 public:
  virtual ~ModeAutomationGate() = default;
  virtual ModeAutomationGateResult setAutomationBlocked(
      ModeAutomation automation, bool blocked, OperationalMode mode) = 0;
  virtual bool isAutomationAllowed(ModeAutomation automation) const = 0;
};

class LocalModeAutomationGate final : public ModeAutomationGate {
 public:
  ModeAutomationGateResult setAutomationBlocked(
      ModeAutomation automation, bool blocked, OperationalMode mode) override;
  bool isAutomationAllowed(ModeAutomation automation) const override;
  OperationalMode lastMode() const;

 private:
  bool atoBlocked_ = false;
  OperationalMode lastMode_ = NORMAL;
};

constexpr bool defaultAtoAutomationAllowed(OperationalMode mode) {
  return mode == NORMAL;
}

}  // namespace reeflow::modules::modes
