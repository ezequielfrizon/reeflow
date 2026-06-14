#pragma once

#include "alerts/alert_mqtt_publisher.h"

namespace reeflow::test::fakes {

class FakeAlertMqttSink final : public alerts::AlertMqttSink {
 public:
  bool available() const override { return available_; }

  alerts::AlertPublishResult publishAlert(
      const alerts::AlertHistoryEntry& entry) override {
    publishCallCount += 1;
    lastEntry = entry;

    if (!available_) {
      return alerts::AlertPublishResult::kUnavailable;
    }

    return nextResult;
  }

  void setAvailable(bool available) { available_ = available; }

  alerts::AlertPublishResult nextResult =
      alerts::AlertPublishResult::kPublished;
  alerts::AlertHistoryEntry lastEntry = {};
  uint32_t publishCallCount = 0;

 private:
  bool available_ = true;
};

}  // namespace reeflow::test::fakes
