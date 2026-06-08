#include <assert.h>

#include "config/config_manager.h"
#include "core/events/event_bus.h"
#include "core/state/system_state.h"
#include "fakes/fake_mode_effects.h"
#include "fakes/fake_mode_store.h"
#include "fakes/fake_time_source.h"
#include "modules/modes/mode_service.h"

namespace {

using reeflow::config::ConfigManager;
using reeflow::core::events::Event;
using reeflow::core::events::EventBus;
using reeflow::core::events::EventType;
using reeflow::core::state::ModesState;
using reeflow::core::state::SystemState;
using reeflow::core::state::WaterLevelStatus;
using reeflow::modules::modes::FEEDING;
using reeflow::modules::modes::MAINTENANCE;
using reeflow::modules::modes::NORMAL;
using reeflow::modules::modes::TPA;
using reeflow::modules::modes::ModeAutomation;
using reeflow::modules::modes::ModeCommandReason;
using reeflow::modules::modes::ModeEffectResult;
using reeflow::modules::modes::ModeService;
using reeflow::modules::modes::ModeServiceResult;
using reeflow::modules::modes::makeLocalModeCommand;
using reeflow::test::fakes::FakeModeEffects;
using reeflow::test::fakes::FakeModeStore;
using reeflow::test::fakes::FakeTimeSource;

struct EventRecorder {
  Event events[16];
  uint8_t count;
};

bool recordEvent(const Event& event, void* context) {
  EventRecorder* recorder = static_cast<EventRecorder*>(context);
  recorder->events[recorder->count++] = event;
  return true;
}

struct Fixture {
  EventBus eventBus;
  ConfigManager configManager;
  FakeTimeSource timeSource;
  FakeModeEffects effects;
  FakeModeStore store;
  ModeService service;
  EventRecorder modeChangedEvents;
  EventRecorder modeStartedEvents;
  EventRecorder modeFinishedEvents;

