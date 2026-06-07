#include <assert.h>

#include "app/core_app.h"
#include "config/config_manager.h"
#include "core/events/event_bus.h"
#include "core/platform/core_platform.h"
#include "core/scheduler/task_scheduler.h"
#include "core/state/system_state.h"
#include "fakes/fake_log_sink.h"
#include "fakes/fake_temperature_sensor.h"
#include "fakes/fake_time_source.h"
#include "fakes/fake_water_level_sensor.h"
#include "fakes/fake_watchdog_backend.h"
#include "modules/water_level/water_level_config.h"
#include "modules/water_level/water_level_service.h"

namespace {

using reeflow::app::CoreApp;
using reeflow::config::ConfigManager;
using reeflow::core::events::Event;
using reeflow::core::events::EventBus;
using reeflow::core::events::EventType;
using reeflow::core::platform::CorePlatform;
using reeflow::core::scheduler::SchedulerRunResult;
using reeflow::core::scheduler::TaskScheduler;
using reeflow::core::state::WaterLevelStatus;
using reeflow::modules::water_level::WaterLevelService;
using reeflow::modules::water_level::makeDefaultWaterLevelModuleConfig;
using reeflow::modules::water_level::runWaterLevelServiceTask;
using reeflow::test::fakes::FakeLogSink;
using reeflow::test::fakes::FakeTemperatureSensor;
using reeflow::test::fakes::FakeTimeSource;
using reeflow::test::fakes::FakeWaterLevelSensor;
using reeflow::test::fakes::FakeWatchdogBackend;

struct EventRecorder {
  Event events[8];
  uint8_t count;
};

bool recordEvent(const Event& event, void* context) {
  EventRecorder* recorder = static_cast<EventRecorder*>(context);
  recorder->events[recorder->count] = event;
  recorder->count += 1;
  return true;
}

struct TestCoreAppContext {
  FakeTimeSource timeSource;
  FakeLogSink logSink;
  FakeWatchdogBackend watchdogBackend;
  CorePlatform platform;
  EventBus eventBus;
  ConfigManager configManager;
  FakeTemperatureSensor temperatureSensor;
  FakeWaterLevelSensor waterLevelSensor;
  CoreApp app;

