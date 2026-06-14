#pragma once

#include <string.h>

#include "network/wifi_adapter.h"

namespace reeflow::test::fakes {

class FakeWifiAdapter final : public reeflow::network::WifiAdapter {
 public:
  bool begin() override {
    beginCallCount_ += 1;
    return beginResult_;
  }

  reeflow::network::WifiConnectResult connect(
      const reeflow::network::WifiCredentials& credentials,
      uint32_t timeoutMillis) override {
    connectCallCount_ += 1;
    lastCredentials_ = credentials;
    lastTimeoutMillis_ = timeoutMillis;

    if (credentials.ssid == nullptr || credentials.ssid[0] == '\0') {
      status_.state = reeflow::network::WifiConnectionState::kDisabled;
      status_.disconnectReason =
          reeflow::network::WifiDisconnectReason::kCredentialMissing;
      return reeflow::network::WifiConnectResult::kCredentialMissing;
    }

    applyConnectResult(connectResult_);
    return connectResult_;
  }

  void disconnect() override {
    disconnectCallCount_ += 1;
    status_.state = reeflow::network::WifiConnectionState::kDisconnected;
    status_.disconnectReason =
        reeflow::network::WifiDisconnectReason::kManualDisconnect;
  }

  reeflow::network::WifiStatus status() const override { return status_; }
  const char* ipAddress() const override { return status_.ipAddress; }
  int32_t rssi() const override { return status_.rssi; }

  reeflow::network::WifiDisconnectReason lastDisconnectReason()
      const override {
    return status_.disconnectReason;
  }

  void simulateSuccessfulConnection(const char* ipAddress, int32_t rssi) {
    connectResult_ = reeflow::network::WifiConnectResult::kConnected;
    pendingIpAddress_ = ipAddress;
    pendingRssi_ = rssi;
    status_.state = reeflow::network::WifiConnectionState::kDisconnected;
    status_.disconnectReason = reeflow::network::WifiDisconnectReason::kUnknown;
    status_.hasIpAddress = false;
    status_.hasRssi = false;
  }

  void simulateConnectionStarted() {
    connectResult_ = reeflow::network::WifiConnectResult::kStarted;
    status_.state = reeflow::network::WifiConnectionState::kConnecting;
    status_.disconnectReason = reeflow::network::WifiDisconnectReason::kUnknown;
    status_.hasIpAddress = false;
    status_.hasRssi = false;
  }

  void simulateAuthenticationFailure() {
    connectResult_ =
        reeflow::network::WifiConnectResult::kAuthenticationFailed;
    status_.state = reeflow::network::WifiConnectionState::kFailed;
    status_.disconnectReason =
        reeflow::network::WifiDisconnectReason::kAuthenticationFailed;
    status_.hasIpAddress = false;
    status_.hasRssi = false;
  }

  void simulateAccessPointUnavailable() {
    connectResult_ =
        reeflow::network::WifiConnectResult::kAccessPointUnavailable;
    status_.state = reeflow::network::WifiConnectionState::kFailed;
    status_.disconnectReason =
        reeflow::network::WifiDisconnectReason::kAccessPointUnavailable;
    status_.hasIpAddress = false;
    status_.hasRssi = false;
  }

  void simulateTimeout() {
    connectResult_ = reeflow::network::WifiConnectResult::kTimeout;
    status_.state = reeflow::network::WifiConnectionState::kFailed;
    status_.disconnectReason =
        reeflow::network::WifiDisconnectReason::kConnectionTimeout;
    status_.hasIpAddress = false;
    status_.hasRssi = false;
  }

  void simulateConnectionDrop() {
    status_.state = reeflow::network::WifiConnectionState::kDisconnected;
    status_.disconnectReason =
        reeflow::network::WifiDisconnectReason::kSignalLost;
    status_.hasIpAddress = false;
    status_.hasRssi = false;
  }

  void setConnectedStatus(const char* ipAddress, int32_t rssi) {
    status_.state = reeflow::network::WifiConnectionState::kConnected;
    status_.disconnectReason = reeflow::network::WifiDisconnectReason::kUnknown;
    status_.hasIpAddress = true;
    copyIp(ipAddress);
    status_.hasRssi = true;
    status_.rssi = rssi;
  }

  void setBeginResult(bool beginResult) { beginResult_ = beginResult; }

  uint16_t beginCallCount() const { return beginCallCount_; }
  uint16_t connectCallCount() const { return connectCallCount_; }
  uint16_t disconnectCallCount() const { return disconnectCallCount_; }
  uint32_t lastTimeoutMillis() const { return lastTimeoutMillis_; }
  reeflow::network::WifiCredentials lastCredentials() const {
    return lastCredentials_;
  }

 private:
  void applyConnectResult(reeflow::network::WifiConnectResult result) {
    if (result == reeflow::network::WifiConnectResult::kConnected) {
      status_.state = reeflow::network::WifiConnectionState::kConnected;
      status_.disconnectReason =
          reeflow::network::WifiDisconnectReason::kUnknown;
      status_.hasIpAddress = true;
      copyIp(pendingIpAddress_);
      status_.hasRssi = true;
      status_.rssi = pendingRssi_;
    } else if (result == reeflow::network::WifiConnectResult::kStarted) {
      status_.state = reeflow::network::WifiConnectionState::kConnecting;
      status_.disconnectReason =
          reeflow::network::WifiDisconnectReason::kUnknown;
    }
  }

  void copyIp(const char* ipAddress) {
    if (ipAddress == nullptr) {
      status_.ipAddress[0] = '\0';
      return;
    }

    strncpy(status_.ipAddress, ipAddress, sizeof(status_.ipAddress) - 1);
    status_.ipAddress[sizeof(status_.ipAddress) - 1] = '\0';
  }

  bool beginResult_ = true;
  reeflow::network::WifiConnectResult connectResult_ =
      reeflow::network::WifiConnectResult::kConnected;
  reeflow::network::WifiStatus status_ = {};
  reeflow::network::WifiCredentials lastCredentials_ = {};
  const char* pendingIpAddress_ = "192.168.1.20";
  int32_t pendingRssi_ = -55;
  uint32_t lastTimeoutMillis_ = 0;
  uint16_t beginCallCount_ = 0;
  uint16_t connectCallCount_ = 0;
  uint16_t disconnectCallCount_ = 0;
};

}  // namespace reeflow::test::fakes
