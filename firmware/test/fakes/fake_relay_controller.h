#pragma once

#include <stddef.h>

#include "modules/relays/relay_controller.h"

namespace reeflow::test::fakes {

class FakeRelayController final : public modules::relays::RelayController {
 public:
  enum class RecordedCallType {
    kSetRelay,
    kAllOff,
  };

  struct RecordedCall {
    RecordedCallType type;
    modules::relays::RelayId relay;
    modules::relays::RelayDesiredState desiredState;
  };

  static constexpr size_t kRelayCount = 4;
  static constexpr size_t kMaxRecordedCalls = 32;

  modules::relays::RelayCommandResult setRelay(
      modules::relays::RelayId relay,
      modules::relays::RelayDesiredState desiredState) override {
    recordCall({RecordedCallType::kSetRelay, relay, desiredState});

    const int index = relayIndex(relay);
    if (index < 0) {
      return modules::relays::RelayCommandResult::kUnknownRelay;
    }

    if (forcedResults_[index] !=
        modules::relays::RelayCommandResult::kSuccess) {
      return forcedResults_[index];
    }

    relayStates_[index] = desiredState;
    return modules::relays::RelayCommandResult::kSuccess;
  }

  modules::relays::RelayCommandResult allOff() override {
    ++allOffCallCount_;
    recordCall({RecordedCallType::kAllOff, modules::relays::RelayId::kUnknown,
                modules::relays::RelayDesiredState::kOff});

    if (allOffResult_ != modules::relays::RelayCommandResult::kSuccess) {
      return allOffResult_;
    }

    for (size_t index = 0; index < kRelayCount; ++index) {
      if (forcedResults_[index] !=
          modules::relays::RelayCommandResult::kSuccess) {
        return forcedResults_[index];
      }
    }

    for (size_t index = 0; index < kRelayCount; ++index) {
      relayStates_[index] = modules::relays::RelayDesiredState::kOff;
    }

    return modules::relays::RelayCommandResult::kSuccess;
  }

  void setInitialState(modules::relays::RelayId relay,
                       modules::relays::RelayDesiredState desiredState) {
    const int index = relayIndex(relay);
    if (index >= 0) {
      relayStates_[index] = desiredState;
    }
  }

  void failRelay(modules::relays::RelayId relay) {
    setRelayResult(relay,
                   modules::relays::RelayCommandResult::kControllerFailure);
  }

  void setRelayResult(modules::relays::RelayId relay,
                      modules::relays::RelayCommandResult result) {
    const int index = relayIndex(relay);
    if (index >= 0) {
      forcedResults_[index] = result;
    }
  }

  void setAllOffResult(modules::relays::RelayCommandResult result) {
    allOffResult_ = result;
  }

  modules::relays::RelayDesiredState state(
      modules::relays::RelayId relay) const {
    const int index = relayIndex(relay);
    if (index < 0) {
      return modules::relays::RelayDesiredState::kOff;
    }

    return relayStates_[index];
  }

  size_t recordedCallCount() const {
    return recordedCallCount_;
  }

  size_t allOffCallCount() const {
    return allOffCallCount_;
  }

  const RecordedCall& recordedCall(size_t index) const {
    return recordedCalls_[index];
  }

 private:
  static int relayIndex(modules::relays::RelayId relay) {
    switch (relay) {
      case modules::relays::RelayId::kRecalque:
        return 0;
      case modules::relays::RelayId::kHeater:
        return 1;
      case modules::relays::RelayId::kAtoPump:
        return 2;
      case modules::relays::RelayId::kReserve:
        return 3;
      case modules::relays::RelayId::kUnknown:
        return -1;
    }

    return -1;
  }

  void recordCall(RecordedCall call) {
    if (recordedCallCount_ < kMaxRecordedCalls) {
      recordedCalls_[recordedCallCount_++] = call;
    }
  }

  modules::relays::RelayDesiredState relayStates_[kRelayCount] = {
      modules::relays::RelayDesiredState::kOff,
      modules::relays::RelayDesiredState::kOff,
      modules::relays::RelayDesiredState::kOff,
      modules::relays::RelayDesiredState::kOff,
  };
  modules::relays::RelayCommandResult forcedResults_[kRelayCount] = {
      modules::relays::RelayCommandResult::kSuccess,
      modules::relays::RelayCommandResult::kSuccess,
      modules::relays::RelayCommandResult::kSuccess,
      modules::relays::RelayCommandResult::kSuccess,
  };
  modules::relays::RelayCommandResult allOffResult_ =
      modules::relays::RelayCommandResult::kSuccess;
  RecordedCall recordedCalls_[kMaxRecordedCalls] = {};
  size_t recordedCallCount_ = 0;
  size_t allOffCallCount_ = 0;
};

}  // namespace reeflow::test::fakes
