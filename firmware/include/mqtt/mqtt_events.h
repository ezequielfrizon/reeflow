#pragma once

#include <stdint.h>

#include "core/events/event_bus.h"
#include "mqtt/mqtt_types.h"

namespace reeflow::mqtt {

enum class MqttEventType {
  kConnected,
  kDisconnected,
  kReconnected,
};

struct MqttEvent {
  MqttEventType type;
  MqttDisconnectReason disconnectReason;
  uint32_t occurredAtMillis;
};

const char* mqttEventName(MqttEventType eventType);
core::events::EventType mqttCoreEventType(MqttEventType eventType);
core::events::Event makeMqttCoreEvent(const MqttEvent& event);
core::events::PublishResult publishMqttEvent(const MqttEvent& event,
                                             core::events::EventBus& bus);

}  // namespace reeflow::mqtt
