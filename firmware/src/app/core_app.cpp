#include "app/core_app.h"

#include "core/state/system_state.h"
#include "modules/ato/ato_config.h"
#include "modules/ato/ato_service.h"
#include "modules/temperature/temperature_service.h"
#include "modules/water_level/water_level_config.h"
#include "modules/water_level/water_level_service.h"

namespace reeflow::app {
namespace {

constexpr const char* kTemperatureTaskName = "temperature";
constexpr const char* kWaterLevelTaskName = "water-level";
constexpr const char* kAtoTaskName = "ato";

}  // namespace

CoreApp::CoreApp(core::platform::CorePlatform& platform,
                 core::events::EventBus& eventBus,
                 config::ConfigManager& config,
                 modules::temperature::TemperatureSensor& temperatureSensor,
                 modules::water_level::WaterLevelSensor& waterLevelSensor,
                 modules::relays::RelayController& relayController)
    : platform_(platform),
      eventBus_(eventBus),
      config_(config),
      logger_(platform_.logSink()),
      scheduler_(platform_.timeSource(), eventBus_, logger_),
      watchdog_(platform_.watchdogBackend(), platform_.timeSource(), logger_),
      temperatureService_(temperatureSensor, config_, platform_.timeSource(),
                          eventBus_),
      waterLevelService_(waterLevelSensor, config_, platform_.timeSource(),
                         eventBus_),
      relayService_(relayController, platform_.timeSource(), eventBus_),
      atoService_(relayService_, config_, platform_.timeSource(), eventBus_) {}

bool CoreApp::setup() {
  core::state::setSystemStateEventBus(eventBus_);
  core::state::resetSystemState();
  config_.loadDefaults();
  scheduler_.clear();

  if (!watchdog_.initialize(kCoreWatchdogTimeoutMillis)) {
    logger_.error("core", "watchdog init failed");
    return false;
  }

  const uint8_t watchdogTaskId =
      scheduler_.registerTask("watchdog", kCoreWatchdogFeedIntervalMillis,
                              core::watchdog::feedWatchdogTask, &watchdog_);
  if (watchdogTaskId == core::scheduler::kInvalidTaskId) {
    logger_.error("core", "watchdog task failed");
    return false;
  }

  const modules::relays::RelayCommandResult relaySafeStateResult =
      relayService_.allOff(core::state::RelaySource::kFailsafe);
  if (relaySafeStateResult !=
      modules::relays::RelayCommandResult::kSuccess) {
    logger_.error("core", "relay safe state failed");
    return false;
  }

  temperatureService_.resetMonitor();
  const uint8_t temperatureTaskId = scheduler_.registerTask(
      kTemperatureTaskName, config_.temperature().readIntervalMillis,
      modules::temperature::runTemperatureServiceTask,
      &temperatureService_);
  if (temperatureTaskId == core::scheduler::kInvalidTaskId) {
    logger_.error("core", "temperature task failed");
    return false;
  }

  waterLevelService_.resetMonitor();
  const modules::water_level::WaterLevelModuleConfig waterLevelConfig =
      modules::water_level::makeDefaultWaterLevelModuleConfig();
  const uint8_t waterLevelTaskId = scheduler_.registerTask(
      kWaterLevelTaskName, waterLevelConfig.readIntervalMillis,
      modules::water_level::runWaterLevelServiceTask,
      &waterLevelService_);
  if (waterLevelTaskId == core::scheduler::kInvalidTaskId) {
    logger_.error("core", "water level task failed");
    return false;
  }

  const modules::ato::AtoModuleConfig atoConfig =
      modules::ato::makeDefaultAtoModuleConfig();
  const uint8_t atoTaskId = scheduler_.registerTask(
      kAtoTaskName, atoConfig.evaluationIntervalMillis,
      modules::ato::runAtoServiceTask, &atoService_);
  if (atoTaskId == core::scheduler::kInvalidTaskId) {
    logger_.error("core", "ato task failed");
    return false;
  }

  initialized_ = true;
  logger_.info("core", "initialized");
  return true;
}

core::scheduler::SchedulerRunResult CoreApp::loopOnce() {
  core::scheduler::SchedulerRunResult result = {};
  if (!initialized_) {
    return result;
  }

  return scheduler_.runDueTasks();
}

modules::relays::RelayCommandResult CoreApp::setLocalRelay(
    modules::relays::RelayId relay,
    modules::relays::RelayDesiredState desiredState) {
  if (!initialized_) {
    return modules::relays::RelayCommandResult::kControllerFailure;
  }

  return relayService_.setLocalRelay(relay, desiredState);
}

config::ConfigManager& CoreApp::configManager() {
  return config_;
}

core::events::EventBus& CoreApp::eventBus() {
  return eventBus_;
}

core::scheduler::TaskScheduler& CoreApp::scheduler() {
  return scheduler_;
}

core::watchdog::WatchdogService& CoreApp::watchdog() {
  return watchdog_;
}

}  // namespace reeflow::app
