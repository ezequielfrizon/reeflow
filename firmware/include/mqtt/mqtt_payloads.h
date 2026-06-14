#pragma once

#include <stddef.h>
#include <stdint.h>

#include "core/events/event_bus.h"
#include "core/state/system_state.h"

namespace reeflow::mqtt {

struct MqttPayloadTimestamp {
  bool synchronized;
  uint32_t epochSeconds;
  uint32_t localMillis;
};

bool serializeTelemetryPayload(const core::state::SystemState& system,
                               const MqttPayloadTimestamp& timestamp,
                               char* output, size_t outputSize);
bool serializeHeartbeatPayload(bool mqttConnected,
                               const MqttPayloadTimestamp& timestamp,
                               char* output, size_t outputSize);
bool serializeEventPayload(const core::events::Event& event,
                           const MqttPayloadTimestamp& timestamp,
                           char* output, size_t outputSize);

}  // namespace reeflow::mqtt
