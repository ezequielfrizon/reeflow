#pragma once

#include "core/events/event_bus.h"
#include "network/network_config.h"
#include "network/network_events.h"
#include "network/network_types.h"

namespace reeflow::network {

struct NetworkHeartbeatSnapshot {
  NetworkHeartbeatStatus status;
  uint32_t nextHeartbeatAtMillis;
};

class NetworkHeartbeat {
 public:
  NetworkHeartbeat(core::events::EventBus& eventBus,
                   NetworkConfig networkConfig = makeDefaultNetworkConfig());

  void tick(uint32_t nowMillis);
  NetworkHeartbeatSnapshot snapshot() const;

 private:
  void publish(uint32_t nowMillis);

  core::events::EventBus& eventBus_;
  NetworkConfig networkConfig_;
  NetworkHeartbeatSnapshot snapshot_ = {};
};

}  // namespace reeflow::network
