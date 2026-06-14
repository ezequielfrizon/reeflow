#include "network/network_status_service.h"

#include <string.h>

#include "core/state/system_state.h"

namespace reeflow::network {
namespace {

bool wifiConnected(const WifiStatus& status) {
  return status.state == WifiConnectionState::kConnected;
}

bool internetAvailable(InternetStatus status) {
  return status == InternetStatus::kAvailable;
}

void copyIpAddress(char* destination, size_t capacity, const char* source) {
  if (capacity == 0) {
    return;
  }

  if (source == nullptr) {
    destination[0] = '\0';
    return;
  }

  strncpy(destination, source, capacity - 1);
  destination[capacity - 1] = '\0';
}

}  // namespace

NetworkStatusService::NetworkStatusService(WifiAdapter& wifiAdapter,
                                           InternetProbe* internetProbe,
                                           NetworkConfig networkConfig)
    : wifiAdapter_(wifiAdapter),
      internetProbe_(internetProbe),
      networkConfig_(networkConfig) {
  snapshot_.internet.status = InternetStatus::kUnknown;
  snapshot_.ntp.status = NtpSyncStatus::kNotStarted;
}

void NetworkStatusService::tick(uint32_t nowMillis,
                                const NtpStatus& ntpStatus) {
  if (!statusUpdateDue(nowMillis)) {
    return;
  }

  const bool refreshSignal = rssiUpdateDue(nowMillis);
  WifiStatus wifiStatus = wifiAdapter_.status();
  snapshot_.wifi = wifiStatus;
  snapshot_.ntp = ntpStatus;
  snapshot_.lastStatusUpdateMillis = nowMillis;
  hasStatusUpdate_ = true;

  if (refreshSignal) {
    snapshot_.lastRssiUpdateMillis = nowMillis;
    hasRssiUpdate_ = true;
  }

  if (wifiConnected(wifiStatus)) {
    updateConnected(nowMillis, wifiStatus, refreshSignal);
  } else {
    updateDisconnected(wifiStatus);
  }
}

NetworkStatusSnapshot NetworkStatusService::snapshot() const {
  return snapshot_;
}

bool NetworkStatusService::statusUpdateDue(uint32_t nowMillis) const {
  return !hasStatusUpdate_ ||
         nowMillis - snapshot_.lastStatusUpdateMillis >=
             networkConfig_.statusUpdateIntervalMillis;
}

bool NetworkStatusService::rssiUpdateDue(uint32_t nowMillis) const {
  return !hasRssiUpdate_ ||
         nowMillis - snapshot_.lastRssiUpdateMillis >=
             networkConfig_.rssiUpdateIntervalMillis;
}

void NetworkStatusService::updateConnected(uint32_t nowMillis,
                                           const WifiStatus& wifiStatus,
                                           bool refreshSignal) {
  updateInternet(nowMillis);

  WifiStatus stateWifiStatus = wifiStatus;
  if (!refreshSignal) {
    const core::state::NetworkState& current =
        core::state::currentSystemState().network;
    stateWifiStatus.hasRssi = true;
    stateWifiStatus.rssi = current.rssi;
    stateWifiStatus.hasIpAddress = current.ipAddress[0] != '\0';
    copyIpAddress(stateWifiStatus.ipAddress, sizeof(stateWifiStatus.ipAddress),
                  current.ipAddress);
  }

  writeNetworkState(stateWifiStatus);
}

void NetworkStatusService::updateDisconnected(const WifiStatus& wifiStatus) {
  snapshot_.internet.status = InternetStatus::kUnavailable;
  writeNetworkState(wifiStatus);
}

void NetworkStatusService::updateInternet(uint32_t nowMillis) {
  if (internetProbe_ == nullptr) {
    snapshot_.internet = {InternetStatus::kUnknown, nowMillis};
    return;
  }

  snapshot_.internet = internetProbe_->probe(nowMillis);
}

void NetworkStatusService::writeNetworkState(const WifiStatus& wifiStatus) {
  core::state::NetworkState network = core::state::currentSystemState().network;

  if (wifiConnected(wifiStatus)) {
    network.wifiConnected = true;
    network.internetAvailable = internetAvailable(snapshot_.internet.status);
    if (wifiStatus.hasIpAddress) {
      copyIpAddress(network.ipAddress, sizeof(network.ipAddress),
                    wifiStatus.ipAddress);
    }
    if (wifiStatus.hasRssi) {
      network.rssi = wifiStatus.rssi;
    }
  } else {
    network.wifiConnected = false;
    network.internetAvailable = false;
    network.ipAddress[0] = '\0';
    network.rssi = 0;
  }

  core::state::updateNetworkState(network);
}

}  // namespace reeflow::network
