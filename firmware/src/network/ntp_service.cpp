#include "network/ntp_service.h"

#include "core/state/system_state.h"

namespace reeflow::network {

NtpService::NtpService(NtpClient& client, core::events::EventBus& eventBus,
                       NetworkConfig networkConfig)
    : client_(client),
      eventBus_(eventBus),
      networkConfig_(networkConfig) {
  status_.status = NtpSyncStatus::kNotStarted;
  status_.timestamp = {};
}

void NtpService::tick(uint32_t nowMillis) {
  if (!wifiConnected()) {
    if (syncInProgress_) {
      handleFailure(nowMillis, NtpSyncResult::kNetworkUnavailable,
                    NtpSyncStatus::kFailed);
    } else {
      lastSyncResult_ = NtpSyncResult::kNetworkUnavailable;
      status_.status = NtpSyncStatus::kNotStarted;
      status_.timestamp = {};
    }
    return;
  }

  if (syncInProgress_) {
    pollSync(nowMillis);
    return;
  }

  if (status_.status == NtpSyncStatus::kSynced) {
    return;
  }

  startSync(nowMillis);
}

NtpServiceSnapshot NtpService::snapshot() const {
  return {status_, lastSyncResult_, syncInProgress_};
}

void NtpService::startSync(uint32_t nowMillis) {
  syncStartedAtMillis_ = nowMillis;
  lastSyncResult_ = client_.startSync(networkConfig_, nowMillis);
  status_ = client_.status();

  if (lastSyncResult_ == NtpSyncResult::kSynced ||
      lastSyncResult_ == NtpSyncResult::kAlreadySynced ||
      status_.status == NtpSyncStatus::kSynced) {
    handleSynced(nowMillis, status_);
    return;
  }

  if (lastSyncResult_ == NtpSyncResult::kStarted) {
    syncInProgress_ = true;
    status_.status = NtpSyncStatus::kSyncing;
    status_.lastAttemptMillis = nowMillis;
    return;
  }

  NtpSyncStatus failedStatus = status_.status;
  if (failedStatus == NtpSyncStatus::kNotStarted ||
      failedStatus == NtpSyncStatus::kSyncing) {
    failedStatus = NtpSyncStatus::kFailed;
  }
  handleFailure(nowMillis, lastSyncResult_, failedStatus);
}

void NtpService::pollSync(uint32_t nowMillis) {
  status_ = client_.status();

  if (status_.status == NtpSyncStatus::kSynced) {
    handleSynced(nowMillis, status_);
    return;
  }

  if (status_.status == NtpSyncStatus::kTimeout) {
    handleFailure(nowMillis, NtpSyncResult::kTimeout, status_.status);
    return;
  }

  if (status_.status == NtpSyncStatus::kServerUnavailable) {
    handleFailure(nowMillis, NtpSyncResult::kServerUnavailable,
                  status_.status);
    return;
  }

  if (status_.status == NtpSyncStatus::kFailed) {
    handleFailure(nowMillis, NtpSyncResult::kClientFailure, status_.status);
    return;
  }

  if (nowMillis - syncStartedAtMillis_ >=
      networkConfig_.ntpSyncTimeoutMillis) {
    handleFailure(nowMillis, NtpSyncResult::kTimeout,
                  NtpSyncStatus::kTimeout);
  }
}

void NtpService::handleSynced(uint32_t nowMillis, const NtpStatus& status) {
  status_ = status;
  status_.status = NtpSyncStatus::kSynced;
  syncInProgress_ = false;
  lastSyncResult_ = NtpSyncResult::kSynced;
  publish(NetworkEventType::kNtpSynced, nowMillis, status_.status);
}

void NtpService::handleFailure(uint32_t nowMillis, NtpSyncResult result,
                               NtpSyncStatus status) {
  syncInProgress_ = false;
  lastSyncResult_ = result;
  status_.status = status;
  status_.timestamp = {};
  publish(NetworkEventType::kNtpSyncFailed, nowMillis, status);
}

void NtpService::publish(NetworkEventType type, uint32_t nowMillis,
                         NtpSyncStatus status) {
  NetworkEvent event = {};
  event.type = type;
  event.ntpStatus = status;
  event.occurredAtMillis = nowMillis;
  publishNetworkEvent(event, eventBus_);
}

bool NtpService::wifiConnected() const {
  return core::state::currentSystemState().network.wifiConnected;
}

}  // namespace reeflow::network
