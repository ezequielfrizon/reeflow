#include "network/wifi_service.h"

#include <string.h>

#include "core/state/system_state.h"

namespace reeflow::network {
namespace {

bool textPresent(const char* value) {
  return value != nullptr && value[0] != '\0';
}

bool connectedStatus(const WifiStatus& status) {
  return status.state == WifiConnectionState::kConnected;
}

WifiConnectResult failureResultForReason(WifiDisconnectReason reason) {
  switch (reason) {
    case WifiDisconnectReason::kCredentialMissing:
      return WifiConnectResult::kCredentialMissing;
    case WifiDisconnectReason::kAuthenticationFailed:
      return WifiConnectResult::kAuthenticationFailed;
    case WifiDisconnectReason::kAccessPointUnavailable:
      return WifiConnectResult::kAccessPointUnavailable;
    case WifiDisconnectReason::kConnectionTimeout:
      return WifiConnectResult::kTimeout;
    case WifiDisconnectReason::kSignalLost:
    case WifiDisconnectReason::kManualDisconnect:
    case WifiDisconnectReason::kUnknown:
      return WifiConnectResult::kAdapterFailure;
  }

  return WifiConnectResult::kAdapterFailure;
}

WifiDisconnectReason reasonForConnectResult(WifiConnectResult result) {
  switch (result) {
    case WifiConnectResult::kCredentialMissing:
      return WifiDisconnectReason::kCredentialMissing;
    case WifiConnectResult::kAuthenticationFailed:
      return WifiDisconnectReason::kAuthenticationFailed;
    case WifiConnectResult::kAccessPointUnavailable:
      return WifiDisconnectReason::kAccessPointUnavailable;
    case WifiConnectResult::kTimeout:
      return WifiDisconnectReason::kConnectionTimeout;
    case WifiConnectResult::kStarted:
    case WifiConnectResult::kAlreadyConnected:
    case WifiConnectResult::kConnected:
    case WifiConnectResult::kAdapterFailure:
      return WifiDisconnectReason::kUnknown;
  }

  return WifiDisconnectReason::kUnknown;
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

WifiService::WifiService(WifiAdapter& adapter,
                         const config::ConfigManager& configManager,
                         core::events::EventBus& eventBus,
                         NetworkConfig networkConfig)
    : adapter_(adapter),
      configManager_(configManager),
      eventBus_(eventBus),
      networkConfig_(networkConfig) {
  resetBackoff();
  status_.state = WifiConnectionState::kDisconnected;
  status_.disconnectReason = WifiDisconnectReason::kUnknown;
}

bool WifiService::begin(uint32_t nowMillis) {
  if (adapterStarted_) {
    return true;
  }

  adapterStarted_ = adapter_.begin();
  if (!adapterStarted_) {
    handleConnectionFailure(nowMillis, WifiConnectResult::kAdapterFailure,
                            WifiDisconnectReason::kUnknown);
  }

  return adapterStarted_;
}

void WifiService::tick(uint32_t nowMillis) {
  if (!adapterStarted_ && !begin(nowMillis)) {
    return;
  }

  if (!wifiConfigured()) {
    lastConnectResult_ = WifiConnectResult::kCredentialMissing;
    connectionAttemptActive_ = false;
    nextReconnectAtMillis_ = 0;
    resetBackoff();
    status_.state = WifiConnectionState::kDisabled;
    status_.disconnectReason = WifiDisconnectReason::kCredentialMissing;
    if (connected_) {
      adapter_.disconnect();
      publish(NetworkEventType::kWifiDisconnected, nowMillis,
              WifiDisconnectReason::kCredentialMissing);
    }
    connected_ = false;
    updateNetworkDisconnected();
    return;
  }

  status_ = adapter_.status();

  if (connectedStatus(status_)) {
    handleConnected(nowMillis, status_);
    return;
  }

  if (connected_) {
    handleDisconnected(nowMillis, adapter_.lastDisconnectReason());
    return;
  }

  if (connectionAttemptActive_) {
    if (nowMillis - attemptStartedAtMillis_ >=
        networkConfig_.wifiConnectTimeoutMillis) {
      status_.state = WifiConnectionState::kFailed;
      status_.disconnectReason = WifiDisconnectReason::kConnectionTimeout;
      handleConnectionFailure(nowMillis, WifiConnectResult::kTimeout,
                              WifiDisconnectReason::kConnectionTimeout);
    }
    return;
  }

  if (nextReconnectAtMillis_ == 0 || nowMillis >= nextReconnectAtMillis_) {
    startConnection(nowMillis);
  }
}

WifiServiceSnapshot WifiService::snapshot() const {
  return {status_, lastConnectResult_, nextReconnectAtMillis_,
          currentBackoffMillis_};
}

bool WifiService::wifiConfigured() const {
  const config::WifiConfig& wifi = configManager_.wifi();
  return wifi.enabled && textPresent(wifi.ssid);
}

WifiCredentials WifiService::credentials() const {
  const config::WifiConfig& wifi = configManager_.wifi();
  return {wifi.ssid, wifi.password};
}

void WifiService::startConnection(uint32_t nowMillis) {
  publish(NetworkEventType::kWifiReconnecting, nowMillis);
  attemptStartedAtMillis_ = nowMillis;
  connectionAttemptActive_ = true;
  status_.state = WifiConnectionState::kConnecting;
  status_.disconnectReason = WifiDisconnectReason::kUnknown;

  lastConnectResult_ =
      adapter_.connect(credentials(), networkConfig_.wifiConnectTimeoutMillis);

  if (lastConnectResult_ == WifiConnectResult::kConnected ||
      lastConnectResult_ == WifiConnectResult::kAlreadyConnected) {
    handleConnected(nowMillis, adapter_.status());
    return;
  }

  if (lastConnectResult_ == WifiConnectResult::kStarted) {
    return;
  }

  handleConnectionFailure(nowMillis, lastConnectResult_,
                          reasonForConnectResult(lastConnectResult_));
}

void WifiService::handleConnected(uint32_t nowMillis,
                                  const WifiStatus& status) {
  status_ = status;
  status_.state = WifiConnectionState::kConnected;
  lastConnectResult_ = WifiConnectResult::kConnected;
  connectionAttemptActive_ = false;
  nextReconnectAtMillis_ = 0;
  resetBackoff();
  updateNetworkConnected(status_);

  if (!connected_) {
    publish(NetworkEventType::kWifiConnected, nowMillis);
  }

  connected_ = true;
}

void WifiService::handleDisconnected(uint32_t nowMillis,
                                     WifiDisconnectReason reason) {
  connected_ = false;
  connectionAttemptActive_ = false;
  status_.state = WifiConnectionState::kDisconnected;
  status_.disconnectReason = reason;
  status_.hasIpAddress = false;
  status_.ipAddress[0] = '\0';
  status_.hasRssi = false;
  status_.rssi = 0;
  lastConnectResult_ = failureResultForReason(reason);
  updateNetworkDisconnected();
  publish(NetworkEventType::kWifiDisconnected, nowMillis, reason);
  scheduleReconnect(nowMillis);
}

void WifiService::handleConnectionFailure(uint32_t nowMillis,
                                          WifiConnectResult result,
                                          WifiDisconnectReason reason) {
  connected_ = false;
  connectionAttemptActive_ = false;
  lastConnectResult_ = result;
  status_.state = WifiConnectionState::kFailed;
  status_.disconnectReason = reason;
  status_.hasIpAddress = false;
  status_.ipAddress[0] = '\0';
  status_.hasRssi = false;
  status_.rssi = 0;
  updateNetworkDisconnected();
  publish(NetworkEventType::kWifiReconnectFailed, nowMillis, reason);

  if (result != WifiConnectResult::kCredentialMissing) {
    scheduleReconnect(nowMillis);
  }
}

void WifiService::scheduleReconnect(uint32_t nowMillis) {
  const uint32_t delayMillis = nextBackoffMillis_;
  currentBackoffMillis_ = delayMillis;
  nextReconnectAtMillis_ = nowMillis + delayMillis;

  if (nextBackoffMillis_ < networkConfig_.reconnectMaxBackoffMillis) {
    const uint32_t doubled = nextBackoffMillis_ * 2;
    nextBackoffMillis_ =
        doubled > networkConfig_.reconnectMaxBackoffMillis
            ? networkConfig_.reconnectMaxBackoffMillis
            : doubled;
  }
}

void WifiService::resetBackoff() {
  currentBackoffMillis_ = 0;
  nextBackoffMillis_ = networkConfig_.reconnectInitialBackoffMillis;
}

void WifiService::publish(NetworkEventType type, uint32_t nowMillis,
                          WifiDisconnectReason reason) {
  NetworkEvent event = {};
  event.type = type;
  event.wifiReason = reason;
  event.occurredAtMillis = nowMillis;
  publishNetworkEvent(event, eventBus_);
}

void WifiService::updateNetworkConnected(const WifiStatus& status) {
  core::state::NetworkState network =
      core::state::currentSystemState().network;
  network.wifiConnected = true;
  network.internetAvailable = false;
  network.ipAddress[0] = '\0';
  network.rssi = 0;
  if (status.hasIpAddress) {
    copyIpAddress(network.ipAddress, sizeof(network.ipAddress),
                  status.ipAddress);
  }
  if (status.hasRssi) {
    network.rssi = status.rssi;
  }
  core::state::updateNetworkState(network);
}

void WifiService::updateNetworkDisconnected() {
  core::state::NetworkState network =
      core::state::currentSystemState().network;
  network.wifiConnected = false;
  network.internetAvailable = false;
  network.ipAddress[0] = '\0';
  network.rssi = 0;
  core::state::updateNetworkState(network);
}

}  // namespace reeflow::network
