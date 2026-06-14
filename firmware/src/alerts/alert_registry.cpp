#include "alerts/alert_registry.h"

namespace reeflow::alerts {
namespace {

constexpr AlertDefinition kAlertDefinitions[kAlertCodeCount] = {
    {AlertCode::kTemperatureHigh, "TEMPERATURE_HIGH",
     AlertCategory::kCritical, AlertPriority::kCritical,
     AlertRecoveryCode::kTemperatureRecovered},
    {AlertCode::kTemperatureLow, "TEMPERATURE_LOW",
     AlertCategory::kWarning, AlertPriority::kWarning,
     AlertRecoveryCode::kTemperatureRecovered},
    {AlertCode::kTemperatureSensorOffline, "TEMPERATURE_SENSOR_OFFLINE",
     AlertCategory::kCritical, AlertPriority::kCritical,
     AlertRecoveryCode::kTemperatureSensorRecovered},
    {AlertCode::kAtoSensorOffline, "ATO_SENSOR_OFFLINE",
     AlertCategory::kCritical, AlertPriority::kCritical,
     AlertRecoveryCode::kAtoSensorRecovered},
    {AlertCode::kAtoTimeout, "ATO_TIMEOUT", AlertCategory::kCritical,
     AlertPriority::kCritical, AlertRecoveryCode::kAtoRecovered},
    {AlertCode::kEsp32Offline, "ESP32_OFFLINE", AlertCategory::kCritical,
     AlertPriority::kCritical, AlertRecoveryCode::kEsp32Recovered},
    {AlertCode::kWifiOffline, "WIFI_OFFLINE", AlertCategory::kWarning,
     AlertPriority::kWarning, AlertRecoveryCode::kWifiRecovered},
    {AlertCode::kMqttOffline, "MQTT_OFFLINE", AlertCategory::kWarning,
     AlertPriority::kWarning, AlertRecoveryCode::kMqttRecovered},
    {AlertCode::kUnexpectedReboot, "UNEXPECTED_REBOOT",
     AlertCategory::kWarning, AlertPriority::kWarning,
     AlertRecoveryCode::kRebootAcknowledged},
};

}  // namespace

const AlertDefinition* alertDefinition(AlertCode code) {
  const size_t index = alertCodeIndex(code);
  if (index >= kAlertCodeCount) {
    return nullptr;
  }

  return &kAlertDefinitions[index];
}

const AlertDefinition* alertDefinitionByIndex(size_t index) {
  if (index >= kAlertCodeCount) {
    return nullptr;
  }

  return &kAlertDefinitions[index];
}

size_t alertDefinitionCount() {
  return kAlertCodeCount;
}

AlertRecoveryCode recoveryForAlert(AlertCode code) {
  const AlertDefinition* definition = alertDefinition(code);
  return definition == nullptr ? AlertRecoveryCode::kUnknown
                               : definition->recoveryCode;
}

AlertCode alertForRecovery(AlertRecoveryCode recoveryCode) {
  switch (recoveryCode) {
    case AlertRecoveryCode::kTemperatureRecovered:
      return AlertCode::kTemperatureHigh;
    case AlertRecoveryCode::kTemperatureSensorRecovered:
      return AlertCode::kTemperatureSensorOffline;
    case AlertRecoveryCode::kAtoRecovered:
      return AlertCode::kAtoTimeout;
    case AlertRecoveryCode::kAtoSensorRecovered:
      return AlertCode::kAtoSensorOffline;
    case AlertRecoveryCode::kEsp32Recovered:
      return AlertCode::kEsp32Offline;
    case AlertRecoveryCode::kWifiRecovered:
      return AlertCode::kWifiOffline;
    case AlertRecoveryCode::kMqttRecovered:
      return AlertCode::kMqttOffline;
    case AlertRecoveryCode::kRebootAcknowledged:
      return AlertCode::kUnexpectedReboot;
    case AlertRecoveryCode::kUnknown:
      return AlertCode::kUnknown;
  }

  return AlertCode::kUnknown;
}

const char* alertCodeName(AlertCode code) {
  const AlertDefinition* definition = alertDefinition(code);
  return definition == nullptr ? "UNKNOWN_ALERT" : definition->name;
}

const char* alertRecoveryName(AlertRecoveryCode recoveryCode) {
  switch (recoveryCode) {
    case AlertRecoveryCode::kTemperatureRecovered:
      return "TEMPERATURE_RECOVERED";
    case AlertRecoveryCode::kTemperatureSensorRecovered:
      return "TEMPERATURE_SENSOR_RECOVERED";
    case AlertRecoveryCode::kAtoRecovered:
      return "ATO_RECOVERED";
    case AlertRecoveryCode::kAtoSensorRecovered:
      return "ATO_SENSOR_RECOVERED";
    case AlertRecoveryCode::kEsp32Recovered:
      return "ESP32_RECOVERED";
    case AlertRecoveryCode::kWifiRecovered:
      return "WIFI_RECOVERED";
    case AlertRecoveryCode::kMqttRecovered:
      return "MQTT_RECOVERED";
    case AlertRecoveryCode::kRebootAcknowledged:
      return "REBOOT_ACKNOWLEDGED";
    case AlertRecoveryCode::kUnknown:
      return "UNKNOWN_RECOVERY";
  }

  return "UNKNOWN_RECOVERY";
}

const char* alertCategoryName(AlertCategory category) {
  switch (category) {
    case AlertCategory::kInfo:
      return "INFO";
    case AlertCategory::kWarning:
      return "WARNING";
    case AlertCategory::kCritical:
      return "CRITICAL";
  }

  return "UNKNOWN_CATEGORY";
}

const char* alertPriorityName(AlertPriority priority) {
  switch (priority) {
    case AlertPriority::kInfo:
      return "INFO";
    case AlertPriority::kWarning:
      return "WARNING";
    case AlertPriority::kCritical:
      return "CRITICAL";
  }

  return "UNKNOWN_PRIORITY";
}

const char* alertSourceName(AlertSource source) {
  switch (source) {
    case AlertSource::kLocalEventBus:
      return "LOCAL_EVENT_BUS";
    case AlertSource::kSystemState:
      return "SYSTEM_STATE";
    case AlertSource::kFirmwareHealth:
      return "FIRMWARE_HEALTH";
    case AlertSource::kLocalContract:
      return "LOCAL_CONTRACT";
  }

  return "UNKNOWN_SOURCE";
}

}  // namespace reeflow::alerts
