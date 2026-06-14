#pragma once

#include <stddef.h>

namespace reeflow::mqtt {

enum class MqttTopicKind {
  kTelemetry,
  kEvents,
  kHeartbeat,
  kConnectionStatus,
  kAlerts,
  kCommandRequest,
  kCommandResponse,
};

struct MqttTopicContext {
  const char* prefix;
  const char* deviceId;
};

const char* mqttTopicSuffix(MqttTopicKind kind);
bool buildMqttTopic(const MqttTopicContext& context, MqttTopicKind kind,
                    char* output, size_t outputSize);

}  // namespace reeflow::mqtt
