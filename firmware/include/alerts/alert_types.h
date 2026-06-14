#pragma once

#include <stddef.h>
#include <stdint.h>

namespace reeflow::alerts {

constexpr size_t kAlertReasonMaxLength = 80;
constexpr size_t kAlertOriginMaxLength = 32;
constexpr size_t kMaxAlertHistoryEntries = 24;
constexpr uint32_t kDefaultAlertCooldownMillis = 300000;
constexpr uint32_t kMinAlertCooldownMillis = 1000;
constexpr uint32_t kMaxAlertCooldownMillis = 86400000;

enum class AlertCode : uint8_t {
  kTemperatureHigh = 0,
  kTemperatureLow = 1,
  kTemperatureSensorOffline = 2,
  kAtoSensorOffline = 3,
  kAtoTimeout = 4,
  kEsp32Offline = 5,
  kWifiOffline = 6,
  kMqttOffline = 7,
  kUnexpectedReboot = 8,
  kUnknown = 255,
};

constexpr size_t kAlertCodeCount = 9;

enum class AlertRecoveryCode : uint8_t {
  kTemperatureRecovered = 0,
  kTemperatureSensorRecovered = 1,
  kAtoRecovered = 2,
  kAtoSensorRecovered = 3,
  kEsp32Recovered = 4,
  kWifiRecovered = 5,
  kMqttRecovered = 6,
  kRebootAcknowledged = 7,
  kUnknown = 255,
};

enum class AlertCategory : uint8_t {
  kInfo,
  kWarning,
  kCritical,
};

enum class AlertPriority : uint8_t {
  kInfo,
  kWarning,
  kCritical,
};

enum class AlertState : uint8_t {
  kRaised,
  kRecovered,
};

enum class AlertSource : uint8_t {
  kLocalEventBus,
  kSystemState,
  kFirmwareHealth,
  kLocalContract,
};

enum class AlertProcessResult : uint8_t {
  kAccepted,
  kRecovered,
  kCooldownSuppressed,
  kUnknownAlert,
  kUnknownRecovery,
  kNoActiveAlert,
  kInvalidInput,
  kHistoryUnavailable,
};

enum class AlertPublishResult : uint8_t {
  kPublished,
  kUnavailable,
  kNotConnected,
  kRejected,
};

struct AlertSignal {
  AlertCode code;
  AlertState state;
  AlertSource source;
  uint32_t occurredAtMillis;
  const char* reason;
};

struct AlertHistoryEntry {
  AlertCode code;
  AlertRecoveryCode recoveryCode;
  AlertCategory category;
  AlertPriority priority;
  AlertState state;
  AlertSource source;
  uint32_t occurredAtMillis;
  uint32_t repeatCount;
  char reason[kAlertReasonMaxLength];
};

constexpr bool isKnownAlertCode(AlertCode code) {
  return code != AlertCode::kUnknown &&
         static_cast<uint8_t>(code) < kAlertCodeCount;
}

constexpr size_t alertCodeIndex(AlertCode code) {
  return isKnownAlertCode(code) ? static_cast<size_t>(code)
                                : kAlertCodeCount;
}

}  // namespace reeflow::alerts
