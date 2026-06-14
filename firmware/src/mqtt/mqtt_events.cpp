#include "mqtt/mqtt_events.h"

namespace reeflow::mqtt {

const char* mqttEventName(MqttEventType eventType) {
  switch (eventType) {
    case MqttEventType::kConnected:
      return "MQTT_CONNECTED";
    case MqttEventType::kDisconnected:
      return "MQTT_DISCONNECTED";
    case MqttEventType::kReconnected:
      return "MQTT_RECONNECTED";
  }

  return "UNKNOWN_MQTT_EVENT";
}

core::events::EventType mqttCoreEventType(MqttEventType eventType) {
  switch (eventType) {
    case MqttEventType::kConnected:
      return core::events::EventType::kMqttConnected;
    case MqttEventType::kDisconnected:
      return core::events::EventType::kMqttDisconnected;
    case MqttEventType::kReconnected:
      return core::events::EventType::kMqttReconnected;
  }

  return core::events::EventType::kMqttDisconnected;
}

core::events::Event makeMqttCoreEvent(const MqttEvent& mqttEvent) {
  core::events::Event event = {};
  event.type = mqttCoreEventType(mqttEvent.type);
  event.stateArea = core::events::StateArea::kNetwork;
  event.payload = &mqttEvent;
  return event;
}

core::events::PublishResult publishMqttEvent(const MqttEvent& mqttEvent,
                                             core::events::EventBus& bus) {
  return bus.publish(makeMqttCoreEvent(mqttEvent));
}

}  // namespace reeflow::mqtt
