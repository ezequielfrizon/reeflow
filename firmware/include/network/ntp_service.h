#pragma once

#include "core/events/event_bus.h"
#include "network/network_config.h"
#include "network/network_events.h"
#include "network/ntp_client.h"
#include "network/network_types.h"

namespace reeflow::network {

struct NtpServiceSnapshot {
  NtpStatus status;
  NtpSyncResult lastSyncResult;
  bool syncInProgress;
};

class NtpService {
 public:
  NtpService(NtpClient& client, core::events::EventBus& eventBus,
             NetworkConfig networkConfig = makeDefaultNetworkConfig());

  void tick(uint32_t nowMillis);
  NtpServiceSnapshot snapshot() const;

 private:
  void startSync(uint32_t nowMillis);
  void pollSync(uint32_t nowMillis);
  void handleSynced(uint32_t nowMillis, const NtpStatus& status);
  void handleFailure(uint32_t nowMillis, NtpSyncResult result,
                     NtpSyncStatus status);
  void publish(NetworkEventType type, uint32_t nowMillis,
               NtpSyncStatus status);
  bool wifiConnected() const;

  NtpClient& client_;
  core::events::EventBus& eventBus_;
  NetworkConfig networkConfig_;
  NtpStatus status_ = {};
  NtpSyncResult lastSyncResult_ = NtpSyncResult::kNetworkUnavailable;
  uint32_t syncStartedAtMillis_ = 0;
  bool syncInProgress_ = false;
};

}  // namespace reeflow::network
