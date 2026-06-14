#include "alerts/alert_mqtt_publisher.h"

#include <stdio.h>
#include <string.h>

#include "alerts/alert_registry.h"
#include "mqtt/mqtt_outbox.h"

namespace reeflow::alerts {
namespace {

const char* alertStateName(AlertState state) {
  switch (state) {
    case AlertState::kRaised:
      return "RAISED";
    case AlertState::kRecovered:
      return "RECOVERED";
  }

  return "UNKNOWN";
}

uint32_t timestampValue(const mqtt::MqttPayloadTimestamp& timestamp) {
  return timestamp.synchronized ? timestamp.epochSeconds
                                : timestamp.localMillis;
}

const char* timestampKind(const mqtt::MqttPayloadTimestamp& timestamp) {
  return timestamp.synchronized ? "epoch" : "local";
}

mqtt::MqttTopicContext topicContextFromPublishContext(
    const mqtt::MqttPublishContext& context) {
  return {context.topic.prefix, context.topic.deviceId};
}

AlertPublishResult mapPublishResult(mqtt::MqttPublishResult result) {
  switch (result) {
    case mqtt::MqttPublishResult::kAccepted:
      return AlertPublishResult::kPublished;
    case mqtt::MqttPublishResult::kNotConnected:
      return AlertPublishResult::kNotConnected;
    case mqtt::MqttPublishResult::kPayloadTooLarge:
    case mqtt::MqttPublishResult::kBrokerUnavailable:
    case mqtt::MqttPublishResult::kClientFailure:
      return AlertPublishResult::kRejected;
  }

  return AlertPublishResult::kRejected;
}

}  // namespace

AlertMqttPublisher::AlertMqttPublisher(mqtt::MqttClient& client)
    : client_(client) {}

bool AlertMqttPublisher::buildMessage(
    const AlertHistoryEntry& entry, const mqtt::MqttPublishContext& context,
    char* topic, size_t topicSize, char* payload,
    size_t payloadSize) const {
  if (topic == nullptr || payload == nullptr || topicSize == 0 ||
      payloadSize == 0 || !isKnownAlertCode(entry.code)) {
    return false;
  }

  if (!mqtt::buildMqttTopic(topicContextFromPublishContext(context),
                            mqtt::MqttTopicKind::kAlerts, topic,
                            topicSize)) {
    return false;
  }

  const int written = snprintf(
      payload, payloadSize,
      "{\"timestamp\":{\"kind\":\"%s\",\"value\":%lu},"
      "\"alert\":{\"code\":\"%s\",\"category\":\"%s\","
      "\"priority\":\"%s\",\"state\":\"%s\",\"recovery\":\"%s\","
      "\"source\":\"%s\",\"occurredAtMillis\":%lu,\"repeatCount\":%lu,"
      "\"reason\":\"%s\"}}",
      timestampKind(context.timestamp),
      static_cast<unsigned long>(timestampValue(context.timestamp)),
      alertCodeName(entry.code), alertCategoryName(entry.category),
      alertPriorityName(entry.priority), alertStateName(entry.state),
      alertRecoveryName(entry.recoveryCode), alertSourceName(entry.source),
      static_cast<unsigned long>(entry.occurredAtMillis),
      static_cast<unsigned long>(entry.repeatCount), entry.reason);

  if (written < 0 || static_cast<size_t>(written) >= payloadSize) {
    return false;
  }

  const size_t payloadLength = strlen(payload);
  return context.maxPayloadBytes == 0 ||
         payloadLength <= context.maxPayloadBytes;
}

AlertPublishResult AlertMqttPublisher::publish(
    const AlertHistoryEntry& entry,
    const mqtt::MqttPublishContext& context) {
  if (!client_.connected()) {
    return AlertPublishResult::kNotConnected;
  }

  char topic[mqtt::kMqttOutboxTopicMaxLength] = {};
  char payload[mqtt::kMqttOutboxPayloadMaxLength] = {};
  if (!buildMessage(entry, context, topic, sizeof(topic), payload,
                    sizeof(payload))) {
    return AlertPublishResult::kRejected;
  }

  const mqtt::MqttMessage message = {
      topic, payload, strlen(payload), mqtt::MqttQos::kAtLeastOnce, false};
  return mapPublishResult(client_.publish(message));
}

}  // namespace reeflow::alerts
