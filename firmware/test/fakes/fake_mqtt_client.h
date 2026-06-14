#pragma once

#include <stdint.h>

#include "mqtt/mqtt_client.h"

namespace reeflow::test::fakes {

class FakeMqttClient final : public mqtt::MqttClient {
 public:
  void configure(const mqtt::MqttConfig& config) override {
    config_ = config;
    configureCallCount += 1;
  }

  mqtt::MqttConnectResult connect() override {
    connectCallCount += 1;
    if (nextConnectResult == mqtt::MqttConnectResult::kConnected ||
        nextConnectResult == mqtt::MqttConnectResult::kAlreadyConnected) {
      connected_ = true;
    }
    return nextConnectResult;
  }

  void disconnect(mqtt::MqttDisconnectReason reason) override {
    disconnectCallCount += 1;
    lastDisconnectReason = reason;
    connected_ = false;
  }

  bool connected() const override { return connected_; }

  mqtt::MqttPublishResult publish(const mqtt::MqttMessage& message) override {
    publishCallCount += 1;
    lastPublishedMessage = message;
    return nextPublishResult;
  }

  mqtt::MqttSubscribeResult subscribe(const char* topic,
                                      mqtt::MqttQos qos) override {
    subscribeCallCount += 1;
    lastSubscribedTopic = topic;
    lastSubscribedQos = qos;
    return nextSubscribeResult;
  }

  void setMessageHandler(mqtt::MqttMessageHandler handler,
                         void* context) override {
    messageHandler_ = handler;
    messageHandlerContext_ = context;
  }

  void loop(uint32_t nowMillis) override {
    loopCallCount += 1;
    lastLoopMillis = nowMillis;
  }

  void deliverMessage(const mqtt::MqttMessage& message) {
    deliveredMessageCount += 1;
    if (messageHandler_ != nullptr) {
      messageHandler_(message, messageHandlerContext_);
    }
  }

  void setConnected(bool connected) { connected_ = connected; }

  mqtt::MqttConfig config_ = mqtt::makeDefaultMqttConfig();
  mqtt::MqttConnectResult nextConnectResult =
      mqtt::MqttConnectResult::kConnected;
  mqtt::MqttPublishResult nextPublishResult =
      mqtt::MqttPublishResult::kAccepted;
  mqtt::MqttSubscribeResult nextSubscribeResult =
      mqtt::MqttSubscribeResult::kSubscribed;
  mqtt::MqttDisconnectReason lastDisconnectReason =
      mqtt::MqttDisconnectReason::kUnknown;
  mqtt::MqttMessage lastPublishedMessage = {};
  const char* lastSubscribedTopic = nullptr;
  mqtt::MqttQos lastSubscribedQos = mqtt::MqttQos::kAtMostOnce;
  uint32_t configureCallCount = 0;
  uint32_t connectCallCount = 0;
  uint32_t disconnectCallCount = 0;
  uint32_t publishCallCount = 0;
  uint32_t subscribeCallCount = 0;
  uint32_t loopCallCount = 0;
  uint32_t deliveredMessageCount = 0;
  uint32_t lastLoopMillis = 0;

 private:
  bool connected_ = false;
  mqtt::MqttMessageHandler messageHandler_ = nullptr;
  void* messageHandlerContext_ = nullptr;
};

}  // namespace reeflow::test::fakes
