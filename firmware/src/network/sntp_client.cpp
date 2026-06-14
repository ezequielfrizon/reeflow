#include "sntp_client.h"

#include <Arduino.h>
#include <time.h>

namespace reeflow::network {

NtpSyncResult SntpClient::startSync(const NetworkConfig& config,
                                    uint32_t nowMillis) {
  const NtpTimestamp currentTimestamp = timestamp();
  if (currentTimestamp.available) {
    status_.status = NtpSyncStatus::kSynced;
    status_.timestamp = currentTimestamp;
    status_.lastAttemptMillis = nowMillis;
    return NtpSyncResult::kAlreadySynced;
  }

  configTime(0, 0, config.primaryNtpServer, config.secondaryNtpServer);
  syncStarted_ = true;
  status_.status = NtpSyncStatus::kSyncing;
  status_.timestamp = {};
  status_.lastAttemptMillis = nowMillis;
  return NtpSyncResult::kStarted;
}

NtpStatus SntpClient::status() const {
  if (!syncStarted_) {
    status_.status = NtpSyncStatus::kNotStarted;
    status_.timestamp = {};
    return status_;
  }

  const NtpTimestamp currentTimestamp = timestamp();
  if (currentTimestamp.available) {
    status_.status = NtpSyncStatus::kSynced;
    status_.timestamp = currentTimestamp;
  } else if (status_.status != NtpSyncStatus::kTimeout &&
             status_.status != NtpSyncStatus::kServerUnavailable &&
             status_.status != NtpSyncStatus::kFailed) {
    status_.status = NtpSyncStatus::kSyncing;
    status_.timestamp = {};
  }

  return status_;
}

NtpTimestamp SntpClient::timestamp() const {
  const time_t now = time(nullptr);
  if (now < 0) {
    return {};
  }

  const uint32_t epochSeconds = static_cast<uint32_t>(now);
  return {validEpoch(epochSeconds), epochSeconds};
}

bool SntpClient::validEpoch(uint32_t epochSeconds) {
  return epochSeconds >= 1600000000UL;
}

}  // namespace reeflow::network
