#pragma once

#include <stdint.h>

#include "config/config_manager.h"
#include "core/events/event_bus.h"
#include "core/logging/logger.h"
#include "core/platform/core_platform.h"
#include "core/scheduler/task_scheduler.h"
#include "core/watchdog/watchdog_service.h"
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
          modules::water_level::WaterLevelSensor& waterLevelSensor);

  bool setup();
  core::scheduler::SchedulerRunResult loopOnce();

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
  bool initialized_ = false;
};

CoreApp& defaultCoreApp();
void setupCoreApp();
void loopCoreApp();

}  // namespace reeflow::app
