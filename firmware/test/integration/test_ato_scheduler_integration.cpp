#include <assert.h>

#include "app/core_app.h"
#include "config/config_manager.h"
#include "core/events/event_bus.h"
#include "core/platform/core_platform.h"
#include "core/scheduler/task_scheduler.h"
#include "core/state/system_state.h"
#include "fakes/fake_log_sink.h"
#include "fakes/fake_lighting_pwm_controller.h"
#include "fakes/fake_mode_store.h"
#include "fakes/fake_relay_controller.h"
#include "fakes/fake_temperature_sensor.h"
#include "fakes/fake_time_source.h"
#include "fakes/fake_water_level_sensor.h"
#include "fakes/fake_watchdog_backend.h"
#include "modules/ato/ato_config.h"

namespace {

using reeflow::app::CoreApp;
using reeflow::config::AtoConfig;
using reeflow::config::ConfigManager;
using reeflow::core::events::Event;
using reeflow::core::events::EventBus;
using reeflow::core::events::EventType;
using reeflow::core::platform::CorePlatform;
using reeflow::core::scheduler::SchedulerRunResult;
using reeflow::core::state::AtoStatus;
using reeflow::core::state::RelaySource;
using reeflow::core::state::WaterLevelStatus;
using reeflow::modules::relays::RelayCommandResult;
using reeflow::modules::relays::RelayDesiredState;
using reeflow::modules::relays::RelayId;
using reeflow::test::fakes::FakeLogSink;
using reeflow::test::fakes::FakeLightingPwmController;
using reeflow::test::fakes::FakeModeStore;
using reeflow::test::fakes::FakeRelayController;
using reeflow::test::fakes::FakeTemperatureSensor;
using reeflow::test::fakes::FakeTimeSource;
using reeflow::test::fakes::FakeWaterLevelSensor;
using reeflow::test::fakes::FakeWatchdogBackend;

struct EventRecorder {
  Event events[16];
  uint8_t count;
};

bool recordEvent(const Event& event, void* context) {
  EventRecorder* recorder = static_cast<EventRecorder*>(context);
  recorder->events[recorder->count++] = event;
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
  FakeLightingPwmController lightingController;
  FakeModeStore modeStore;
  CoreApp app;

  TestCoreAppContext()
      : platform(timeSource, logSink, watchdogBackend),
        configManager(eventBus),
        app(platform, eventBus, configManager, temperatureSensor,
            waterLevelSensor, relayController, lightingController, modeStore) {}
};

void setWaterLevel(uint16_t level, WaterLevelStatus status) {
  reeflow::core::state::WaterLevelState waterLevel =
      reeflow::core::state::currentSystemState().waterLevel;
  waterLevel.currentLevel = level;
  waterLevel.minimumLevel = 20;
  waterLevel.maximumLevel = 80;
  waterLevel.status = status;
  waterLevel.lastUpdate = 100;
  reeflow::core::state::updateWaterLevelState(waterLevel);
}

void enableAto(ConfigManager& configManager) {
  AtoConfig config = configManager.ato();
  config.enabled = true;
  config.minimumLevel = 20;
  config.maximumLevel = 80;
  config.timeoutMillis = 60000;
  config.cooldownMillis = 300000;
  assert(configManager.updateAto(config));
}

void testAtoTaskUsesDefaultIntervalAndDoesNotRunEarly() {
  TestCoreAppContext context;
  assert(context.app.setup());

  context.timeSource.advanceMillis(999);
  SchedulerRunResult result = context.app.loopOnce();

  assert(result.executedCount == 0);
  assert(reeflow::core::state::currentSystemState().ato.status ==
         AtoStatus::kDisabled);

  context.timeSource.advanceMillis(1);
  result = context.app.loopOnce();

  assert(result.executedCount == 4);
  assert(result.failedCount == 0);
  assert(context.watchdogBackend.feedCalls() == 1);
  assert(reeflow::core::state::currentSystemState().ato.status ==
         AtoStatus::kDisabled);
  assert(!reeflow::core::state::currentSystemState().relays.atoPump.enabled);
}

void testAtoTaskRunsOnMultipleIntervals() {
  TestCoreAppContext context;
  assert(context.app.setup());

  context.timeSource.advanceMillis(1000);
  assert(context.app.loopOnce().executedCount == 4);

  context.timeSource.advanceMillis(999);
  assert(context.app.loopOnce().executedCount == 0);

  context.timeSource.advanceMillis(1);
  assert(context.app.loopOnce().executedCount == 4);
  assert(context.watchdogBackend.feedCalls() == 2);
}

void testEnabledAtoStartsRefillFromSystemStateFixture() {
  TestCoreAppContext context;
  EventRecorder atoStartRecorder = {};
  context.eventBus.subscribe(EventType::kAtoStart, recordEvent,
                             &atoStartRecorder);
  assert(context.app.setup());
  enableAto(context.configManager);
  setWaterLevel(20, WaterLevelStatus::kNormal);

  context.timeSource.advanceMillis(1000);
  const SchedulerRunResult result = context.app.loopOnce();

  assert(result.executedCount == 4);
  assert(result.failedCount == 0);
  assert(context.relayController.state(RelayId::kAtoPump) ==
         RelayDesiredState::kOn);
  assert(reeflow::core::state::currentSystemState().ato.status ==
         AtoStatus::kRefilling);
  assert(reeflow::core::state::currentSystemState().ato.pumpRunning);
  assert(reeflow::core::state::currentSystemState().relays.atoPump.enabled);
  assert(reeflow::core::state::currentSystemState().relays.atoPump.source ==
         RelaySource::kAutomation);
  assert(atoStartRecorder.count == 1);
}

void testEnabledAtoStopsRefillAtMaximumLevel() {
  TestCoreAppContext context;
  EventRecorder atoStopRecorder = {};
  context.eventBus.subscribe(EventType::kAtoStop, recordEvent,
                             &atoStopRecorder);
  assert(context.app.setup());
  enableAto(context.configManager);
  setWaterLevel(20, WaterLevelStatus::kNormal);

  context.timeSource.advanceMillis(1000);
  assert(context.app.loopOnce().failedCount == 0);

  setWaterLevel(80, WaterLevelStatus::kNormal);
  context.timeSource.advanceMillis(1000);
  const SchedulerRunResult result = context.app.loopOnce();

  assert(result.failedCount == 0);
  assert(context.relayController.state(RelayId::kAtoPump) ==
         RelayDesiredState::kOff);
  assert(reeflow::core::state::currentSystemState().ato.status ==
         AtoStatus::kNormal);
  assert(!reeflow::core::state::currentSystemState().ato.pumpRunning);
  assert(!reeflow::core::state::currentSystemState().relays.atoPump.enabled);
  assert(atoStopRecorder.count == 1);
}

void testTimeoutIsAppliedBySchedulerWithoutRealDelay() {
  TestCoreAppContext context;
  EventRecorder atoTimeoutRecorder = {};
  context.eventBus.subscribe(EventType::kAtoTimeout, recordEvent,
                             &atoTimeoutRecorder);
  context.waterLevelSensor.setValidLevel(20);
  assert(context.app.setup());
  enableAto(context.configManager);
  setWaterLevel(20, WaterLevelStatus::kNormal);

  context.timeSource.advanceMillis(1000);
  assert(context.app.loopOnce().failedCount == 0);

  context.timeSource.advanceMillis(60000);
  const SchedulerRunResult result = context.app.loopOnce();

  assert(result.failedCount == 0);
  assert(reeflow::core::state::currentSystemState().ato.status ==
         AtoStatus::kTimeout);
  assert(!reeflow::core::state::currentSystemState().ato.pumpRunning);
  assert(reeflow::core::state::currentSystemState().ato.timeoutCounter == 1);
  assert(atoTimeoutRecorder.count == 1);
}

void testCooldownPreventsImmediateRestartAfterNormalStop() {
  TestCoreAppContext context;
  assert(context.app.setup());
  enableAto(context.configManager);
  setWaterLevel(20, WaterLevelStatus::kNormal);

  context.timeSource.advanceMillis(1000);
  assert(context.app.loopOnce().failedCount == 0);

  setWaterLevel(80, WaterLevelStatus::kNormal);
  context.timeSource.advanceMillis(1000);
  assert(context.app.loopOnce().failedCount == 0);

  setWaterLevel(20, WaterLevelStatus::kNormal);
  context.timeSource.advanceMillis(1000);
  const SchedulerRunResult result = context.app.loopOnce();

  assert(result.failedCount == 0);
  assert(!reeflow::core::state::currentSystemState().ato.pumpRunning);
  assert(!reeflow::core::state::currentSystemState().relays.atoPump.enabled);
  assert(context.relayController.recordedCallCount() == 3);
}

void testRelayFailureIsReportedAsSchedulerFailure() {
  TestCoreAppContext context;
  EventRecorder schedulerFailureRecorder = {};
  context.eventBus.subscribe(EventType::kSchedulerTaskFailed, recordEvent,
                             &schedulerFailureRecorder);
  assert(context.app.setup());
  enableAto(context.configManager);
  setWaterLevel(20, WaterLevelStatus::kNormal);
  context.relayController.failRelay(RelayId::kAtoPump);

  context.timeSource.advanceMillis(1000);
  const SchedulerRunResult result = context.app.loopOnce();

  assert(result.executedCount == 4);
  assert(result.failedCount == 1);
  assert(!reeflow::core::state::currentSystemState().ato.pumpRunning);
  assert(schedulerFailureRecorder.count == 1);
}

}  // namespace

int main() {
  testAtoTaskUsesDefaultIntervalAndDoesNotRunEarly();
  testAtoTaskRunsOnMultipleIntervals();
  testEnabledAtoStartsRefillFromSystemStateFixture();
  testEnabledAtoStopsRefillAtMaximumLevel();
  testTimeoutIsAppliedBySchedulerWithoutRealDelay();
  testCooldownPreventsImmediateRestartAfterNormalStop();
  testRelayFailureIsReportedAsSchedulerFailure();
  return 0;
}
