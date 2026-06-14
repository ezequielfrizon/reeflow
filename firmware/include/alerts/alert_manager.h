#pragma once

#include "alerts/alert_cooldown.h"
#include "alerts/alert_events.h"
#include "alerts/alert_history.h"
#include "core/events/event_bus.h"
#include "core/state/system_state.h"

namespace reeflow::alerts {

struct AlertRequest {
  AlertCode code;
  AlertSource source;
  uint32_t occurredAtMillis;
  const char* reason;
};

struct AlertRecoveryRequest {
  AlertRecoveryCode recoveryCode;
  AlertSource source;
  uint32_t occurredAtMillis;
  const char* reason;
};

class AlertManager final {
 public:
  AlertManager(core::events::EventBus& eventBus,
               AlertHistoryStore& historyStore,
               const AlertCooldownConfig& cooldownConfig);

  AlertProcessResult raise(const AlertRequest& request);
  AlertProcessResult recover(const AlertRecoveryRequest& request);

  const AlertCooldownConfig& cooldownConfig() const;
  void updateCooldownConfig(const AlertCooldownConfig& cooldownConfig);
  void resetCooldown();

 private:
  AlertProcessResult recordHistoryAndPublish(
      const AlertHistoryEntry& entry, AlertEventType eventType);
  void publishAlertEvent(const AlertHistoryEntry& entry,
                         AlertEventType eventType) const;
  bool upsertActiveAlert(core::state::AlertsState& alerts,
                         const AlertHistoryEntry& entry);
  bool removeRecoveredAlerts(core::state::AlertsState& alerts,
                             AlertRecoveryCode recoveryCode,
                             AlertCode* recoveredCodes,
                             size_t recoveredCodesCapacity,
                             size_t& recoveredCount) const;
  void recalculatePriority(core::state::AlertsState& alerts) const;

  core::events::EventBus& eventBus_;
  AlertHistoryStore& historyStore_;
  AlertCooldownConfig cooldownConfig_;
  AlertCooldownState cooldownState_ = {};
  uint32_t repeatCounts_[kAlertCodeCount] = {};
};

}  // namespace reeflow::alerts
