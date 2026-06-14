#pragma once

#include "alerts/alert_events.h"
#include "mqtt/mqtt_client.h"
#include "mqtt/mqtt_telemetry_publisher.h"

namespace reeflow::alerts {

class AlertMqttSink {
 public:
  virtual ~AlertMqttSink() = default;

  virtual bool available() const = 0;
  virtual AlertPublishResult publishAlert(const AlertHistoryEntry& entry) = 0;
};

class AlertMqttPublisher final {
 public:
  explicit AlertMqttPublisher(mqtt::MqttClient& client);

  bool buildMessage(const AlertHistoryEntry& entry,
                    const mqtt::MqttPublishContext& context, char* topic,
                    size_t topicSize, char* payload,
                    size_t payloadSize) const;
  AlertPublishResult publish(const AlertHistoryEntry& entry,
                             const mqtt::MqttPublishContext& context);

 private:
  mqtt::MqttClient& client_;
};

}  // namespace reeflow::alerts
