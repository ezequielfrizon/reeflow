#include "network/network_heartbeat.h"

#include "core/state/system_state.h"

namespace reeflow::network {

NetworkHeartbeat::NetworkHeartbeat(core::events::EventBus& eventBus,
                                   NetworkConfig networkConfig)
    : eventBus_(eventBus), networkConfig_(networkConfig) {}

void NetworkHeartbeat::tick(uint32_t nowMillis) {
  if (snapshot_.nextHeartbeatAtMillis != 0 &&
      nowMillis < snapshot_.nextHeartbeatAtMillis) {
    return;
  }

  snapshot_.status.lastHeartbeatMillis = nowMillis;
  snapshot_.nextHeartbeatAtMillis =
      nowMillis + networkConfig_.heartbeatIntervalMillis;

  core::state::NetworkState network = core::state::currentSystemState().network;
  network.lastHeartbeat = nowMillis;
  core::state::updateNetworkState(network);
  publish(nowMillis);
}

NetworkHeartbeatSnapshot NetworkHeartbeat::snapshot() const {
  return snapshot_;
}

void NetworkHeartbeat::publish(uint32_t nowMillis) {
  NetworkEvent event = {};
  event.type = NetworkEventType::kNetworkHeartbeat;
  event.occurredAtMillis = nowMillis;
  publishNetworkEvent(event, eventBus_);
}

}  // namespace reeflow::network
