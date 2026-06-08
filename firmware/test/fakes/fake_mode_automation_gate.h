#pragma once

#include <stddef.h>

#include "modules/modes/mode_automation_gate.h"

namespace reeflow::test::fakes {

class FakeModeAutomationGate final
    : public modules::modes::ModeAutomationGate {
 public:
  struct RecordedCall {
    modules::modes::ModeAutomation automation;
    bool blocked;
    modules::modes::OperationalMode mode;
  };

  static constexpr size_t kMaxRecordedCalls = 16;

  modules::modes::ModeAutomationGateResult setAutomationBlocked(
      modules::modes::ModeAutomation automation, bool blocked,
      modules::modes::OperationalMode mode) override {
    recordCall({automation, blocked, mode});

    if (automation != modules::modes::ModeAutomation::kAto) {
      return modules::modes::ModeAutomationGateResult::kUnknownAutomation;
    }

    if (result_ != modules::modes::ModeAutomationGateResult::kSuccess) {
      return result_;
    }

    atoBlocked_ = blocked;
    return modules::modes::ModeAutomationGateResult::kSuccess;
  }

  bool isAutomationAllowed(
      modules::modes::ModeAutomation automation) const override {
    if (automation != modules::modes::ModeAutomation::kAto) {
      return false;
    }

    return !atoBlocked_;
  }

  void simulateFailure() {
    result_ = modules::modes::ModeAutomationGateResult::kFailed;
  }

  void simulateSuccess() {
    result_ = modules::modes::ModeAutomationGateResult::kSuccess;
  }

  size_t callCount() const {
    return callCount_;
  }

  const RecordedCall& call(size_t index) const {
    return calls_[index];
  }

 private:
  void recordCall(RecordedCall call) {
    if (callCount_ < kMaxRecordedCalls) {
      calls_[callCount_] = call;
    }
    ++callCount_;
  }

  modules::modes::ModeAutomationGateResult result_ =
      modules::modes::ModeAutomationGateResult::kSuccess;
  bool atoBlocked_ = false;
  RecordedCall calls_[kMaxRecordedCalls] = {};
  size_t callCount_ = 0;
};

}  // namespace reeflow::test::fakes
