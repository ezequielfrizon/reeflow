#pragma once

#include "alerts/alert_types.h"
#include "core/events/event_bus.h"

namespace reeflow::alerts {

enum class AlertEventType : uint8_t {
  kAlertRaised,
  kAlertRecovered,
  kAlertCooldownSuppressed,
  kAlertHistoryRecorded,
};

struct AlertEvent {
  AlertEventType type;
  AlertCode code;
  AlertRecoveryCode recoveryCode;
  AlertCategory category;
  AlertPriority priority;
  AlertSource source;
  uint32_t occurredAtMillis;
  uint32_t repeatCount;
  const AlertHistoryEntry* historyEntry;
};

const char* alertEventName(AlertEventType eventType);
core::events::EventType alertCoreEventType(AlertEventType eventType);

}  // namespace reeflow::alerts
