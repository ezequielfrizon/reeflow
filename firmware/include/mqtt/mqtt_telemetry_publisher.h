#pragma once

#include "core/events/event_bus.h"
#include "core/state/system_state.h"
#include "mqtt/mqtt_client.h"
#include "mqtt/mqtt_payloads.h"
#include "mqtt/mqtt_topics.h"

namespace reeflow::mqtt {

struct MqttPublishContext {
  MqttTopicContext topic;
  MqttPayloadTimestamp timestamp;
  uint16_t maxPayloadBytes;
};

class MqttTelemetryPublisher {
 public:
  explicit MqttTelemetryPublisher(MqttClient& client) : client_(client) {}

  MqttPublishResult publishTelemetry(const core::state::SystemState& system,
                                      const MqttPublishContext& context);
  MqttPublishResult publishHeartbeat(const core::state::SystemState& system,
                                      const MqttPublishContext& context);
  MqttPublishResult publishEvent(const core::events::Event& event,
                                 const MqttPublishContext& context);

 private:
  MqttClient& client_;
};

}  // namespace reeflow::mqtt
