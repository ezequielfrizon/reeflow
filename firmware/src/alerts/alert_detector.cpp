#include "alerts/alert_detector.h"

namespace reeflow::alerts {

AlertDetector::AlertDetector(AlertManager& manager,
                             uint32_t esp32HeartbeatTimeoutMillis)
    : manager_(manager),
      esp32HeartbeatTimeoutMillis_(esp32HeartbeatTimeoutMillis == 0
                                       ? kDefaultEsp32HeartbeatTimeoutMillis
                                       : esp32HeartbeatTimeoutMillis) {}

void AlertDetector::observeEvent(const core::events::Event& event,
                                 const core::state::SystemState& state,
                                 uint32_t nowMillis) {
  if (event.type == core::events::EventType::kSystemStateChanged) {
    switch (event.stateArea) {
      case core::events::StateArea::kTemperature:
        evaluateTemperature(state.temperature.status, nowMillis);
        return;
      case core::events::StateArea::kAto:
        evaluateAto(state.ato.status, nowMillis);
        return;
      case core::events::StateArea::kNetwork:
        evaluateNetwork(state.network, nowMillis);
        evaluateEsp32Heartbeat(state.network, nowMillis);
        return;
      case core::events::StateArea::kSystemHealth:
        evaluateSystemHealth(state.systemHealth, nowMillis);
        return;
      case core::events::StateArea::kSystem:
      case core::events::StateArea::kWaterLevel:
      case core::events::StateArea::kLighting:
      case core::events::StateArea::kRelays:
      case core::events::StateArea::kModes:
      case core::events::StateArea::kAlerts:
        return;
    }
  }

  handleDirectEvent(event.type, nowMillis);
}

void AlertDetector::evaluateSnapshot(
    const core::state::SystemState& state, uint32_t nowMillis) {
  evaluateTemperature(state.temperature.status, nowMillis);
  evaluateAto(state.ato.status, nowMillis);
  evaluateNetwork(state.network, nowMillis);
  evaluateSystemHealth(state.systemHealth, nowMillis);
  evaluateEsp32Heartbeat(state.network, nowMillis);
}

void AlertDetector::reset() {
  hasTemperatureStatus_ = false;
  hasAtoStatus_ = false;
  hasWifiState_ = false;
  hasMqttState_ = false;
  hasSystemHealth_ = false;
  temperatureConditionActive_ = false;
  temperatureSensorOfflineActive_ = false;
  atoTimeoutActive_ = false;
  atoSensorOfflineActive_ = false;
  wifiOfflineActive_ = false;
  mqttOfflineActive_ = false;
  esp32OfflineActive_ = false;
  unexpectedRebootActive_ = false;
  lastTemperatureStatus_ = core::state::TemperatureStatus::kNormal;
  lastAtoStatus_ = core::state::AtoStatus::kNormal;
  lastWifiConnected_ = false;
  lastMqttConnected_ = false;
  lastWatchdogTriggered_ = false;
}

void AlertDetector::evaluateTemperature(
    core::state::TemperatureStatus status, uint32_t nowMillis) {
  if (hasTemperatureStatus_ && status == lastTemperatureStatus_) {
    return;
  }

  if (status != core::state::TemperatureStatus::kSensorOffline &&
      temperatureSensorOfflineActive_) {
    recover(AlertRecoveryCode::kTemperatureSensorRecovered,
            AlertSource::kSystemState, nowMillis,
            "temperature sensor recovered");
    temperatureSensorOfflineActive_ = false;
  }

  if (status == core::state::TemperatureStatus::kNormal &&
      temperatureConditionActive_) {
    recover(AlertRecoveryCode::kTemperatureRecovered,
            AlertSource::kSystemState, nowMillis, "temperature recovered");
    temperatureConditionActive_ = false;
  }

  if (status == core::state::TemperatureStatus::kHigh) {
    if (temperatureConditionActive_) {
      recover(AlertRecoveryCode::kTemperatureRecovered,
              AlertSource::kSystemState, nowMillis,
              "temperature condition changed");
    }
    raise(AlertCode::kTemperatureHigh, AlertSource::kSystemState, nowMillis,
          "temperature high");
    temperatureConditionActive_ = true;
  } else if (status == core::state::TemperatureStatus::kLow) {
    if (temperatureConditionActive_) {
      recover(AlertRecoveryCode::kTemperatureRecovered,
              AlertSource::kSystemState, nowMillis,
              "temperature condition changed");
    }
    raise(AlertCode::kTemperatureLow, AlertSource::kSystemState, nowMillis,
          "temperature low");
    temperatureConditionActive_ = true;
  } else if (status == core::state::TemperatureStatus::kSensorOffline) {
    raise(AlertCode::kTemperatureSensorOffline, AlertSource::kSystemState,
          nowMillis, "temperature sensor offline");
    temperatureSensorOfflineActive_ = true;
  }

  lastTemperatureStatus_ = status;
  hasTemperatureStatus_ = true;
}

void AlertDetector::evaluateAto(core::state::AtoStatus status,
                                uint32_t nowMillis) {
  if (hasAtoStatus_ && status == lastAtoStatus_) {
    return;
  }

  if (status != core::state::AtoStatus::kSensorOffline &&
      atoSensorOfflineActive_) {
    recover(AlertRecoveryCode::kAtoSensorRecovered,
            AlertSource::kSystemState, nowMillis, "ATO sensor recovered");
    atoSensorOfflineActive_ = false;
  }

  if (status != core::state::AtoStatus::kTimeout && atoTimeoutActive_) {
    recover(AlertRecoveryCode::kAtoRecovered, AlertSource::kSystemState,
            nowMillis, "ATO recovered");
    atoTimeoutActive_ = false;
  }

  if (status == core::state::AtoStatus::kSensorOffline) {
    raise(AlertCode::kAtoSensorOffline, AlertSource::kSystemState, nowMillis,
          "ATO sensor offline");
    atoSensorOfflineActive_ = true;
  } else if (status == core::state::AtoStatus::kTimeout) {
    raise(AlertCode::kAtoTimeout, AlertSource::kSystemState, nowMillis,
          "ATO timeout");
    atoTimeoutActive_ = true;
  }

  lastAtoStatus_ = status;
  hasAtoStatus_ = true;
}

void AlertDetector::evaluateNetwork(const core::state::NetworkState& network,
                                    uint32_t nowMillis) {
  if ((!network.wifiConnected && hasWifiState_ && lastWifiConnected_) ||
      (!network.wifiConnected && wifiOfflineActive_)) {
    if (!wifiOfflineActive_) {
      raise(AlertCode::kWifiOffline, AlertSource::kSystemState, nowMillis,
            "Wi-Fi offline");
      wifiOfflineActive_ = true;
    }
  } else if (network.wifiConnected && wifiOfflineActive_) {
    recover(AlertRecoveryCode::kWifiRecovered, AlertSource::kSystemState,
            nowMillis, "Wi-Fi recovered");
    wifiOfflineActive_ = false;
  }

  if ((!network.mqttConnected && hasMqttState_ && lastMqttConnected_) ||
      (!network.mqttConnected && mqttOfflineActive_)) {
    if (!mqttOfflineActive_) {
      raise(AlertCode::kMqttOffline, AlertSource::kSystemState, nowMillis,
            "MQTT offline");
      mqttOfflineActive_ = true;
    }
  } else if (network.mqttConnected && mqttOfflineActive_) {
    recover(AlertRecoveryCode::kMqttRecovered, AlertSource::kSystemState,
            nowMillis, "MQTT recovered");
    mqttOfflineActive_ = false;
  }

  lastWifiConnected_ = network.wifiConnected;
  lastMqttConnected_ = network.mqttConnected;
  hasWifiState_ = true;
  hasMqttState_ = true;
}

void AlertDetector::evaluateSystemHealth(
    const core::state::SystemHealthState& systemHealth,
    uint32_t nowMillis) {
  const bool unexpectedReboot = systemHealth.watchdogTriggered;
  if (unexpectedReboot && !unexpectedRebootActive_) {
    raise(AlertCode::kUnexpectedReboot, AlertSource::kFirmwareHealth,
          nowMillis, "unexpected reboot");
    unexpectedRebootActive_ = true;
  } else if (!unexpectedReboot && unexpectedRebootActive_ &&
             hasSystemHealth_) {
    recover(AlertRecoveryCode::kRebootAcknowledged,
            AlertSource::kFirmwareHealth, nowMillis,
            "unexpected reboot acknowledged");
    unexpectedRebootActive_ = false;
  }

  lastWatchdogTriggered_ = systemHealth.watchdogTriggered;
  hasSystemHealth_ = true;
}

void AlertDetector::evaluateEsp32Heartbeat(
    const core::state::NetworkState& network, uint32_t nowMillis) {
  if (network.lastHeartbeat == 0) {
    return;
  }

  const bool heartbeatMissing =
      nowMillis - network.lastHeartbeat > esp32HeartbeatTimeoutMillis_;
  if (heartbeatMissing && !esp32OfflineActive_) {
    raise(AlertCode::kEsp32Offline, AlertSource::kFirmwareHealth, nowMillis,
          "ESP32 heartbeat missing");
    esp32OfflineActive_ = true;
  } else if (!heartbeatMissing && esp32OfflineActive_) {
    recover(AlertRecoveryCode::kEsp32Recovered,
            AlertSource::kFirmwareHealth, nowMillis,
            "ESP32 heartbeat recovered");
    esp32OfflineActive_ = false;
  }
}

void AlertDetector::handleDirectEvent(core::events::EventType eventType,
                                      uint32_t nowMillis) {
  switch (eventType) {
    case core::events::EventType::kTemperatureHigh:
      raise(AlertCode::kTemperatureHigh, AlertSource::kLocalEventBus,
            nowMillis, "temperature high");
      temperatureConditionActive_ = true;
      break;
    case core::events::EventType::kTemperatureLow:
      raise(AlertCode::kTemperatureLow, AlertSource::kLocalEventBus,
            nowMillis, "temperature low");
      temperatureConditionActive_ = true;
      break;
    case core::events::EventType::kTemperatureSensorOffline:
      raise(AlertCode::kTemperatureSensorOffline, AlertSource::kLocalEventBus,
            nowMillis, "temperature sensor offline");
      temperatureSensorOfflineActive_ = true;
      break;
    case core::events::EventType::kTemperatureSensorRecovered:
      recover(AlertRecoveryCode::kTemperatureSensorRecovered,
              AlertSource::kLocalEventBus, nowMillis,
              "temperature sensor recovered");
      temperatureSensorOfflineActive_ = false;
      break;
    case core::events::EventType::kAtoTimeout:
      raise(AlertCode::kAtoTimeout, AlertSource::kLocalEventBus, nowMillis,
            "ATO timeout");
      atoTimeoutActive_ = true;
      break;
    case core::events::EventType::kAtoSensorOffline:
      raise(AlertCode::kAtoSensorOffline, AlertSource::kLocalEventBus,
            nowMillis, "ATO sensor offline");
      atoSensorOfflineActive_ = true;
      break;
    case core::events::EventType::kAtoRecovered:
      recover(AlertRecoveryCode::kAtoRecovered, AlertSource::kLocalEventBus,
              nowMillis, "ATO recovered");
      atoTimeoutActive_ = false;
      atoSensorOfflineActive_ = false;
      break;
    case core::events::EventType::kWifiDisconnected:
    case core::events::EventType::kWifiReconnectFailed:
      raise(AlertCode::kWifiOffline, AlertSource::kLocalEventBus, nowMillis,
            "Wi-Fi offline");
      wifiOfflineActive_ = true;
      break;
    case core::events::EventType::kWifiConnected:
      recover(AlertRecoveryCode::kWifiRecovered, AlertSource::kLocalEventBus,
              nowMillis, "Wi-Fi recovered");
      wifiOfflineActive_ = false;
      break;
    case core::events::EventType::kMqttDisconnected:
      if (hasMqttState_ && lastMqttConnected_) {
        raise(AlertCode::kMqttOffline, AlertSource::kLocalEventBus,
              nowMillis, "MQTT offline");
        mqttOfflineActive_ = true;
      }
      break;
    case core::events::EventType::kMqttConnected:
    case core::events::EventType::kMqttReconnected:
      recover(AlertRecoveryCode::kMqttRecovered, AlertSource::kLocalEventBus,
              nowMillis, "MQTT recovered");
      mqttOfflineActive_ = false;
      lastMqttConnected_ = true;
      hasMqttState_ = true;
      break;
    case core::events::EventType::kNetworkHeartbeat:
      recover(AlertRecoveryCode::kEsp32Recovered,
              AlertSource::kFirmwareHealth, nowMillis,
              "ESP32 heartbeat recovered");
      esp32OfflineActive_ = false;
      break;
    case core::events::EventType::kSystemStateChanged:
    case core::events::EventType::kConfigChanged:
    case core::events::EventType::kSchedulerTaskFailed:
    case core::events::EventType::kTemperatureUpdated:
    case core::events::EventType::kWaterLevelUpdated:
    case core::events::EventType::kWaterLevelLow:
    case core::events::EventType::kWaterLevelHigh:
    case core::events::EventType::kWaterLevelSensorOffline:
    case core::events::EventType::kWaterLevelSensorRecovered:
    case core::events::EventType::kRelayOn:
    case core::events::EventType::kRelayOff:
    case core::events::EventType::kAtoStart:
    case core::events::EventType::kAtoStop:
    case core::events::EventType::kModeChanged:
    case core::events::EventType::kModeStarted:
    case core::events::EventType::kModeFinished:
    case core::events::EventType::kLightingProfileChanged:
    case core::events::EventType::kLightingStarted:
    case core::events::EventType::kLightingStopped:
    case core::events::EventType::kConfigSaved:
    case core::events::EventType::kConfigRestored:
    case core::events::EventType::kConfigReset:
    case core::events::EventType::kWifiReconnecting:
    case core::events::EventType::kNtpSynced:
    case core::events::EventType::kNtpSyncFailed:
    case core::events::EventType::kAlertRaised:
    case core::events::EventType::kAlertRecovered:
    case core::events::EventType::kAlertCooldownSuppressed:
    case core::events::EventType::kAlertHistoryRecorded:
      break;
  }
}

void AlertDetector::raise(AlertCode code, AlertSource source,
                          uint32_t nowMillis, const char* reason) {
  AlertRequest request = {};
  request.code = code;
  request.source = source;
  request.occurredAtMillis = nowMillis;
  request.reason = reason;
  manager_.raise(request);
}

void AlertDetector::recover(AlertRecoveryCode recoveryCode,
                            AlertSource source, uint32_t nowMillis,
                            const char* reason) {
  AlertRecoveryRequest request = {};
  request.recoveryCode = recoveryCode;
  request.source = source;
  request.occurredAtMillis = nowMillis;
  request.reason = reason;
  manager_.recover(request);
}

}  // namespace reeflow::alerts