  Fixture()
      : configManager(eventBus),
        service(configManager, timeSource, eventBus, effects, store),
        modeChangedEvents({}),
        modeStartedEvents({}),
        modeFinishedEvents({}) {
    reeflow::core::state::resetSystemState();
    reeflow::core::state::setSystemStateEventBus(eventBus);
    timeSource.setUptimeMillis(10000);
    eventBus.subscribe(EventType::kModeChanged, recordEvent,
                       &modeChangedEvents);
    eventBus.subscribe(EventType::kModeStarted, recordEvent,
                       &modeStartedEvents);
    eventBus.subscribe(EventType::kModeFinished, recordEvent,
                       &modeFinishedEvents);
  }
};

void setMode(reeflow::modules::modes::OperationalMode mode,
             uint32_t startedAt, uint32_t remainingTime) {
  ModesState modes = {};
  modes.currentMode = mode;
  modes.startedAt = startedAt;
  modes.remainingTime = remainingTime;
  reeflow::core::state::updateModesState(modes);
}

void setWaterLevel(uint16_t level, WaterLevelStatus status) {
  auto waterLevel = reeflow::core::state::currentSystemState().waterLevel;
  waterLevel.currentLevel = level;
  waterLevel.status = status;
  waterLevel.lastUpdate = 500;
  reeflow::core::state::updateWaterLevelState(waterLevel);
}

void assertMode(reeflow::modules::modes::OperationalMode mode,
                uint32_t startedAt, uint32_t remainingTime) {
  const ModesState& modes = reeflow::core::state::currentSystemState().modes;
  assert(modes.currentMode == mode);
  assert(modes.startedAt == startedAt);
  assert(modes.remainingTime == remainingTime);
}

void assertUnrelatedBlocksMatch(const SystemState& before) {
  const SystemState& after = reeflow::core::state::currentSystemState();
  assert(after.temperature.status == before.temperature.status);
  assert(after.temperature.currentTemperature ==
         before.temperature.currentTemperature);
  assert(after.waterLevel.currentLevel == before.waterLevel.currentLevel);
  assert(after.waterLevel.status == before.waterLevel.status);
  assert(after.lighting.mode == before.lighting.mode);
  assert(after.ato.status == before.ato.status);
  assert(after.ato.pumpRunning == before.ato.pumpRunning);
  assert(after.network.wifiConnected == before.network.wifiConnected);
  assert(after.alerts.activeAlertCount == before.alerts.activeAlertCount);
  assert(after.systemHealth.uptime == before.systemHealth.uptime);
}

void testEnterFeedingAppliesEffectsUpdatesStateEventsAndStore() {
  Fixture fixture;

  const auto result =
      fixture.service.requestMode(makeLocalModeCommand(FEEDING, 10000));

  assert(result == ModeServiceResult::kSuccess);
  assertMode(FEEDING, 10000, 600);
  assert(fixture.effects.requestCount() == 1);
  assert(fixture.effects.request(0).mode == FEEDING);
  assert(fixture.effects.request(0).turnOffRecalque);
  assert(fixture.effects.request(0).updateAtoAutomationGate);
  assert(fixture.effects.request(0).blockAtoAutomation);
  assert(fixture.store.saveCount() == 1);
  assert(fixture.store.lastSavedMode() == FEEDING);
  assert(fixture.modeStartedEvents.count == 1);
  assert(fixture.modeChangedEvents.count == 1);
  assert(fixture.modeFinishedEvents.count == 0);
}

void testEffectFailurePreservesPreviousModeAndSkipsEventsAndStore() {
  Fixture fixture;
  fixture.effects.simulateFailure();

  const auto result =
      fixture.service.requestMode(makeLocalModeCommand(FEEDING, 10000));

  assert(result == ModeServiceResult::kEffectsFailed);
  assertMode(NORMAL, 0, 0);
  assert(fixture.effects.requestCount() == 1);
  assert(fixture.store.saveCount() == 0);
  assert(fixture.modeStartedEvents.count == 0);
  assert(fixture.modeChangedEvents.count == 0);
}

void testEnterTpaRequestsConfiguredEffectsAndStore() {
  Fixture fixture;

  const auto result =
      fixture.service.requestMode(makeLocalModeCommand(TPA, 10000));

  assert(result == ModeServiceResult::kSuccess);
  assertMode(TPA, 10000, 0);
  assert(fixture.effects.requestCount() == 1);
  assert(fixture.effects.request(0).mode == TPA);
  assert(fixture.effects.request(0).turnOffRecalque);
  assert(fixture.effects.request(0).turnOffAtoPump);
  assert(fixture.effects.request(0).updateAtoAutomationGate);
  assert(fixture.effects.request(0).blockAtoAutomation);
  assert(fixture.store.lastSavedMode() == TPA);
}

void testEnterMaintenanceRequestsAutomationSuspension() {
  Fixture fixture;

  const auto result =
      fixture.service.requestMode(makeLocalModeCommand(MAINTENANCE, 10000));

  assert(result == ModeServiceResult::kSuccess);
  assertMode(MAINTENANCE, 10000, 0);
  assert(fixture.effects.requestCount() == 1);
  assert(fixture.effects.request(0).mode == MAINTENANCE);
  assert(fixture.effects.request(0).preserveManualRelayControl);
  assert(fixture.effects.request(0).updateAtoAutomationGate);
  assert(fixture.effects.request(0).blockAtoAutomation);
  assert(fixture.modeStartedEvents.count == 1);
}

void testReturnToNormalReleasesBlocksWithoutRelayOnCommand() {
  Fixture fixture;
  setMode(FEEDING, 1000, 500);

  const auto result =
      fixture.service.requestMode(makeLocalModeCommand(NORMAL, 10000));

  assert(result == ModeServiceResult::kSuccess);
  assertMode(NORMAL, 10000, 0);
  assert(fixture.effects.requestCount() == 1);
  assert(fixture.effects.request(0).mode == NORMAL);
  assert(fixture.effects.request(0).releaseModeBlocks);
  assert(!fixture.effects.request(0).turnOffRecalque);
  assert(!fixture.effects.request(0).turnOffAtoPump);
  assert(fixture.modeFinishedEvents.count == 1);
  assert(fixture.modeChangedEvents.count == 1);
  assert(fixture.modeStartedEvents.count == 0);
  assert(fixture.store.lastSavedMode() == NORMAL);
}

void testInvalidModeCommandDoesNotAlterSystemState() {
  Fixture fixture;

  const auto result = fixture.service.requestMode(makeLocalModeCommand(
      static_cast<reeflow::modules::modes::OperationalMode>(255), 10000));

  assert(result == ModeServiceResult::kInvalidCommand);
  assertMode(NORMAL, 0, 0);
  assert(fixture.effects.requestCount() == 0);
  assert(fixture.store.saveCount() == 0);
  assert(fixture.modeChangedEvents.count == 0);
}

void testIdempotentCommandDoesNotDuplicateEventsOrStore() {
  Fixture fixture;

  assert(fixture.service.requestMode(makeLocalModeCommand(NORMAL, 10000)) ==
         ModeServiceResult::kSuccess);

  assertMode(NORMAL, 0, 0);
  assert(fixture.effects.requestCount() == 0);
  assert(fixture.store.saveCount() == 0);
  assert(fixture.modeChangedEvents.count == 0);
  assert(fixture.modeStartedEvents.count == 0);
  assert(fixture.modeFinishedEvents.count == 0);
}

void testEvaluateOnceUpdatesFeedingRemainingTimeWithoutEventsOrStore() {
  Fixture fixture;
  setMode(FEEDING, 1000, 600);
  fixture.timeSource.setUptimeMillis(301000);

  const auto result = fixture.service.evaluateOnce();

  assert(result.result == ModeServiceResult::kSuccess);
  assert(!result.modeChanged);
  assertMode(FEEDING, 1000, 300);
  assert(fixture.effects.requestCount() == 0);
  assert(fixture.store.saveCount() == 0);
  assert(fixture.modeChangedEvents.count == 0);
}

void testEvaluateOnceReturnsFeedingToNormalByTimeout() {
  Fixture fixture;
  setMode(FEEDING, 1000, 1);
  fixture.timeSource.setUptimeMillis(601000);

  const auto result = fixture.service.evaluateOnce();

  assert(result.result == ModeServiceResult::kSuccess);
  assert(result.modeChanged);
  assertMode(NORMAL, 601000, 0);
  assert(fixture.effects.requestCount() == 1);
  assert(fixture.effects.request(0).mode == NORMAL);
  assert(fixture.effects.request(0).releaseModeBlocks);
  assert(!fixture.effects.request(0).turnOffRecalque);
  assert(fixture.modeFinishedEvents.count == 1);
  assert(fixture.modeChangedEvents.count == 1);
  assert(fixture.store.saveCount() == 1);
  assert(fixture.store.lastSavedMode() == NORMAL);
}

void testStoreFailureIsReportedAfterOperationalTransition() {
  Fixture fixture;
  fixture.store.simulateSaveFailure();

  const auto result =
      fixture.service.requestMode(makeLocalModeCommand(FEEDING, 10000));

  assert(result == ModeServiceResult::kStoreFailed);
  assertMode(FEEDING, 10000, 600);
  assert(fixture.effects.requestCount() == 1);
  assert(fixture.store.saveCount() == 1);
  assert(fixture.modeStartedEvents.count == 1);
  assert(fixture.modeChangedEvents.count == 1);
}

void testRestoreWithoutSavedModeKeepsNormal() {
  Fixture fixture;
  fixture.store.simulateNoSavedMode();

  const auto result = fixture.service.restoreModeFromStore();

  assert(result == ModeServiceResult::kSuccess);
  assertMode(NORMAL, 0, 0);
  assert(fixture.effects.requestCount() == 0);
  assert(fixture.store.loadCount() == 1);
  assert(fixture.store.saveCount() == 0);
}

void testRestoreInvalidSavedModeKeepsNormalWithoutEffects() {
  Fixture fixture;
  fixture.store.simulateInvalidSavedMode();

  const auto result = fixture.service.restoreModeFromStore();

  assert(result == ModeServiceResult::kStoreFailed);
  assertMode(NORMAL, 0, 0);
  assert(fixture.effects.requestCount() == 0);
  assert(fixture.store.saveCount() == 0);
}

void testRestoreSavedNormalDoesNotTurnRelaysOn() {
  Fixture fixture;
  fixture.store.simulateLoadedMode(NORMAL);

  const auto result = fixture.service.restoreModeFromStore();

  assert(result == ModeServiceResult::kSuccess);
  assertMode(NORMAL, 0, 0);
  assert(fixture.effects.requestCount() == 0);
  assert(fixture.store.saveCount() == 0);
}

void testRestoreSavedFeedingFallsBackToNormalSafely() {
  Fixture fixture;
  fixture.store.simulateLoadedMode(FEEDING);

  const auto result = fixture.service.restoreModeFromStore();

  assert(result == ModeServiceResult::kSuccess);
  assertMode(NORMAL, 0, 0);
  assert(fixture.effects.requestCount() == 0);
  assert(fixture.store.saveCount() == 0);
}

void testRestoreSavedTpaReappliesSafeEffects() {
  Fixture fixture;
  fixture.store.simulateLoadedMode(TPA);

  const auto result = fixture.service.restoreModeFromStore();

  assert(result == ModeServiceResult::kSuccess);
  assertMode(TPA, 10000, 0);
  assert(fixture.effects.requestCount() == 1);
  assert(fixture.effects.request(0).mode == TPA);
  assert(fixture.effects.request(0).turnOffRecalque);
  assert(fixture.effects.request(0).turnOffAtoPump);
  assert(fixture.effects.request(0).updateAtoAutomationGate);
  assert(fixture.effects.request(0).blockAtoAutomation);
  assert(fixture.store.saveCount() == 0);
}

void testRestoreSavedMaintenanceReappliesAutomationBlock() {
  Fixture fixture;
  fixture.store.simulateLoadedMode(MAINTENANCE);

  const auto result = fixture.service.restoreModeFromStore();

  assert(result == ModeServiceResult::kSuccess);
  assertMode(MAINTENANCE, 10000, 0);
  assert(fixture.effects.requestCount() == 1);
  assert(fixture.effects.request(0).mode == MAINTENANCE);
  assert(fixture.effects.request(0).preserveManualRelayControl);
  assert(fixture.effects.request(0).updateAtoAutomationGate);
  assert(fixture.effects.request(0).blockAtoAutomation);
  assert(fixture.store.saveCount() == 0);
}

void testRestoreEffectFailureKeepsNormalAndReportsFailure() {
  Fixture fixture;
  fixture.store.simulateLoadedMode(TPA);
  fixture.effects.simulateFailure();

  const auto result = fixture.service.restoreModeFromStore();

  assert(result == ModeServiceResult::kEffectsFailed);
  assertMode(NORMAL, 0, 0);
  assert(fixture.effects.requestCount() == 1);
  assert(fixture.store.saveCount() == 0);
}

void testUnrelatedStateBlocksArePreserved() {
  Fixture fixture;
  setWaterLevel(42, WaterLevelStatus::kNormal);
  const SystemState before = reeflow::core::state::currentSystemState();

  const auto result =
      fixture.service.requestMode(makeLocalModeCommand(FEEDING, 10000));

  assert(result == ModeServiceResult::kSuccess);
  assertUnrelatedBlocksMatch(before);
}

}  // namespace

