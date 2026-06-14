#include "alerts/alert_events.h"

namespace reeflow::alerts {

const char* alertEventName(AlertEventType eventType) {
  switch (eventType) {
    case AlertEventType::kAlertRaised:
      return "ALERT_RAISED";
    case AlertEventType::kAlertRecovered:
      return "ALERT_RECOVERED";
    case AlertEventType::kAlertCooldownSuppressed:
      return "ALERT_COOLDOWN_SUPPRESSED";
    case AlertEventType::kAlertHistoryRecorded:
      return "ALERT_HISTORY_RECORDED";
  }

  return "UNKNOWN_ALERT_EVENT";
}

core::events::EventType alertCoreEventType(AlertEventType eventType) {
  switch (eventType) {
    case AlertEventType::kAlertRaised:
      return core::events::EventType::kAlertRaised;
    case AlertEventType::kAlertRecovered:
      return core::events::EventType::kAlertRecovered;
    case AlertEventType::kAlertCooldownSuppressed:
      return core::events::EventType::kAlertCooldownSuppressed;
    case AlertEventType::kAlertHistoryRecorded:
      return core::events::EventType::kAlertHistoryRecorded;
  }

  return core::events::EventType::kAlertRaised;
}

}  // namespace reeflow::alerts
