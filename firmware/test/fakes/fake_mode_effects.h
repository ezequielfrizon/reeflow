#pragma once

#include <stddef.h>

#include "modules/modes/mode_effects.h"

namespace reeflow::test::fakes {

class FakeModeEffects final : public modules::modes::ModeEffects {
 public:
  static constexpr size_t kMaxRecordedRequests = 16;

  modules::modes::ModeEffectResult applyModeEffects(
      const modules::modes::ModeEffectsRequest& request) override {
    if (requestCount_ < kMaxRecordedRequests) {
      requests_[requestCount_] = request;
    }
    ++requestCount_;
    return result_;
  }

  void simulateFailure() {
    result_ = modules::modes::ModeEffectResult::kFailed;
  }

  void simulateRejected() {
    result_ = modules::modes::ModeEffectResult::kRejected;
  }

  void simulateSuccess() {
    result_ = modules::modes::ModeEffectResult::kSuccess;
  }

  size_t requestCount() const {
    return requestCount_;
  }

  const modules::modes::ModeEffectsRequest& request(size_t index) const {
    return requests_[index];
  }

 private:
  modules::modes::ModeEffectResult result_ =
      modules::modes::ModeEffectResult::kSuccess;
  modules::modes::ModeEffectsRequest requests_[kMaxRecordedRequests] = {};
  size_t requestCount_ = 0;
};

}  // namespace reeflow::test::fakes
