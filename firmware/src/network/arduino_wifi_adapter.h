#pragma once

#include "network/wifi_adapter.h"

namespace reeflow::network {

class ArduinoWifiAdapter final : public WifiAdapter {
 public:
  bool begin() override;
  WifiConnectResult connect(const WifiCredentials& credentials,
                            uint32_t timeoutMillis) override;
  void disconnect() override;
  WifiStatus status() const override;
  const char* ipAddress() const override;
  int32_t rssi() const override;
  WifiDisconnectReason lastDisconnectReason() const override;

 private:
  void refreshStatus() const;
  static WifiDisconnectReason mapDisconnectReason(int wifiStatus);

  mutable WifiStatus status_ = {};
};

}  // namespace reeflow::network
