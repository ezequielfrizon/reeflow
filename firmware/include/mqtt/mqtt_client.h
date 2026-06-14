#pragma once

#include "mqtt/mqtt_config.h"
#include "mqtt/mqtt_types.h"

namespace reeflow::mqtt {

class MqttClient {
 public:
  virtual ~MqttClient() = default;

  virtual void configure(const MqttConfig& config) = 0;
  virtual MqttConnectResult connect() = 0;
  virtual void disconnect(MqttDisconnectReason reason) = 0;
  virtual bool connected() const = 0;
  virtual MqttPublishResult publish(const MqttMessage& message) = 0;
  virtual MqttSubscribeResult subscribe(const char* topic, MqttQos qos) = 0;
  virtual void setMessageHandler(MqttMessageHandler handler,
                                 void* context) = 0;
  virtual void loop(uint32_t nowMillis) = 0;
};

}  // namespace reeflow::mqtt
