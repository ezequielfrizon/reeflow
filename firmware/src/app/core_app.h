#pragma once

#include <stdint.h>

#include "config/config_manager.h"
#include "core/events/event_bus.h"
#include "core/logging/logger.h"
#include "core/platform/core_platform.h"
#include "core/scheduler/task_scheduler.h"
#include "core/watchdog/watchdog_service.h"
#include "modules/ato/ato_service.h"
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

namespace reeflow::app {

constexpr uint32_t kCoreWatchdogTimeoutMillis = 5000;
constexpr uint32_t kCoreWatchdogFeedIntervalMillis = 1000;

class CoreApp {
 public:
  CoreApp(core::platform::CorePlatform& platform,
          core::events::EventBus& eventBus, config::ConfigManager& config,
          modules::temperature::TemperatureSensor& temperatureSensor,
          modules::water_level::WaterLevelSensor& waterLevelSensor,
          modules::relays::RelayController& relayController,
          modules::modes::ModeStore& modeStore);

  bool setup();
  core::scheduler::SchedulerRunResult loopOnce();
  modules::relays::RelayCommandResult setLocalRelay(
      modules::relays::RelayId relay,
      modules::relays::RelayDesiredState desiredState);
  modules::modes::ModeServiceResult requestMode(
      modules::modes::OperationalMode mode);

  config::ConfigManager& configManager();
  core::events::EventBus& eventBus();
  core::scheduler::TaskScheduler& scheduler();
  core::watchdog::WatchdogService& watchdog();

 private:
  core::platform::CorePlatform& platform_;
  core::events::EventBus& eventBus_;
  config::ConfigManager& config_;
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
  bool initialized_ = false;
};

CoreApp& defaultCoreApp();
void setupCoreApp();
void loopCoreApp();

}  // namespace reeflow::app
