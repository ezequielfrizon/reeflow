#include "alerts/alert_manager.h"

#include <string.h>

#include "alerts/alert_registry.h"

namespace reeflow::alerts {
namespace {

using reeflow::core::state::ActiveAlertState;
using reeflow::core::state::AlertsState;
using SystemAlertPriority = reeflow::core::state::AlertPriority;

SystemAlertPriority toSystemPriority(reeflow::alerts::AlertPriority priority) {
  switch (priority) {
    case reeflow::alerts::AlertPriority::kInfo:
      return SystemAlertPriority::kInfo;
    case reeflow::alerts::AlertPriority::kWarning:
      return SystemAlertPriority::kWarning;
    case reeflow::alerts::AlertPriority::kCritical:
      return SystemAlertPriority::kCritical;
  }

  return SystemAlertPriority::kInfo;
}

uint8_t priorityWeight(SystemAlertPriority priority) {
  switch (priority) {
    case SystemAlertPriority::kInfo:
      return 0;
    case SystemAlertPriority::kWarning:
      return 1;
    case SystemAlertPriority::kCritical:
      return 2;
  }

  return 0;
}

void copyAlertName(char* destination, size_t capacity, const char* value) {
  if (destination == nullptr || capacity == 0) {
    return;
  }

  if (value == nullptr) {
    destination[0] = '\0';
    return;
  }

  strncpy(destination, value, capacity - 1);
  destination[capacity - 1] = '\0';
}

bool sameAlertName(const ActiveAlertState& activeAlert, AlertCode code) {
  return activeAlert.active &&
         strncmp(activeAlert.name, alertCodeName(code),
                 core::state::kAlertNameMaxLength) == 0;
}

bool recoveryMatches(AlertCode code, AlertRecoveryCode recoveryCode) {
  return recoveryForAlert(code) == recoveryCode;
}

}  // namespace

AlertManager::AlertManager(core::events::EventBus& eventBus,
                           AlertHistoryStore& historyStore,
                           const AlertCooldownConfig& cooldownConfig)
    : eventBus_(eventBus),
      historyStore_(historyStore),
      cooldownConfig_(cooldownConfig) {
  resetCooldown();
}

AlertProcessResult AlertManager::raise(const AlertRequest& request) {
  const AlertDefinition* definition = alertDefinition(request.code);
  if (definition == nullptr) {
    return AlertProcessResult::kUnknownAlert;
  }

  if (alertCooldownSuppresses(cooldownConfig_, cooldownState_, request.code,
                              request.occurredAtMillis)) {
    AlertHistoryEntry suppressedEntry = makeAlertHistoryEntry(
        request.code, AlertState::kRaised, request.source,
        request.occurredAtMillis, repeatCounts_[alertCodeIndex(request.code)],
        request.reason);
    publishAlertEvent(suppressedEntry,
                      AlertEventType::kAlertCooldownSuppressed);
    return AlertProcessResult::kCooldownSuppressed;
  }

  const size_t index = alertCodeIndex(request.code);
  repeatCounts_[index] += 1;
  markAlertCooldownRaised(cooldownState_, request.code,
                          request.occurredAtMillis);

  AlertHistoryEntry entry = makeAlertHistoryEntry(
      request.code, AlertState::kRaised, request.source,
      request.occurredAtMillis, repeatCounts_[index], request.reason);

  AlertsState alerts = core::state::currentSystemState().alerts;
  upsertActiveAlert(alerts, entry);
  copyAlertName(alerts.lastAlert, sizeof(alerts.lastAlert), definition->name);
  recalculatePriority(alerts);
  core::state::updateAlertsState(alerts);

  return recordHistoryAndPublish(entry, AlertEventType::kAlertRaised);
}

AlertProcessResult AlertManager::recover(
    const AlertRecoveryRequest& request) {
  if (request.recoveryCode == AlertRecoveryCode::kUnknown) {
    return AlertProcessResult::kUnknownRecovery;
  }

  AlertsState alerts = core::state::currentSystemState().alerts;
  AlertCode recoveredCodes[core::state::kMaxActiveAlerts] = {};
  size_t recoveredCount = 0;
  if (!removeRecoveredAlerts(alerts, request.recoveryCode, recoveredCodes,
                             core::state::kMaxActiveAlerts,
                             recoveredCount)) {
    return AlertProcessResult::kNoActiveAlert;
  }

  copyAlertName(alerts.lastRecovery, sizeof(alerts.lastRecovery),
                alertRecoveryName(request.recoveryCode));
  recalculatePriority(alerts);
  core::state::updateAlertsState(alerts);

  AlertProcessResult result = AlertProcessResult::kRecovered;
  for (size_t index = 0; index < recoveredCount; ++index) {
    const AlertCode code = recoveredCodes[index];
    const size_t codeIndex = alertCodeIndex(code);
    const uint32_t repeatCount =
        codeIndex < kAlertCodeCount ? repeatCounts_[codeIndex] : 0;
    AlertHistoryEntry entry = makeAlertHistoryEntry(
        code, AlertState::kRecovered, request.source,
        request.occurredAtMillis, repeatCount, request.reason);
    entry.recoveryCode = request.recoveryCode;

    const AlertProcessResult entryResult =
        recordHistoryAndPublish(entry, AlertEventType::kAlertRecovered);
    if (entryResult == AlertProcessResult::kHistoryUnavailable) {
      result = entryResult;
    }
  }

  return result;
}

const AlertCooldownConfig& AlertManager::cooldownConfig() const {
  return cooldownConfig_;
}

void AlertManager::updateCooldownConfig(
    const AlertCooldownConfig& cooldownConfig) {
  cooldownConfig_ = cooldownConfig;
}

void AlertManager::resetCooldown() {
  resetAlertCooldownState(cooldownState_);
  for (size_t index = 0; index < kAlertCodeCount; ++index) {
    repeatCounts_[index] = 0;
  }
}

AlertProcessResult AlertManager::recordHistoryAndPublish(
    const AlertHistoryEntry& entry, AlertEventType eventType) {
  const AlertHistoryResult historyResult = historyStore_.append(entry);
  if (historyResult == AlertHistoryResult::kRecorded) {
    publishAlertEvent(entry, AlertEventType::kAlertHistoryRecorded);
    publishAlertEvent(entry, eventType);
    return entry.state == AlertState::kRecovered
               ? AlertProcessResult::kRecovered
               : AlertProcessResult::kAccepted;
  }

  publishAlertEvent(entry, eventType);
  return AlertProcessResult::kHistoryUnavailable;
}

void AlertManager::publishAlertEvent(const AlertHistoryEntry& entry,
                                     AlertEventType eventType) const {
  AlertEvent payload = {};
  payload.type = eventType;
  payload.code = entry.code;
  payload.recoveryCode = entry.recoveryCode;
  payload.category = entry.category;
  payload.priority = entry.priority;
  payload.source = entry.source;
  payload.occurredAtMillis = entry.occurredAtMillis;
  payload.repeatCount = entry.repeatCount;
  payload.historyEntry = &entry;

  core::events::Event event = {};
  event.type = alertCoreEventType(eventType);
  event.stateArea = core::events::StateArea::kAlerts;
  event.payload = &payload;
  eventBus_.publish(event);
}

bool AlertManager::upsertActiveAlert(AlertsState& alerts,
                                     const AlertHistoryEntry& entry) {
  const char* name = alertCodeName(entry.code);
  const SystemAlertPriority systemPriority = toSystemPriority(entry.priority);

  for (size_t index = 0; index < core::state::kMaxActiveAlerts; ++index) {
    ActiveAlertState& activeAlert = alerts.activeAlerts[index];
    if (!sameAlertName(activeAlert, entry.code)) {
      continue;
    }

    activeAlert.priority = systemPriority;
    activeAlert.raisedAt = entry.occurredAtMillis;
    return true;
  }

  for (size_t index = 0; index < core::state::kMaxActiveAlerts; ++index) {
    ActiveAlertState& activeAlert = alerts.activeAlerts[index];
    if (activeAlert.active) {
      continue;
    }

    activeAlert.active = true;
    copyAlertName(activeAlert.name, sizeof(activeAlert.name), name);
    activeAlert.priority = systemPriority;
    activeAlert.raisedAt = entry.occurredAtMillis;
    if (alerts.activeAlertCount < core::state::kMaxActiveAlerts) {
      alerts.activeAlertCount += 1;
    }
    return true;
  }

  size_t replacementIndex = 0;
  for (size_t index = 1; index < core::state::kMaxActiveAlerts; ++index) {
    const ActiveAlertState& candidate = alerts.activeAlerts[index];
    const ActiveAlertState& replacement =
        alerts.activeAlerts[replacementIndex];
    if (priorityWeight(candidate.priority) <
            priorityWeight(replacement.priority) ||
        (candidate.priority == replacement.priority &&
         candidate.raisedAt < replacement.raisedAt)) {
      replacementIndex = index;
    }
  }

  ActiveAlertState& replacement = alerts.activeAlerts[replacementIndex];
  replacement.active = true;
  copyAlertName(replacement.name, sizeof(replacement.name), name);
  replacement.priority = systemPriority;
  replacement.raisedAt = entry.occurredAtMillis;
  alerts.activeAlertCount = core::state::kMaxActiveAlerts;
  return true;
}

bool AlertManager::removeRecoveredAlerts(
    AlertsState& alerts, AlertRecoveryCode recoveryCode,
    AlertCode* recoveredCodes, size_t recoveredCodesCapacity,
    size_t& recoveredCount) const {
  recoveredCount = 0;

  for (size_t index = 0; index < core::state::kMaxActiveAlerts; ++index) {
    ActiveAlertState& activeAlert = alerts.activeAlerts[index];
    if (!activeAlert.active) {
      continue;
    }

    for (size_t alertIndex = 0; alertIndex < kAlertCodeCount; ++alertIndex) {
      const AlertDefinition* definition = alertDefinitionByIndex(alertIndex);
      if (definition == nullptr ||
          !sameAlertName(activeAlert, definition->code) ||
          !recoveryMatches(definition->code, recoveryCode)) {
        continue;
      }

      if (recoveredCount < recoveredCodesCapacity) {
        recoveredCodes[recoveredCount] = definition->code;
      }
      recoveredCount += 1;
      activeAlert = {};
      break;
    }
  }

  if (recoveredCount == 0) {
    return false;
  }

  ActiveAlertState compacted[core::state::kMaxActiveAlerts] = {};
  uint8_t compactedCount = 0;
  for (size_t index = 0; index < core::state::kMaxActiveAlerts; ++index) {
    if (!alerts.activeAlerts[index].active) {
      continue;
    }

    compacted[compactedCount] = alerts.activeAlerts[index];
    compactedCount += 1;
  }

  for (size_t index = 0; index < core::state::kMaxActiveAlerts; ++index) {
    alerts.activeAlerts[index] = compacted[index];
  }
  alerts.activeAlertCount = compactedCount;
  return true;
}

void AlertManager::recalculatePriority(AlertsState& alerts) const {
  SystemAlertPriority highest = SystemAlertPriority::kInfo;
  uint8_t activeCount = 0;

  for (size_t index = 0; index < core::state::kMaxActiveAlerts; ++index) {
    const ActiveAlertState& activeAlert = alerts.activeAlerts[index];
    if (!activeAlert.active) {
      continue;
    }

    activeCount += 1;
    if (priorityWeight(activeAlert.priority) > priorityWeight(highest)) {
      highest = activeAlert.priority;
    }
  }

  alerts.activeAlertCount = activeCount;
  alerts.priority = highest;
}

}  // namespace reeflow::alerts
