#include "app/core_app.h"

#include "core/state/system_state.h"
#include "modules/temperature/temperature_service.h"

namespace reeflow::app {
namespace {

constexpr const char* kTemperatureTaskName = "temperature";

}  // namespace

CoreApp::CoreApp(core::platform::CorePlatform& platform,
                 core::events::EventBus& eventBus,
                 config::ConfigManager& config,
                 modules::temperature::TemperatureSensor& temperatureSensor)
    : platform_(platform),
      eventBus_(eventBus),
      config_(config),
      logger_(platform_.logSink()),
      scheduler_(platform_.timeSource(), eventBus_, logger_),
      watchdog_(platform_.watchdogBackend(), platform_.timeSource(), logger_),
      temperatureService_(temperatureSensor, config_, platform_.timeSource(),
                          eventBus_) {}

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

  temperatureService_.resetMonitor();
  const uint8_t temperatureTaskId = scheduler_.registerTask(
      kTemperatureTaskName, config_.temperature().readIntervalMillis,
      modules::temperature::runTemperatureServiceTask,
      &temperatureService_);
  if (temperatureTaskId == core::scheduler::kInvalidTaskId) {
    logger_.error("core", "temperature task failed");
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
