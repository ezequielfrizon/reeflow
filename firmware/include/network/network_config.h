#pragma once

#include <stdint.h>

namespace reeflow::network {

constexpr uint32_t kDefaultWifiConnectTimeoutMillis = 15000;
constexpr uint32_t kDefaultReconnectInitialBackoffMillis = 1000;
constexpr uint32_t kDefaultReconnectMaxBackoffMillis = 60000;
constexpr uint32_t kDefaultNetworkHeartbeatIntervalMillis = 30000;
constexpr uint32_t kDefaultNetworkStatusUpdateIntervalMillis = 5000;
constexpr uint32_t kDefaultRssiUpdateIntervalMillis = 10000;
constexpr uint32_t kDefaultNtpSyncTimeoutMillis = 10000;
constexpr const char* kDefaultPrimaryNtpServer = "pool.ntp.org";
constexpr const char* kDefaultSecondaryNtpServer = "time.nist.gov";

struct NetworkConfig {
  uint32_t wifiConnectTimeoutMillis;
  uint32_t reconnectInitialBackoffMillis;
  uint32_t reconnectMaxBackoffMillis;
  uint32_t heartbeatIntervalMillis;
  uint32_t statusUpdateIntervalMillis;
  uint32_t rssiUpdateIntervalMillis;
  uint32_t ntpSyncTimeoutMillis;
  const char* primaryNtpServer;
  const char* secondaryNtpServer;
};

inline NetworkConfig makeDefaultNetworkConfig() {
  return {kDefaultWifiConnectTimeoutMillis,
          kDefaultReconnectInitialBackoffMillis,
          kDefaultReconnectMaxBackoffMillis,
          kDefaultNetworkHeartbeatIntervalMillis,
          kDefaultNetworkStatusUpdateIntervalMillis,
          kDefaultRssiUpdateIntervalMillis,
          kDefaultNtpSyncTimeoutMillis,
          kDefaultPrimaryNtpServer,
          kDefaultSecondaryNtpServer};
}

inline bool validNetworkConfig(const NetworkConfig& config) {
  return config.wifiConnectTimeoutMillis > 0 &&
         config.reconnectInitialBackoffMillis > 0 &&
         config.reconnectMaxBackoffMillis >=
             config.reconnectInitialBackoffMillis &&
         config.heartbeatIntervalMillis > 0 &&
         config.statusUpdateIntervalMillis > 0 &&
         config.rssiUpdateIntervalMillis > 0 &&
         config.ntpSyncTimeoutMillis > 0 &&
         config.primaryNtpServer != nullptr &&
         config.primaryNtpServer[0] != '\0' &&
         config.secondaryNtpServer != nullptr &&
         config.secondaryNtpServer[0] != '\0';
}

}  // namespace reeflow::network
