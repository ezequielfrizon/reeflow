#pragma once

#include "network/ntp_client.h"

namespace reeflow::test::fakes {

class FakeNtpClient final : public reeflow::network::NtpClient {
 public:
  reeflow::network::NtpSyncResult startSync(
      const reeflow::network::NetworkConfig& config,
      uint32_t nowMillis) override {
    syncCallCount_ += 1;
    lastConfig_ = config;
    status_.lastAttemptMillis = nowMillis;
    applyResult(syncResult_);
    return syncResult_;
  }

  reeflow::network::NtpStatus status() const override { return status_; }

  reeflow::network::NtpTimestamp timestamp() const override {
    return status_.timestamp;
  }

  void simulateSynced(uint32_t epochSeconds) {
    syncResult_ = reeflow::network::NtpSyncResult::kSynced;
    status_.status = reeflow::network::NtpSyncStatus::kSynced;
    status_.timestamp = {true, epochSeconds};
  }

  void simulateSyncing() {
    syncResult_ = reeflow::network::NtpSyncResult::kStarted;
    status_.status = reeflow::network::NtpSyncStatus::kSyncing;
    status_.timestamp = {};
  }

  void simulateTimeout() {
    syncResult_ = reeflow::network::NtpSyncResult::kTimeout;
    status_.status = reeflow::network::NtpSyncStatus::kTimeout;
    status_.timestamp = {};
  }

  void simulateServerUnavailable() {
    syncResult_ = reeflow::network::NtpSyncResult::kServerUnavailable;
    status_.status = reeflow::network::NtpSyncStatus::kServerUnavailable;
    status_.timestamp = {};
  }

  void completeSync(uint32_t epochSeconds) {
    status_.status = reeflow::network::NtpSyncStatus::kSynced;
    status_.timestamp = {true, epochSeconds};
  }

  void failWithTimeout() {
    status_.status = reeflow::network::NtpSyncStatus::kTimeout;
    status_.timestamp = {};
  }

  uint16_t syncCallCount() const { return syncCallCount_; }
  reeflow::network::NetworkConfig lastConfig() const { return lastConfig_; }

 private:
  void applyResult(reeflow::network::NtpSyncResult result) {
    if (result == reeflow::network::NtpSyncResult::kSynced) {
      status_.status = reeflow::network::NtpSyncStatus::kSynced;
    } else if (result == reeflow::network::NtpSyncResult::kTimeout) {
      status_.status = reeflow::network::NtpSyncStatus::kTimeout;
      status_.timestamp = {};
    } else if (result ==
               reeflow::network::NtpSyncResult::kServerUnavailable) {
      status_.status = reeflow::network::NtpSyncStatus::kServerUnavailable;
      status_.timestamp = {};
    } else if (result == reeflow::network::NtpSyncResult::kStarted) {
      status_.status = reeflow::network::NtpSyncStatus::kSyncing;
      status_.timestamp = {};
    }
  }

  reeflow::network::NtpSyncResult syncResult_ =
      reeflow::network::NtpSyncResult::kStarted;
  reeflow::network::NtpStatus status_ = {};
  reeflow::network::NetworkConfig lastConfig_ = {};
  uint16_t syncCallCount_ = 0;
};

}  // namespace reeflow::test::fakes
