#pragma once

#include <stddef.h>
#include <stdint.h>

#include "modules/lighting/lighting_types.h"
#include "modules/modes/mode_types.h"
#include "modules/relays/relay_types.h"
#include "mqtt/mqtt_types.h"

namespace reeflow::mqtt {

constexpr size_t kMqttCommandCorrelationMaxLength = 32;
constexpr size_t kMqttCommandResponseMaxLength = 192;

struct MqttCommandRequest {
  const char* topic;
  const char* expectedTopic;
  const char* payload;
  size_t payloadLength;
  bool mqttConnected;
};

struct MqttCommandConfigRequest {
  bool updateMqttHeartbeat;
  uint32_t mqttHeartbeatIntervalMillis;
  bool updateMqttMaxPayload;
  uint16_t mqttMaxPayloadBytes;
};

using MqttModeCommandCallback =
    MqttCommandResult (*)(modules::modes::OperationalMode mode,
                          void* context);
using MqttRelayCommandCallback =
    MqttCommandResult (*)(modules::relays::RelayId relay,
                          modules::relays::RelayDesiredState desiredState,
                          void* context);
using MqttLightingCommandCallback =
    MqttCommandResult (*)(modules::lighting::LightingChannel channel,
                          uint16_t duty, void* context);
using MqttConfigCommandCallback =
    MqttCommandResult (*)(const MqttCommandConfigRequest& request,
                          void* context);

struct MqttCommandCallbacks {
  MqttModeCommandCallback mode;
  MqttRelayCommandCallback relay;
  MqttLightingCommandCallback lighting;
  MqttConfigCommandCallback config;
  void* context;
};

struct MqttCommandResponse {
  MqttCommandResult result;
  char correlation[kMqttCommandCorrelationMaxLength + 1];
  char payload[kMqttCommandResponseMaxLength];
};

class MqttCommandHandler {
 public:
  void configure(const MqttCommandCallbacks& callbacks);
  MqttCommandResponse handle(const MqttCommandRequest& request) const;

 private:
  MqttCommandCallbacks callbacks_ = {};
};

}  // namespace reeflow::mqtt
