#pragma once

#include <WiFiClient.h>

#include "mqtt/mqtt_client.h"

namespace reeflow::mqtt {

class ArduinoMqttClient final : public MqttClient {
 public:
  void configure(const MqttConfig& config) override;
  MqttConnectResult connect() override;
  void disconnect(MqttDisconnectReason reason) override;
  bool connected() const override;
  MqttPublishResult publish(const MqttMessage& message) override;
  MqttSubscribeResult subscribe(const char* topic, MqttQos qos) override;
  void setMessageHandler(MqttMessageHandler handler, void* context) override;
  void loop(uint32_t nowMillis) override;

 private:
  bool writeString(const char* value);
  bool writeRemainingLength(uint32_t length);
  bool readPacketHeader(uint8_t expectedType, uint32_t* remainingLength);
  bool readExact(uint8_t* buffer, size_t length);
  MqttPublishResult publishPacket(const MqttMessage& message);
  void handleIncomingPublish(uint8_t packetType, uint32_t remainingLength);

  mutable WiFiClient client_;
  MqttConfig config_ = makeDefaultMqttConfig();
  MqttMessageHandler messageHandler_ = nullptr;
  void* messageHandlerContext_ = nullptr;
  uint16_t nextPacketId_ = 1;
};

}  // namespace reeflow::mqtt