int main() {
  testEnterFeedingAppliesEffectsUpdatesStateEventsAndStore();
  testEffectFailurePreservesPreviousModeAndSkipsEventsAndStore();
  testEnterTpaRequestsConfiguredEffectsAndStore();
  testEnterMaintenanceRequestsAutomationSuspension();
  testReturnToNormalReleasesBlocksWithoutRelayOnCommand();
  testInvalidModeCommandDoesNotAlterSystemState();
  testIdempotentCommandDoesNotDuplicateEventsOrStore();
  testEvaluateOnceUpdatesFeedingRemainingTimeWithoutEventsOrStore();
  testEvaluateOnceReturnsFeedingToNormalByTimeout();
  testStoreFailureIsReportedAfterOperationalTransition();
  testRestoreWithoutSavedModeKeepsNormal();
  testRestoreInvalidSavedModeKeepsNormalWithoutEffects();
  testRestoreSavedNormalDoesNotTurnRelaysOn();
  testRestoreSavedFeedingFallsBackToNormalSafely();
  testRestoreSavedTpaReappliesSafeEffects();
  testRestoreSavedMaintenanceReappliesAutomationBlock();
  testRestoreEffectFailureKeepsNormalAndReportsFailure();
  testUnrelatedStateBlocksArePreserved();
  return 0;
}
