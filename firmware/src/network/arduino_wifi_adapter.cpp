#include "arduino_wifi_adapter.h"

#include <WiFi.h>
#include <string.h>

namespace reeflow::network {

bool ArduinoWifiAdapter::begin() {
  WiFi.persistent(false);
  return WiFi.mode(WIFI_STA);
}

WifiConnectResult ArduinoWifiAdapter::connect(
    const WifiCredentials& credentials, uint32_t) {
  if (credentials.ssid == nullptr || credentials.ssid[0] == '\0') {
    status_.state = WifiConnectionState::kDisabled;
    status_.disconnectReason = WifiDisconnectReason::kCredentialMissing;
    return WifiConnectResult::kCredentialMissing;
  }

  WiFi.mode(WIFI_STA);
  const int result = WiFi.begin(credentials.ssid, credentials.password);
  refreshStatus();

  if (result == WL_CONNECTED || status_.state == WifiConnectionState::kConnected) {
    return WifiConnectResult::kConnected;
  }

  if (result == WL_NO_SSID_AVAIL) {
    status_.state = WifiConnectionState::kFailed;
    status_.disconnectReason = WifiDisconnectReason::kAccessPointUnavailable;
    return WifiConnectResult::kAccessPointUnavailable;
  }

  if (result == WL_CONNECT_FAILED) {
    status_.state = WifiConnectionState::kFailed;
    status_.disconnectReason = WifiDisconnectReason::kAuthenticationFailed;
    return WifiConnectResult::kAuthenticationFailed;
  }

  status_.state = WifiConnectionState::kConnecting;
  status_.disconnectReason = WifiDisconnectReason::kUnknown;
  return WifiConnectResult::kStarted;
}

void ArduinoWifiAdapter::disconnect() {
  WiFi.disconnect(false, false);
  status_.state = WifiConnectionState::kDisconnected;
  status_.disconnectReason = WifiDisconnectReason::kManualDisconnect;
  status_.hasIpAddress = false;
  status_.ipAddress[0] = '\0';
  status_.hasRssi = false;
  status_.rssi = 0;
}

WifiStatus ArduinoWifiAdapter::status() const {
  refreshStatus();
  return status_;
}

const char* ArduinoWifiAdapter::ipAddress() const {
  refreshStatus();
  return status_.ipAddress;
}

int32_t ArduinoWifiAdapter::rssi() const {
  refreshStatus();
  return status_.rssi;
}

WifiDisconnectReason ArduinoWifiAdapter::lastDisconnectReason() const {
  refreshStatus();
  return status_.disconnectReason;
}

void ArduinoWifiAdapter::refreshStatus() const {
  const int wifiStatus = WiFi.status();

  if (wifiStatus == WL_CONNECTED) {
    status_.state = WifiConnectionState::kConnected;
    status_.disconnectReason = WifiDisconnectReason::kUnknown;
    status_.hasIpAddress = true;
    const String ip = WiFi.localIP().toString();
    strncpy(status_.ipAddress, ip.c_str(), sizeof(status_.ipAddress) - 1);
    status_.ipAddress[sizeof(status_.ipAddress) - 1] = '\0';
    status_.hasRssi = true;
    status_.rssi = WiFi.RSSI();
    return;
  }

  status_.hasIpAddress = false;
  status_.ipAddress[0] = '\0';
  status_.hasRssi = false;
  status_.rssi = 0;
  status_.disconnectReason = mapDisconnectReason(wifiStatus);

  if (wifiStatus == WL_IDLE_STATUS) {
    status_.state = WifiConnectionState::kConnecting;
  } else if (wifiStatus == WL_NO_SSID_AVAIL ||
             wifiStatus == WL_CONNECT_FAILED) {
    status_.state = WifiConnectionState::kFailed;
  } else {
    status_.state = WifiConnectionState::kDisconnected;
  }
}

WifiDisconnectReason ArduinoWifiAdapter::mapDisconnectReason(int wifiStatus) {
  if (wifiStatus == WL_NO_SSID_AVAIL) {
    return WifiDisconnectReason::kAccessPointUnavailable;
  }
  if (wifiStatus == WL_CONNECT_FAILED) {
    return WifiDisconnectReason::kAuthenticationFailed;
  }
  if (wifiStatus == WL_CONNECTION_LOST) {
    return WifiDisconnectReason::kSignalLost;
  }
  return WifiDisconnectReason::kUnknown;
}

}  // namespace reeflow::network
