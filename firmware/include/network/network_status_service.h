#pragma once

#include "network/network_config.h"
#include "network/network_types.h"
#include "network/wifi_adapter.h"

namespace reeflow::network {

struct NetworkStatusSnapshot {
  WifiStatus wifi;
  InternetProbeResult internet;
  NtpStatus ntp;
  uint32_t lastStatusUpdateMillis;
  uint32_t lastRssiUpdateMillis;
};

class NetworkStatusService {
 public:
  NetworkStatusService(WifiAdapter& wifiAdapter, InternetProbe* internetProbe,
                       NetworkConfig networkConfig =
                           makeDefaultNetworkConfig());

  void tick(uint32_t nowMillis, const NtpStatus& ntpStatus = {});
  NetworkStatusSnapshot snapshot() const;

 private:
  bool statusUpdateDue(uint32_t nowMillis) const;
  bool rssiUpdateDue(uint32_t nowMillis) const;
  void updateConnected(uint32_t nowMillis, const WifiStatus& wifiStatus,
                       bool refreshSignal);
  void updateDisconnected(const WifiStatus& wifiStatus);
  void updateInternet(uint32_t nowMillis);
  void writeNetworkState(const WifiStatus& wifiStatus);

  WifiAdapter& wifiAdapter_;
  InternetProbe* internetProbe_;
  NetworkConfig networkConfig_;
  NetworkStatusSnapshot snapshot_ = {};
  bool hasStatusUpdate_ = false;
  bool hasRssiUpdate_ = false;
};

}  // namespace reeflow::network
