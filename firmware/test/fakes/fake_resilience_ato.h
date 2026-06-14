#pragma once

#include <stdint.h>

#include "core/state/system_state.h"

namespace reeflow::test::fakes {

class FakeResilienceAto {
 public:
  void startAllowed() {
    pumpRunning_ = false;
    timedOut_ = false;
    cooldown_ = false;
  }

  void simulatePumpRunning() { pumpRunning_ = true; }

  void simulateTimeout(uint32_t nowMillis) {
    pumpRunning_ = false;
    timedOut_ = true;
    cooldown_ = true;
    lastTimeoutMillis_ = nowMillis;
  }

  core::state::AtoState snapshot() const {
    core::state::AtoState state = {};
    state.enabled = true;
    state.status = timedOut_ ? core::state::AtoStatus::kTimeout
                             : core::state::AtoStatus::kNormal;
    state.pumpRunning = pumpRunning_;
    state.timeoutCounter = timedOut_ ? 1 : 0;
    state.lastActivation = pumpRunning_ ? lastTimeoutMillis_ : 0;
    state.lastCompletion = cooldown_ ? lastTimeoutMillis_ : 0;
    return state;
  }

  bool pumpRunning() const { return pumpRunning_; }
  bool timedOut() const { return timedOut_; }
  bool cooldown() const { return cooldown_; }

 private:
  bool pumpRunning_ = false;
  bool timedOut_ = false;
  bool cooldown_ = false;
  uint32_t lastTimeoutMillis_ = 0;
};

}  // namespace reeflow::test::fakes
