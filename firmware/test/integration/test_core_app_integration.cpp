#include <assert.h>

#include "app/core_app.h"
#include "config/config_manager.h"
#include "core/events/event_bus.h"
#include "core/platform/core_platform.h"
#include "core/state/system_state.h"
#include "fakes/fake_log_sink.h"
#include "fakes/fake_time_source.h"
#include "fakes/fake_temperature_sensor.h"
#include "fakes/fake_water_level_sensor.h"
#include "fakes/fake_watchdog_backend.h"

namespace {

using reeflow::app::CoreApp;
using reeflow::config::ConfigManager;
using reeflow::core::events::ConfigDomain;
using reeflow::core::events::Event;
using reeflow::core::events::EventBus;
using reeflow::core::events::EventType;
using reeflow::core::events::StateArea;
using reeflow::core::platform::CorePlatform;
using reeflow::core::state::TemperatureStatus;
using reeflow::test::fakes::FakeLogSink;
using reeflow::test::fakes::FakeTimeSource;
using reeflow::test::fakes::FakeTemperatureSensor;
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

void testSetupInitializesCoreInOrder() {
  TestCoreAppContext context;

  assert(context.app.setup());

  assert(context.watchdogBackend.configureCalls() == 1);
  assert(context.watchdogBackend.configuredTimeoutMillis() ==
         reeflow::app::kCoreWatchdogTimeoutMillis);
  assert(context.app.watchdog().initialized());
  assert(reeflow::core::state::currentSystemState()
             .systemHealth.watchdogTriggered == false);
  assert(context.configManager.currentConfig()
             .temperature.targetTemperature == 26.5F);
  assert(context.logSink.messages().size() == 2);
  assert(context.logSink.messages()[0] ==
         "[INFO] watchdog: initialized\n");
  assert(context.logSink.messages()[1] == "[INFO] core: initialized\n");
}

void testLoopFeedsWatchdogThroughScheduler() {
  TestCoreAppContext context;

  assert(context.app.setup());
  context.timeSource.advanceMillis(999);
  reeflow::core::scheduler::SchedulerRunResult result =
      context.app.loopOnce();

  assert(result.executedCount == 0);
  assert(context.watchdogBackend.feedCalls() == 0);

  context.timeSource.advanceMillis(1);
  result = context.app.loopOnce();

  assert(result.executedCount == 1);
  assert(result.failedCount == 0);
  assert(context.watchdogBackend.feedCalls() == 1);
}

void testStateAndConfigEventsFlowDuringIntegration() {
  TestCoreAppContext context;
  EventRecorder stateRecorder = {};
  EventRecorder configRecorder = {};

  assert(context.app.setup());
  context.app.eventBus().subscribe(EventType::kSystemStateChanged,
                                   recordEvent, &stateRecorder);
  context.app.eventBus().subscribe(EventType::kConfigChanged, recordEvent,
                                   &configRecorder);

  reeflow::core::state::TemperatureState temperature =
      reeflow::core::state::currentSystemState().temperature;
  temperature.currentTemperature = 26.5F;
  temperature.status = TemperatureStatus::kNormal;
  temperature.lastUpdate = 10;
  reeflow::core::state::updateTemperatureState(temperature);

  reeflow::config::TemperatureConfig temperatureConfig =
      context.app.configManager().temperature();
  temperatureConfig.targetTemperature = 26.0F;
  assert(context.app.configManager().updateTemperature(temperatureConfig));

  assert(stateRecorder.count == 1);
  assert(stateRecorder.events[0].type == EventType::kSystemStateChanged);
  assert(stateRecorder.events[0].stateArea == StateArea::kTemperature);
  assert(configRecorder.count == 1);
  assert(configRecorder.events[0].type == EventType::kConfigChanged);
  assert(configRecorder.events[0].configDomain ==
         ConfigDomain::kTemperature);
}

}  // namespace

int main() {
  testSetupInitializesCoreInOrder();
  testLoopFeedsWatchdogThroughScheduler();
  testStateAndConfigEventsFlowDuringIntegration();
  return 0;
}
