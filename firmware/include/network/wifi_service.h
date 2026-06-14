#pragma once

#include "config/config_manager.h"
#include "core/events/event_bus.h"
#include "network/network_config.h"
#include "network/network_events.h"
#include "network/network_types.h"
#include "network/wifi_adapter.h"

namespace reeflow::network {

struct WifiServiceSnapshot {
  WifiStatus status;
  WifiConnectResult lastConnectResult;
  uint32_t nextReconnectAtMillis;
  uint32_t currentBackoffMillis;
};

struct WifiReconnectPolicy {
  uint32_t initialBackoffMillis;
  uint32_t maxBackoffMillis;
};

inline WifiReconnectPolicy makeWifiReconnectPolicy(
    const NetworkConfig& config) {
  return {config.reconnectInitialBackoffMillis,
          config.reconnectMaxBackoffMillis};
}

class WifiService {
 public:
  WifiService(WifiAdapter& adapter, const config::ConfigManager& configManager,
              core::events::EventBus& eventBus,
              NetworkConfig networkConfig = makeDefaultNetworkConfig());

  bool begin(uint32_t nowMillis);
  void tick(uint32_t nowMillis);
  WifiServiceSnapshot snapshot() const;

 private:
  bool wifiConfigured() const;
  WifiCredentials credentials() const;
  void startConnection(uint32_t nowMillis);
  void handleConnected(uint32_t nowMillis, const WifiStatus& status);
  void handleDisconnected(uint32_t nowMillis, WifiDisconnectReason reason);
  void handleConnectionFailure(uint32_t nowMillis, WifiConnectResult result,
                               WifiDisconnectReason reason);
  void scheduleReconnect(uint32_t nowMillis);
  void resetBackoff();
  void publish(NetworkEventType type, uint32_t nowMillis,
               WifiDisconnectReason reason = WifiDisconnectReason::kUnknown);
  void updateNetworkConnected(const WifiStatus& status);
  void updateNetworkDisconnected();

  WifiAdapter& adapter_;
  const config::ConfigManager& configManager_;
  core::events::EventBus& eventBus_;
  NetworkConfig networkConfig_;
  WifiStatus status_ = {};
  WifiConnectResult lastConnectResult_ = WifiConnectResult::kStarted;
  uint32_t attemptStartedAtMillis_ = 0;
  uint32_t nextReconnectAtMillis_ = 0;
  uint32_t currentBackoffMillis_ = 0;
  uint32_t nextBackoffMillis_ = 0;
  bool adapterStarted_ = false;
  bool connectionAttemptActive_ = false;
  bool connected_ = false;
};

}  // namespace reeflow::network
