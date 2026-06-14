#include "network/network_events.h"

namespace reeflow::network {

const char* networkEventName(NetworkEventType eventType) {
  switch (eventType) {
    case NetworkEventType::kWifiConnected:
      return "WIFI_CONNECTED";
    case NetworkEventType::kWifiDisconnected:
      return "WIFI_DISCONNECTED";
    case NetworkEventType::kWifiReconnecting:
      return "WIFI_RECONNECTING";
    case NetworkEventType::kWifiReconnectFailed:
      return "WIFI_RECONNECT_FAILED";
    case NetworkEventType::kNtpSynced:
      return "NTP_SYNCED";
    case NetworkEventType::kNtpSyncFailed:
      return "NTP_SYNC_FAILED";
    case NetworkEventType::kNetworkHeartbeat:
      return "NETWORK_HEARTBEAT";
  }

  return "UNKNOWN_NETWORK_EVENT";
}

core::events::EventType networkCoreEventType(NetworkEventType eventType) {
  switch (eventType) {
    case NetworkEventType::kWifiConnected:
      return core::events::EventType::kWifiConnected;
    case NetworkEventType::kWifiDisconnected:
      return core::events::EventType::kWifiDisconnected;
    case NetworkEventType::kWifiReconnecting:
      return core::events::EventType::kWifiReconnecting;
    case NetworkEventType::kWifiReconnectFailed:
      return core::events::EventType::kWifiReconnectFailed;
    case NetworkEventType::kNtpSynced:
      return core::events::EventType::kNtpSynced;
    case NetworkEventType::kNtpSyncFailed:
      return core::events::EventType::kNtpSyncFailed;
    case NetworkEventType::kNetworkHeartbeat:
      return core::events::EventType::kNetworkHeartbeat;
  }

  return core::events::EventType::kNetworkHeartbeat;
}

core::events::Event makeNetworkCoreEvent(const NetworkEvent& networkEvent) {
  core::events::Event event = {};
  event.type = networkCoreEventType(networkEvent.type);
  event.stateArea = core::events::StateArea::kNetwork;
  event.payload = &networkEvent;
  return event;
}

core::events::PublishResult publishNetworkEvent(
    const NetworkEvent& networkEvent, core::events::EventBus& bus) {
  return bus.publish(makeNetworkCoreEvent(networkEvent));
}

}  // namespace reeflow::network
