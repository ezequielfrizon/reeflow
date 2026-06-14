#pragma once

#include "network/wifi_adapter.h"

namespace reeflow::test::fakes {

class FakeInternetProbe final : public reeflow::network::InternetProbe {
 public:
  reeflow::network::InternetProbeResult probe(uint32_t nowMillis) override {
    probeCallCount_ += 1;
    result_.checkedAtMillis = nowMillis;
    return result_;
  }

  void simulateAvailable() {
    result_.status = reeflow::network::InternetStatus::kAvailable;
  }

  void simulateUnavailable() {
    result_.status = reeflow::network::InternetStatus::kUnavailable;
  }

  void simulateDnsFailure() {
    result_.status = reeflow::network::InternetStatus::kDnsFailure;
  }

  void simulateTimeout() {
    result_.status = reeflow::network::InternetStatus::kTimeout;
  }

  uint16_t probeCallCount() const { return probeCallCount_; }

 private:
  reeflow::network::InternetProbeResult result_ = {
      reeflow::network::InternetStatus::kUnknown, 0};
  uint16_t probeCallCount_ = 0;
};

}  // namespace reeflow::test::fakes
