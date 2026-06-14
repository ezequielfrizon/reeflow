#pragma once

#include "fakes/fake_mqtt_client.h"

namespace reeflow::test::fakes {

class FakeResilienceMqtt {
 public:
  void startOnline() {
    mqtt_.nextConnectResult = mqtt::MqttConnectResult::kConnected;
    mqtt_.nextPublishResult = mqtt::MqttPublishResult::kAccepted;
    mqtt_.setConnected(true);
  }

  void failMqtt() {
    mqtt_.setConnected(false);
    mqtt_.nextConnectResult = mqtt::MqttConnectResult::kBrokerUnavailable;
    mqtt_.nextPublishResult = mqtt::MqttPublishResult::kNotConnected;
  }

  void recoverMqtt() {
    mqtt_.nextConnectResult = mqtt::MqttConnectResult::kConnected;
    mqtt_.nextPublishResult = mqtt::MqttPublishResult::kAccepted;
  }

  FakeMqttClient& mqtt() { return mqtt_; }

 private:
  FakeMqttClient mqtt_;
};

}  // namespace reeflow::test::fakes
