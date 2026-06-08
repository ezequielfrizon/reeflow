#include <assert.h>

#include "app/core_app.h"
#include "config/config_manager.h"
#include "core/events/event_bus.h"
#include "core/platform/core_platform.h"
#include "core/state/system_state.h"
#include "fakes/fake_log_sink.h"
#include "fakes/fake_lighting_pwm_controller.h"
#include "fakes/fake_mode_store.h"
#include "fakes/fake_relay_controller.h"
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
using reeflow::core::state::AtoStatus;
using reeflow::core::state::RelaySource;
using reeflow::core::state::TemperatureStatus;
using reeflow::core::state::WaterLevelStatus;
using reeflow::modules::modes::ModeServiceResult;
using reeflow::modules::relays::RelayCommandResult;
using reeflow::modules::relays::RelayDesiredState;
using reeflow::modules::relays::RelayId;
using reeflow::modules::modes::FEEDING;
using reeflow::modules::modes::MAINTENANCE;
using reeflow::modules::modes::NORMAL;
using reeflow::modules::modes::TPA;
using reeflow::test::fakes::FakeLogSink;
using reeflow::test::fakes::FakeLightingPwmController;
using reeflow::test::fakes::FakeModeStore;
using reeflow::test::fakes::FakeRelayController;
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

void testSetupAppliesRelaySafeStateWithoutRelayEvents() {
  TestCoreAppContext context;
  EventRecorder relayOnRecorder = {};
  EventRecorder relayOffRecorder = {};
  context.eventBus.subscribe(EventType::kRelayOn, recordEvent,
                             &relayOnRecorder);
  context.eventBus.subscribe(EventType::kRelayOff, recordEvent,
                             &relayOffRecorder);

  assert(context.app.setup());

  assert(context.relayController.allOffCallCount() == 1);
  assert(context.relayController.state(RelayId::kRecalque) ==
         RelayDesiredState::kOff);
  assert(context.relayController.state(RelayId::kHeater) ==
         RelayDesiredState::kOff);
  assert(context.relayController.state(RelayId::kAtoPump) ==
         RelayDesiredState::kOff);
  assert(context.relayController.state(RelayId::kReserve) ==
         RelayDesiredState::kOff);
  assert(!reeflow::core::state::currentSystemState().relays.recalque.enabled);
  assert(!reeflow::core::state::currentSystemState().relays.heater.enabled);
  assert(!reeflow::core::state::currentSystemState().relays.atoPump.enabled);
  assert(!reeflow::core::state::currentSystemState().relays.reserve.enabled);
  assert(relayOnRecorder.count == 0);
  assert(relayOffRecorder.count == 0);
}

void testSetupWithNoSavedModeKeepsNormal() {
  TestCoreAppContext context;
  context.modeStore.simulateNoSavedMode();

  assert(context.app.setup());

  assert(reeflow::core::state::currentSystemState().modes.currentMode ==
         NORMAL);
  assert(reeflow::core::state::currentSystemState().modes.startedAt == 0);
  assert(reeflow::core::state::currentSystemState().modes.remainingTime == 0);
}

void testSetupRejectsInvalidSavedModeAndKeepsNormal() {
  TestCoreAppContext context;
  context.modeStore.simulateInvalidSavedMode();

  assert(context.app.setup());

  assert(reeflow::core::state::currentSystemState().modes.currentMode ==
         NORMAL);
  assert(context.relayController.recordedCallCount() == 1);
  assert(context.logSink.messages().back() ==
         "[INFO] core: initialized\n");
}

void testSetupRestoresSavedNormalWithoutRelayOn() {
  TestCoreAppContext context;
  context.modeStore.simulateLoadedMode(NORMAL);

  assert(context.app.setup());

  assert(reeflow::core::state::currentSystemState().modes.currentMode ==
         NORMAL);
  assert(context.relayController.recordedCallCount() == 1);
}

void testSetupFallsBackFromSavedFeedingToNormal() {
  TestCoreAppContext context;
  context.modeStore.simulateLoadedMode(FEEDING);

  assert(context.app.setup());

  assert(reeflow::core::state::currentSystemState().modes.currentMode ==
         NORMAL);
  assert(context.relayController.recordedCallCount() == 1);
}

void testSetupRestoresSavedTpaWithSafeEffects() {
  TestCoreAppContext context;
  context.modeStore.simulateLoadedMode(TPA);

  assert(context.app.setup());

  assert(reeflow::core::state::currentSystemState().modes.currentMode == TPA);
  assert(!reeflow::core::state::currentSystemState().relays.recalque.enabled);
  assert(!reeflow::core::state::currentSystemState().relays.atoPump.enabled);
  assert(context.relayController.recordedCallCount() == 1);
}