  TestCoreAppContext()
      : platform(timeSource, logSink, watchdogBackend),
        configManager(eventBus),
        app(platform, eventBus, configManager, temperatureSensor,
            waterLevelSensor) {}
};

void testRuntimeWaterLevelTaskUsesDefaultInterval() {
  TestCoreAppContext context;
  context.temperatureSensor.setValidTemperature(26.5F);
  context.waterLevelSensor.setValidLevel(50);

  assert(context.app.setup());
  context.timeSource.advanceMillis(4999);
  SchedulerRunResult result = context.app.loopOnce();

  assert(result.executedCount == 1);
  assert(context.watchdogBackend.feedCalls() == 1);
  assert(context.waterLevelSensor.readCount() == 0);

  context.timeSource.advanceMillis(1);
  result = context.app.loopOnce();

  assert(result.executedCount == 2);
  assert(result.failedCount == 0);
  assert(context.waterLevelSensor.readCount() == 1);
  assert(reeflow::core::state::currentSystemState().waterLevel.currentLevel ==
         50);
}

void testWaterLevelTaskUsesLocalModuleContractInterval() {
  FakeTimeSource timeSource;
  EventBus eventBus;
  FakeLogSink logSink;
  reeflow::core::logging::Logger logger(logSink);
  ConfigManager configManager(eventBus);
  FakeWaterLevelSensor sensor;
  sensor.setValidLevel(55);
  WaterLevelService service(sensor, configManager, timeSource, eventBus);
  TaskScheduler scheduler(timeSource, eventBus, logger);
  const auto moduleConfig = makeDefaultWaterLevelModuleConfig();
  scheduler.registerTask("water-level", moduleConfig.readIntervalMillis,
                         runWaterLevelServiceTask, &service);

  timeSource.advanceMillis(4999);
  SchedulerRunResult result = scheduler.runDueTasks();
  assert(result.executedCount == 0);
  assert(sensor.readCount() == 0);

  timeSource.advanceMillis(1);
  result = scheduler.runDueTasks();
  assert(result.executedCount == 1);
  assert(result.failedCount == 0);
  assert(sensor.readCount() == 1);
}

void testWaterLevelTaskRunsOnMultipleIntervals() {
  FakeTimeSource timeSource;
  EventBus eventBus;
  FakeLogSink logSink;
  reeflow::core::logging::Logger logger(logSink);
  ConfigManager configManager(eventBus);
  FakeWaterLevelSensor sensor;
  sensor.setValidLevel(55);
  WaterLevelService service(sensor, configManager, timeSource, eventBus);
  TaskScheduler scheduler(timeSource, eventBus, logger);
  const auto moduleConfig = makeDefaultWaterLevelModuleConfig();
  scheduler.registerTask("water-level", moduleConfig.readIntervalMillis,
                         runWaterLevelServiceTask, &service);

  timeSource.advanceMillis(5000);
  assert(scheduler.runDueTasks().executedCount == 1);
  timeSource.advanceMillis(4999);
  assert(scheduler.runDueTasks().executedCount == 0);
  timeSource.advanceMillis(1);
  assert(scheduler.runDueTasks().executedCount == 1);
  assert(sensor.readCount() == 2);
}

void testWaterLevelTaskFailureIsReportedByScheduler() {
  FakeTimeSource timeSource;
  EventBus eventBus;
  EventRecorder recorder = {};
  eventBus.subscribe(EventType::kSchedulerTaskFailed, recordEvent, &recorder);
  FakeLogSink logSink;
  reeflow::core::logging::Logger logger(logSink);
  ConfigManager configManager(eventBus);
  FakeWaterLevelSensor sensor;
  sensor.setReadError();
  WaterLevelService service(sensor, configManager, timeSource, eventBus);
  TaskScheduler scheduler(timeSource, eventBus, logger);
  const auto moduleConfig = makeDefaultWaterLevelModuleConfig();
  scheduler.registerTask("water-level", moduleConfig.readIntervalMillis,
                         runWaterLevelServiceTask, &service);

  timeSource.advanceMillis(5000);
  const SchedulerRunResult result = scheduler.runDueTasks();

  assert(result.executedCount == 1);
  assert(result.failedCount == 1);
  assert(recorder.count == 1);
  assert(recorder.events[0].type == EventType::kSchedulerTaskFailed);
}

void testSensorFailuresDoNotBlockSchedulerUntilOffline() {
  FakeTimeSource timeSource;
  EventBus eventBus;
  FakeLogSink logSink;
  reeflow::core::logging::Logger logger(logSink);
  ConfigManager configManager(eventBus);
  FakeWaterLevelSensor sensor;
  sensor.setReadError();
  WaterLevelService service(sensor, configManager, timeSource, eventBus);
  TaskScheduler scheduler(timeSource, eventBus, logger);
  const auto moduleConfig = makeDefaultWaterLevelModuleConfig();
  scheduler.registerTask("water-level", moduleConfig.readIntervalMillis,
                         runWaterLevelServiceTask, &service);

  timeSource.advanceMillis(30001);
  const SchedulerRunResult result = scheduler.runDueTasks();

  assert(result.executedCount == 1);
  assert(result.failedCount == 0);
  assert(reeflow::core::state::currentSystemState().waterLevel.status ==
         WaterLevelStatus::kSensorOffline);
}

}  // namespace

int main() {
  testRuntimeWaterLevelTaskUsesDefaultInterval();
  testWaterLevelTaskUsesLocalModuleContractInterval();
  testWaterLevelTaskRunsOnMultipleIntervals();
  testWaterLevelTaskFailureIsReportedByScheduler();
  testSensorFailuresDoNotBlockSchedulerUntilOffline();
  return 0;
}
