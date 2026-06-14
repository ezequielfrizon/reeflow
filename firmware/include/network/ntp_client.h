#pragma once

#include "network/network_config.h"
#include "network/network_types.h"

namespace reeflow::network {

class NtpClient {
 public:
  virtual ~NtpClient() = default;

  virtual NtpSyncResult startSync(const NetworkConfig& config,
                                  uint32_t nowMillis) = 0;
  virtual NtpStatus status() const = 0;
  virtual NtpTimestamp timestamp() const = 0;
};

}  // namespace reeflow::network