void testSetupRestoresSavedMaintenanceWithoutRelayCommand() {
  TestCoreAppContext context;
  context.modeStore.simulateLoadedMode(MAINTENANCE);

  assert(context.app.setup());

  assert(reeflow::core::state::currentSystemState().modes.currentMode ==
         MAINTENANCE);
  assert(context.relayController.recordedCallCount() == 1);
}

void testLocalRelayApiAllowsManualCommandsAfterSetup() {
  TestCoreAppContext context;
  assert(context.app.setup());
  context.timeSource.setUptimeMillis(250);

  assert(context.app.setLocalRelay(RelayId::kRecalque,
                                   RelayDesiredState::kOn) ==
         RelayCommandResult::kSuccess);
  assert(context.app.setLocalRelay(RelayId::kHeater,
                                   RelayDesiredState::kOn) ==
         RelayCommandResult::kSuccess);
  assert(context.app.setLocalRelay(RelayId::kAtoPump,
                                   RelayDesiredState::kOn) ==
         RelayCommandResult::kSuccess);
  assert(context.app.setLocalRelay(RelayId::kReserve,
                                   RelayDesiredState::kOn) ==
         RelayCommandResult::kSuccess);
  assert(context.app.setLocalRelay(RelayId::kReserve,
                                   RelayDesiredState::kOff) ==
         RelayCommandResult::kSuccess);

  assert(reeflow::core::state::currentSystemState().relays.recalque.enabled);
  assert(reeflow::core::state::currentSystemState().relays.heater.enabled);
  assert(reeflow::core::state::currentSystemState().relays.atoPump.enabled);
  assert(!reeflow::core::state::currentSystemState().relays.reserve.enabled);
  assert(reeflow::core::state::currentSystemState().relays.recalque.source ==
         RelaySource::kLocal);
  assert(reeflow::core::state::currentSystemState().relays.reserve.source ==
         RelaySource::kLocal);
}

void testRelaySafeStateFailureKeepsSetupFailedAndRelaysOff() {
  TestCoreAppContext context;
  context.relayController.setAllOffResult(
      RelayCommandResult::kControllerFailure);

  assert(!context.app.setup());

  assert(context.relayController.allOffCallCount() == 1);
  assert(!reeflow::core::state::currentSystemState().relays.recalque.enabled);
  assert(!reeflow::core::state::currentSystemState().relays.heater.enabled);
  assert(!reeflow::core::state::currentSystemState().relays.atoPump.enabled);
  assert(!reeflow::core::state::currentSystemState().relays.reserve.enabled);
  assert(context.app.setLocalRelay(RelayId::kRecalque,
                                   RelayDesiredState::kOn) ==
         RelayCommandResult::kControllerFailure);
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

  assert(result.executedCount == 4);
  assert(result.failedCount == 0);
  assert(context.watchdogBackend.feedCalls() == 1);
  assert(reeflow::core::state::currentSystemState().ato.status ==
         AtoStatus::kDisabled);
}

void testModeTaskRunsOnlyAfterConfiguredInterval() {
  TestCoreAppContext context;
  assert(context.app.setup());
  assert(context.app.requestMode(FEEDING) == ModeServiceResult::kSuccess);
  assert(reeflow::core::state::currentSystemState().modes.remainingTime == 600);

  context.timeSource.advanceMillis(999);
  reeflow::core::scheduler::SchedulerRunResult result =
      context.app.loopOnce();

  assert(result.executedCount == 0);
  assert(reeflow::core::state::currentSystemState().modes.remainingTime == 600);

  context.timeSource.advanceMillis(1);
  result = context.app.loopOnce();

  assert(result.executedCount == 4);
  assert(result.failedCount == 0);
  assert(reeflow::core::state::currentSystemState().modes.remainingTime == 599);
}

