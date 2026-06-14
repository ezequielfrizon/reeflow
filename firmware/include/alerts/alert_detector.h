#pragma once

#include "alerts/alert_manager.h"
#include "core/events/event_bus.h"
#include "core/state/system_state.h"

namespace reeflow::alerts {

constexpr uint32_t kDefaultEsp32HeartbeatTimeoutMillis = 90000;

class AlertDetector final {
 public:
  explicit AlertDetector(
      AlertManager& manager,
      uint32_t esp32HeartbeatTimeoutMillis =
          kDefaultEsp32HeartbeatTimeoutMillis);

  void observeEvent(const core::events::Event& event,
                    const core::state::SystemState& state,
                    uint32_t nowMillis);
  void evaluateSnapshot(const core::state::SystemState& state,
                        uint32_t nowMillis);
  void reset();

 private:
  void evaluateTemperature(core::state::TemperatureStatus status,
                           uint32_t nowMillis);
  void evaluateAto(core::state::AtoStatus status, uint32_t nowMillis);
  void evaluateNetwork(const core::state::NetworkState& network,
                       uint32_t nowMillis);
  void evaluateSystemHealth(
      const core::state::SystemHealthState& systemHealth,
      uint32_t nowMillis);
  void evaluateEsp32Heartbeat(const core::state::NetworkState& network,
                              uint32_t nowMillis);
  void handleDirectEvent(core::events::EventType eventType,
                         uint32_t nowMillis);
  void raise(AlertCode code, AlertSource source, uint32_t nowMillis,
             const char* reason);
  void recover(AlertRecoveryCode recoveryCode, AlertSource source,
               uint32_t nowMillis, const char* reason);

  AlertManager& manager_;
  uint32_t esp32HeartbeatTimeoutMillis_;
  bool hasTemperatureStatus_ = false;
  bool hasAtoStatus_ = false;
  bool hasWifiState_ = false;
  bool hasMqttState_ = false;
  bool hasSystemHealth_ = false;
  bool temperatureConditionActive_ = false;
  bool temperatureSensorOfflineActive_ = false;
  bool atoTimeoutActive_ = false;
  bool atoSensorOfflineActive_ = false;
  bool wifiOfflineActive_ = false;
  bool mqttOfflineActive_ = false;
  bool esp32OfflineActive_ = false;
  bool unexpectedRebootActive_ = false;
  core::state::TemperatureStatus lastTemperatureStatus_ =
      core::state::TemperatureStatus::kNormal;
  core::state::AtoStatus lastAtoStatus_ = core::state::AtoStatus::kNormal;
  bool lastWifiConnected_ = false;
  bool lastMqttConnected_ = false;
  bool lastWatchdogTriggered_ = false;
};

}  // namespace reeflow::alerts
