#pragma once

#include <stdint.h>

#include "alerts/alert_detector.h"
#include "alerts/alert_history.h"
#include "alerts/alert_manager.h"
#include "config/config_manager.h"
#include "core/events/event_bus.h"
#include "core/logging/logger.h"
#include "core/platform/core_platform.h"
#include "core/scheduler/task_scheduler.h"
#include "core/watchdog/watchdog_service.h"
#include "modules/ato/ato_service.h"
#include "modules/lighting/lighting_profile.h"
#include "modules/lighting/lighting_pwm_controller.h"
#include "modules/lighting/lighting_service.h"
#include "modules/lighting/lighting_types.h"
#include "modules/modes/mode_automation_gate.h"
#include "modules/modes/mode_effects.h"
#include "modules/modes/mode_service.h"
#include "modules/modes/mode_store.h"
#include "modules/relays/relay_controller.h"
#include "modules/relays/relay_service.h"
#include "modules/relays/relay_types.h"
#include "modules/temperature/temperature_sensor.h"
#include "modules/temperature/temperature_service.h"
#include "modules/water_level/water_level_sensor.h"
#include "modules/water_level/water_level_service.h"
#include "mqtt/mqtt_service.h"
#include "network/network_heartbeat.h"
#include "network/network_status_service.h"
#include "network/ntp_service.h"
#include "network/wifi_service.h"
#include "storage/storage_service.h"

namespace reeflow::app {

constexpr uint32_t kCoreWatchdogTimeoutMillis = 5000;
constexpr uint32_t kCoreWatchdogFeedIntervalMillis = 1000;
constexpr uint32_t kStorageFlushIntervalMillis = 5000;
constexpr uint32_t kAlertsEvaluationIntervalMillis = 5000;

class CoreApp {
 public:
  CoreApp(core::platform::CorePlatform& platform,
          core::events::EventBus& eventBus, config::ConfigManager& config,
          modules::temperature::TemperatureSensor& temperatureSensor,
          modules::water_level::WaterLevelSensor& waterLevelSensor,
          modules::relays::RelayController& relayController,
          modules::lighting::LightingPwmController& lightingController,
          modules::modes::ModeStore& modeStore,
          storage::StorageService* storageService = nullptr,
          network::WifiService* wifiService = nullptr,
          network::NtpService* ntpService = nullptr,
          network::NetworkStatusService* networkStatusService = nullptr,
          network::NetworkHeartbeat* networkHeartbeat = nullptr,
          mqtt::MqttService* mqttService = nullptr);

  bool setup();
  core::scheduler::SchedulerRunResult loopOnce();
  modules::relays::RelayCommandResult setLocalRelay(
      modules::relays::RelayId relay,
      modules::relays::RelayDesiredState desiredState);
  modules::modes::ModeServiceResult requestMode(
      modules::modes::OperationalMode mode);
  modules::lighting::LightingServiceResult requestLightingManualMode();
  modules::lighting::LightingServiceResult requestLightingAutomaticMode();
  modules::lighting::LightingServiceResult requestLightingAcclimationMode();
  modules::lighting::LightingServiceResult setLightingProfile(
      const modules::lighting::LightingProfile& profile);
  modules::lighting::LightingServiceResult setLightingManualDuty(
      modules::lighting::LightingChannel channel, uint16_t duty);

  config::ConfigManager& configManager();
  core::events::EventBus& eventBus();
  core::scheduler::TaskScheduler& scheduler();
  core::watchdog::WatchdogService& watchdog();

 private:
  static bool handleConfigChanged(const core::events::Event& event,
                                  void* context);
  static bool handleAlertStateChanged(const core::events::Event& event,
                                      void* context);
  static bool handleNetworkEvent(const core::events::Event& event,
                                 void* context);
  static bool runWifiTask(void* context);
  static bool runNtpTask(void* context);
  static bool runNetworkStatusTask(void* context);
  static bool runNetworkHeartbeatTask(void* context);
  static bool runMqttTask(void* context);
  static bool runAlertDetectorTask(void* context);
  static mqtt::MqttCommandResult handleMqttModeCommand(
      modules::modes::OperationalMode mode, void* context);
  static mqtt::MqttCommandResult handleMqttRelayCommand(
      modules::relays::RelayId relay,
      modules::relays::RelayDesiredState desiredState, void* context);
  static mqtt::MqttCommandResult handleMqttLightingCommand(
      modules::lighting::LightingChannel channel, uint16_t duty,
      void* context);
  static mqtt::MqttCommandResult handleMqttConfigCommand(
      const mqtt::MqttCommandConfigRequest& request, void* context);
  bool registerStorageFlushTask();
  bool registerNetworkTasks();
  bool registerAlertDetector();
  void configureMqttCommandHandler();
  void restorePersistedConfiguration();
  void trackConfigChange(core::events::ConfigDomain domain);
  void syncLightingPersistedState();
  void logNetworkEvent(const network::NetworkEvent& event);

  core::platform::CorePlatform& platform_;
  core::events::EventBus& eventBus_;
  config::ConfigManager& config_;
  storage::StorageService* storageService_;
  network::WifiService* wifiService_;
  network::NtpService* ntpService_;
  network::NetworkStatusService* networkStatusService_;
  network::NetworkHeartbeat* networkHeartbeat_;
  mqtt::MqttService* mqttService_;
  core::logging::Logger logger_;
  core::scheduler::TaskScheduler scheduler_;
  core::watchdog::WatchdogService watchdog_;
  modules::temperature::TemperatureService temperatureService_;
  modules::water_level::WaterLevelService waterLevelService_;
  modules::relays::RelayService relayService_;
  modules::modes::LocalModeAutomationGate modeAutomationGate_;
  modules::modes::RelayModeEffects modeEffects_;
  modules::modes::ModeService modeService_;
  modules::ato::AtoService atoService_;
  storage::LightingPersistedState lightingPersistedState_;
  modules::lighting::LightingProfile lightingProfile_;
  modules::lighting::LightingModuleConfig lightingModuleConfig_;
  modules::lighting::LightingService lightingService_;
  alerts::AlertHistoryBuffer alertHistory_;
  alerts::AlertManager alertManager_;
  alerts::AlertDetector alertDetector_;
  bool initialized_ = false;
};

CoreApp& defaultCoreApp();
void setupCoreApp();
void loopCoreApp();

}  // namespace reeflow::app
