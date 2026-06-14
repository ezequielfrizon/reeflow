#include "mqtt/mqtt_telemetry_publisher.h"

#include <string.h>

namespace reeflow::mqtt {
namespace {

constexpr size_t kMqttTopicBufferLength = 128;
constexpr size_t kMqttPayloadBufferLength = 768;

MqttPublishResult publishSerialized(MqttClient& client,
                                     MqttTopicKind topicKind,
                                     const MqttPublishContext& context,
                                     const char* payload, MqttQos qos) {
  if (!client.connected()) {
    return MqttPublishResult::kNotConnected;
  }
  if (payload == nullptr) {
    return MqttPublishResult::kClientFailure;
  }

  const size_t payloadLength = strlen(payload);
  if (context.maxPayloadBytes > 0 &&
      payloadLength > context.maxPayloadBytes) {
    return MqttPublishResult::kPayloadTooLarge;
  }

  char topic[kMqttTopicBufferLength] = {};
  if (!buildMqttTopic(context.topic, topicKind, topic, sizeof(topic))) {
    return MqttPublishResult::kClientFailure;
  }

  const MqttMessage message = {topic, payload, payloadLength, qos, false};
  return client.publish(message);
}

}  // namespace

MqttPublishResult MqttTelemetryPublisher::publishTelemetry(
    const core::state::SystemState& system,
    const MqttPublishContext& context) {
  char payload[kMqttPayloadBufferLength] = {};
  if (!serializeTelemetryPayload(system, context.timestamp, payload,
                                 sizeof(payload))) {
    return MqttPublishResult::kPayloadTooLarge;
  }

  return publishSerialized(client_, MqttTopicKind::kTelemetry, context, payload,
                           MqttQos::kAtMostOnce);
}

MqttPublishResult MqttTelemetryPublisher::publishHeartbeat(
    const core::state::SystemState& system,
    const MqttPublishContext& context) {
  char payload[kMqttPayloadBufferLength] = {};
  if (!serializeHeartbeatPayload(system.network.mqttConnected,
                                 context.timestamp, payload,
                                 sizeof(payload))) {
    return MqttPublishResult::kPayloadTooLarge;
  }

  return publishSerialized(client_, MqttTopicKind::kHeartbeat, context, payload,
                           MqttQos::kAtMostOnce);
}

MqttPublishResult MqttTelemetryPublisher::publishEvent(
    const core::events::Event& event, const MqttPublishContext& context) {
  char payload[kMqttPayloadBufferLength] = {};
  if (!serializeEventPayload(event, context.timestamp, payload,
                             sizeof(payload))) {
    return MqttPublishResult::kPayloadTooLarge;
  }

  return publishSerialized(client_, MqttTopicKind::kEvents, context, payload,
                           MqttQos::kAtLeastOnce);
}

}  // namespace reeflow::mqtt
