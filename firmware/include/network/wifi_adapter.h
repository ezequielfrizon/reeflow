#pragma once

#include "network/network_types.h"

namespace reeflow::network {

class WifiAdapter {
 public:
  virtual ~WifiAdapter() = default;

  virtual bool begin() = 0;
  virtual WifiConnectResult connect(const WifiCredentials& credentials,
                                    uint32_t timeoutMillis) = 0;
  virtual void disconnect() = 0;
  virtual WifiStatus status() const = 0;
  virtual const char* ipAddress() const = 0;
  virtual int32_t rssi() const = 0;
  virtual WifiDisconnectReason lastDisconnectReason() const = 0;
};

class InternetProbe {
 public:
  virtual ~InternetProbe() = default;

  virtual InternetProbeResult probe(uint32_t nowMillis) = 0;
};

}  // namespace reeflow::network