void testLocalModeApiSupportsAllCanonicalModes() {
  TestCoreAppContext context;
  assert(context.app.setup());

  assert(context.app.requestMode(FEEDING) == ModeServiceResult::kSuccess);
  assert(reeflow::core::state::currentSystemState().modes.currentMode ==
         FEEDING);

  assert(context.app.requestMode(NORMAL) == ModeServiceResult::kSuccess);
  assert(reeflow::core::state::currentSystemState().modes.currentMode ==
         NORMAL);

  assert(context.app.requestMode(TPA) == ModeServiceResult::kSuccess);
  assert(reeflow::core::state::currentSystemState().modes.currentMode == TPA);

  assert(context.app.requestMode(NORMAL) == ModeServiceResult::kSuccess);
  assert(context.app.requestMode(MAINTENANCE) ==
         ModeServiceResult::kSuccess);
  assert(reeflow::core::state::currentSystemState().modes.currentMode ==
         MAINTENANCE);
}

void testModeSchedulerFinishesFeedingButNotManualModes() {
  TestCoreAppContext context;
  assert(context.app.setup());
  assert(context.app.requestMode(FEEDING) == ModeServiceResult::kSuccess);

  context.timeSource.advanceMillis(600000);
  reeflow::core::scheduler::SchedulerRunResult result =
      context.app.loopOnce();

  assert(result.failedCount == 0);
  assert(reeflow::core::state::currentSystemState().modes.currentMode ==
         NORMAL);
  assert(!reeflow::core::state::currentSystemState().relays.recalque.enabled);

  assert(context.app.requestMode(TPA) == ModeServiceResult::kSuccess);
  context.timeSource.advanceMillis(600000);
  result = context.app.loopOnce();
  assert(result.failedCount == 0);
  assert(reeflow::core::state::currentSystemState().modes.currentMode == TPA);

  assert(context.app.requestMode(NORMAL) == ModeServiceResult::kSuccess);
  assert(context.app.requestMode(MAINTENANCE) ==
         ModeServiceResult::kSuccess);
  context.timeSource.advanceMillis(600000);
  result = context.app.loopOnce();
  assert(result.failedCount == 0);
  assert(reeflow::core::state::currentSystemState().modes.currentMode ==
         MAINTENANCE);
}

void testAtoDoesNotStartWhenModeGateBlocksAto() {
  TestCoreAppContext context;
  assert(context.app.setup());
  auto atoConfig = context.configManager.ato();
  atoConfig.enabled = true;
  assert(context.configManager.updateAto(atoConfig));

  auto waterLevel = reeflow::core::state::currentSystemState().waterLevel;
  waterLevel.currentLevel = 0;
  waterLevel.minimumLevel = 20;
  waterLevel.maximumLevel = 80;
  waterLevel.status = WaterLevelStatus::kNormal;
  reeflow::core::state::updateWaterLevelState(waterLevel);

  assert(context.app.requestMode(TPA) == ModeServiceResult::kSuccess);
  context.timeSource.advanceMillis(1000);
  const reeflow::core::scheduler::SchedulerRunResult result =
      context.app.loopOnce();

  assert(result.failedCount == 0);
  assert(!reeflow::core::state::currentSystemState().ato.pumpRunning);
  assert(!reeflow::core::state::currentSystemState().relays.atoPump.enabled);
}

void testModeTaskFailureIsReportedByScheduler() {
  TestCoreAppContext context;
  assert(context.app.setup());
  assert(context.app.requestMode(FEEDING) == ModeServiceResult::kSuccess);
  context.modeStore.simulateSaveFailure();

  context.timeSource.advanceMillis(600000);
  const reeflow::core::scheduler::SchedulerRunResult result =
      context.app.loopOnce();

  assert(result.failedCount == 1);
  assert(reeflow::core::state::currentSystemState().modes.currentMode ==
         NORMAL);
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
  testSetupAppliesRelaySafeStateWithoutRelayEvents();
  testSetupWithNoSavedModeKeepsNormal();
  testSetupRejectsInvalidSavedModeAndKeepsNormal();
  testSetupRestoresSavedNormalWithoutRelayOn();
  testSetupFallsBackFromSavedFeedingToNormal();
  testSetupRestoresSavedTpaWithSafeEffects();
  testSetupRestoresSavedMaintenanceWithoutRelayCommand();
  testLocalRelayApiAllowsManualCommandsAfterSetup();
  testRelaySafeStateFailureKeepsSetupFailedAndRelaysOff();
  testLoopFeedsWatchdogThroughScheduler();
  testModeTaskRunsOnlyAfterConfiguredInterval();
  testLocalModeApiSupportsAllCanonicalModes();
  testModeSchedulerFinishesFeedingButNotManualModes();
  testAtoDoesNotStartWhenModeGateBlocksAto();
  testModeTaskFailureIsReportedByScheduler();
  testStateAndConfigEventsFlowDuringIntegration();
  return 0;
}
