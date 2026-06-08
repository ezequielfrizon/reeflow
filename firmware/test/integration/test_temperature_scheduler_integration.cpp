#include <assert.h>

#include "app/core_app.h"
#include "config/config_manager.h"
#include "core/events/event_bus.h"
#include "core/platform/core_platform.h"
#include "core/scheduler/task_scheduler.h"
#include "fakes/fake_log_sink.h"
#include "fakes/fake_relay_controller.h"
#include "fakes/fake_temperature_sensor.h"
#include "fakes/fake_time_source.h"
#include "fakes/fake_water_level_sensor.h"
#include "fakes/fake_watchdog_backend.h"
#include "modules/temperature/temperature_service.h"

namespace {

using reeflow::app::CoreApp;
using reeflow::config::ConfigManager;
using reeflow::core::events::Event;
using reeflow::core::events::EventBus;
using reeflow::core::events::EventType;
using reeflow::core::platform::CorePlatform;
using reeflow::core::scheduler::SchedulerRunResult;
using reeflow::core::scheduler::TaskScheduler;
using reeflow::modules::temperature::TemperatureService;
using reeflow::modules::temperature::runTemperatureServiceTask;
using reeflow::test::fakes::FakeLogSink;
using reeflow::test::fakes::FakeRelayController;
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
  FakeRelayController relayController;
  CoreApp app;

  TestCoreAppContext()
      : platform(timeSource, logSink, watchdogBackend),
        configManager(eventBus),
        app(platform, eventBus, configManager, temperatureSensor,
            waterLevelSensor, relayController) {}
};

void testRuntimeTemperatureTaskUsesDefaultInterval() {
  TestCoreAppContext context;
  context.temperatureSensor.setValidTemperature(26.5F);
  context.waterLevelSensor.setValidLevel(50);

  assert(context.app.setup());
  context.timeSource.advanceMillis(4999);
  SchedulerRunResult result = context.app.loopOnce();

  assert(result.executedCount == 2);
  assert(context.watchdogBackend.feedCalls() == 1);
  assert(context.temperatureSensor.readCount() == 0);

  context.timeSource.advanceMillis(1);
  result = context.app.loopOnce();

  assert(result.executedCount == 2);
  assert(result.failedCount == 0);
  assert(context.temperatureSensor.readCount() == 1);
  assert(context.waterLevelSensor.readCount() == 1);
}

void testTemperatureTaskUsesConfiguredInterval() {
  FakeTimeSource timeSource;
  EventBus eventBus;
  FakeLogSink logSink;
  reeflow::core::logging::Logger logger(logSink);
  ConfigManager configManager(eventBus);
  reeflow::config::TemperatureConfig config = configManager.temperature();
  config.readIntervalMillis = 2500;
  assert(configManager.updateTemperature(config));
  FakeTemperatureSensor sensor;
  sensor.setValidTemperature(26.5F);
  TemperatureService service(sensor, configManager, timeSource, eventBus);
  TaskScheduler scheduler(timeSource, eventBus, logger);
  scheduler.registerTask("temperature", configManager.temperature().readIntervalMillis,
                         runTemperatureServiceTask, &service);

  timeSource.advanceMillis(2499);
  SchedulerRunResult result = scheduler.runDueTasks();
  assert(result.executedCount == 0);
  assert(sensor.readCount() == 0);

  timeSource.advanceMillis(1);
  result = scheduler.runDueTasks();
  assert(result.executedCount == 1);
  assert(result.failedCount == 0);
  assert(sensor.readCount() == 1);
}

void testTemperatureTaskRunsOnMultipleIntervals() {
  FakeTimeSource timeSource;
  EventBus eventBus;
  FakeLogSink logSink;
  reeflow::core::logging::Logger logger(logSink);
  ConfigManager configManager(eventBus);
  FakeTemperatureSensor sensor;
  sensor.setValidTemperature(26.5F);
  TemperatureService service(sensor, configManager, timeSource, eventBus);
  TaskScheduler scheduler(timeSource, eventBus, logger);
  scheduler.registerTask("temperature", configManager.temperature().readIntervalMillis,
                         runTemperatureServiceTask, &service);

  timeSource.advanceMillis(5000);
  assert(scheduler.runDueTasks().executedCount == 1);
  timeSource.advanceMillis(4999);
  assert(scheduler.runDueTasks().executedCount == 0);
  timeSource.advanceMillis(1);
  assert(scheduler.runDueTasks().executedCount == 1);
  assert(sensor.readCount() == 2);
}

void testTemperatureTaskFailureIsReportedByScheduler() {
  FakeTimeSource timeSource;
  EventBus eventBus;
  EventRecorder recorder = {};
  eventBus.subscribe(EventType::kSchedulerTaskFailed, recordEvent, &recorder);
  FakeLogSink logSink;
  reeflow::core::logging::Logger logger(logSink);
  ConfigManager configManager(eventBus);
  FakeTemperatureSensor sensor;
  sensor.setReadError();
  TemperatureService service(sensor, configManager, timeSource, eventBus);
  TaskScheduler scheduler(timeSource, eventBus, logger);
  scheduler.registerTask("temperature", configManager.temperature().readIntervalMillis,
                         runTemperatureServiceTask, &service);

  timeSource.advanceMillis(5000);
  const SchedulerRunResult result = scheduler.runDueTasks();

  assert(result.executedCount == 1);
  assert(result.failedCount == 1);
  assert(recorder.count == 1);
  assert(recorder.events[0].type == EventType::kSchedulerTaskFailed);
}

void testConversionPendingDoesNotFailScheduler() {
  FakeTimeSource timeSource;
  EventBus eventBus;
  FakeLogSink logSink;
  reeflow::core::logging::Logger logger(logSink);
  ConfigManager configManager(eventBus);
  FakeTemperatureSensor sensor;
  sensor.setConversionPending();
  TemperatureService service(sensor, configManager, timeSource, eventBus);
  TaskScheduler scheduler(timeSource, eventBus, logger);
  scheduler.registerTask("temperature", configManager.temperature().readIntervalMillis,
                         runTemperatureServiceTask, &service);

  timeSource.advanceMillis(5000);
  const SchedulerRunResult result = scheduler.runDueTasks();

  assert(result.executedCount == 1);
  assert(result.failedCount == 0);
  assert(sensor.readCount() == 1);
}

}  // namespace

int main() {
  testRuntimeTemperatureTaskUsesDefaultInterval();
  testTemperatureTaskUsesConfiguredInterval();
  testTemperatureTaskRunsOnMultipleIntervals();
  testTemperatureTaskFailureIsReportedByScheduler();
  testConversionPendingDoesNotFailScheduler();
  return 0;
}
