#include "mqtt/mqtt_topics.h"

#include <stdio.h>

#include "mqtt/mqtt_config.h"

namespace reeflow::mqtt {
namespace {

const char* presentOrDefault(const char* value, const char* defaultValue) {
  return mqttTextPresent(value) ? value : defaultValue;
}

}  // namespace

const char* mqttTopicSuffix(MqttTopicKind kind) {
  switch (kind) {
    case MqttTopicKind::kTelemetry:
      return "telemetry";
    case MqttTopicKind::kEvents:
      return "events";
    case MqttTopicKind::kHeartbeat:
      return "heartbeat";
    case MqttTopicKind::kConnectionStatus:
      return "status/connection";
    case MqttTopicKind::kAlerts:
      return "alerts";
    case MqttTopicKind::kCommandRequest:
      return "commands/in";
    case MqttTopicKind::kCommandResponse:
      return "commands/response";
  }

  return "";
}

bool buildMqttTopic(const MqttTopicContext& context, MqttTopicKind kind,
                    char* output, size_t outputSize) {
  if (output == nullptr || outputSize == 0) {
    return false;
  }

  const char* prefix =
      presentOrDefault(context.prefix, kDefaultMqttTopicPrefix);
  const char* suffix = mqttTopicSuffix(kind);
  const int written =
      mqttTextPresent(context.deviceId)
          ? snprintf(output, outputSize, "%s/%s/%s", prefix, context.deviceId,
                     suffix)
          : snprintf(output, outputSize, "%s/%s", prefix, suffix);

  return written >= 0 && static_cast<size_t>(written) < outputSize;
}

}  // namespace reeflow::mqtt
