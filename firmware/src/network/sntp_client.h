#pragma once

#include "network/ntp_client.h"

namespace reeflow::network {

class SntpClient final : public NtpClient {
 public:
  NtpSyncResult startSync(const NetworkConfig& config,
                          uint32_t nowMillis) override;
  NtpStatus status() const override;
  NtpTimestamp timestamp() const override;

 private:
  static bool validEpoch(uint32_t epochSeconds);

  mutable NtpStatus status_ = {};
  bool syncStarted_ = false;
};

}  // namespace reeflow::network
