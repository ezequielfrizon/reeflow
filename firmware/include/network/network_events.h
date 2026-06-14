#pragma once

#include <stdint.h>

#include "core/events/event_bus.h"
#include "network/network_types.h"

namespace reeflow::network {

enum class NetworkEventType {
  kWifiConnected,
  kWifiDisconnected,
  kWifiReconnecting,
  kWifiReconnectFailed,
  kNtpSynced,
  kNtpSyncFailed,
  kNetworkHeartbeat,
};

struct NetworkEvent {
  NetworkEventType type;
  WifiDisconnectReason wifiReason;
  InternetStatus internetStatus;
  NtpSyncStatus ntpStatus;
  uint32_t occurredAtMillis;
};

const char* networkEventName(NetworkEventType eventType);
core::events::EventType networkCoreEventType(NetworkEventType eventType);
core::events::Event makeNetworkCoreEvent(const NetworkEvent& event);
core::events::PublishResult publishNetworkEvent(const NetworkEvent& event,
                                                core::events::EventBus& bus);

}  // namespace reeflow::network
