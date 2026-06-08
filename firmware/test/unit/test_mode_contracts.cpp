#include <assert.h>
#include <string.h>

#include "config/config_manager.h"
#include "core/events/event_bus.h"
#include "fakes/fake_mode_automation_gate.h"
#include "fakes/fake_mode_effects.h"
#include "fakes/fake_mode_store.h"
#include "modules/modes/mode_automation_gate.h"
#include "modules/modes/mode_config.h"
#include "modules/modes/mode_effects.h"
#include "modules/modes/mode_events.h"
#include "modules/modes/mode_policy.h"
#include "modules/modes/mode_service.h"
#include "modules/modes/mode_store.h"
#include "modules/modes/mode_types.h"

namespace {

using reeflow::config::ConfigManager;
using reeflow::core::events::Event;
using reeflow::core::events::EventBus;
using reeflow::core::events::EventType;
using reeflow::core::events::StateArea;
using reeflow::core::state::OperationalMode;
using reeflow::modules::modes::FEEDING;
using reeflow::modules::modes::MAINTENANCE;
using reeflow::modules::modes::NORMAL;
using reeflow::modules::modes::TPA;
using reeflow::modules::modes::ModeAutomation;
using reeflow::modules::modes::ModeAutomationGateResult;
using reeflow::modules::modes::ModeBehaviorConfig;
using reeflow::modules::modes::ModeCommandReason;
using reeflow::modules::modes::ModeCommandSource;
using reeflow::modules::modes::ModeEffectResult;
using reeflow::modules::modes::ModeEffectsRequest;
using reeflow::modules::modes::ModeEventType;
using reeflow::modules::modes::ModeFinishReason;
using reeflow::modules::modes::ModePolicyDecision;
using reeflow::modules::modes::ModeReturnBehavior;
using reeflow::modules::modes::ModeServiceResult;
using reeflow::modules::modes::ModeStoreLoadStatus;
using reeflow::modules::modes::ModeStoreSaveResult;
using reeflow::modules::modes::defaultAtoAutomationAllowed;
using reeflow::modules::modes::isCanonicalMode;
using reeflow::modules::modes::makeLoadedModeResult;
using reeflow::modules::modes::makeLocalModeCommand;
using reeflow::modules::modes::makeModeBehaviorConfig;
using reeflow::modules::modes::makeModeCoreEvent;
using reeflow::modules::modes::modeCoreEventType;
using reeflow::modules::modes::modeEventName;
using reeflow::modules::modes::modeName;
using reeflow::test::fakes::FakeModeAutomationGate;
using reeflow::test::fakes::FakeModeEffects;
using reeflow::test::fakes::FakeModeStore;

struct Recorder {
  Event events[4];
  unsigned int count;
};

bool recordEvent(const Event& event, void* context) {
  Recorder* recorder = static_cast<Recorder*>(context);
  recorder->events[recorder->count++] = event;
  return true;
}

void testCanonicalModesMatchSystemStateModes() {
  assert(NORMAL == OperationalMode::kNormal);
  assert(FEEDING == OperationalMode::kFeeding);
  assert(TPA == OperationalMode::kTpa);
  assert(MAINTENANCE == OperationalMode::kMaintenance);

  assert(isCanonicalMode(NORMAL));
  assert(isCanonicalMode(FEEDING));
  assert(isCanonicalMode(TPA));
  assert(isCanonicalMode(MAINTENANCE));
  assert(!isCanonicalMode(static_cast<OperationalMode>(255)));

  assert(strcmp(modeName(NORMAL), "NORMAL") == 0);
  assert(strcmp(modeName(FEEDING), "FEEDING") == 0);
  assert(strcmp(modeName(TPA), "TPA") == 0);
  assert(strcmp(modeName(MAINTENANCE), "MAINTENANCE") == 0);
}

void testLocalModeCommandDoesNotExposeRemoteTransport() {
  const auto command =
      makeLocalModeCommand(FEEDING, 1200, ModeCommandReason::kUserRequest);

  assert(command.requestedMode == FEEDING);
  assert(command.source == ModeCommandSource::kLocal);
  assert(command.requestedAtMillis == 1200);
  assert(command.reason == ModeCommandReason::kUserRequest);
}

void testModeEventsMapToLocalEventBus() {
  assert(strcmp(modeEventName(ModeEventType::kModeChanged),
                "MODE_CHANGED") == 0);
  assert(strcmp(modeEventName(ModeEventType::kModeStarted),
                "MODE_STARTED") == 0);
  assert(strcmp(modeEventName(ModeEventType::kModeFinished),
                "MODE_FINISHED") == 0);

  assert(modeCoreEventType(ModeEventType::kModeChanged) ==
         EventType::kModeChanged);
  assert(modeCoreEventType(ModeEventType::kModeStarted) ==
         EventType::kModeStarted);
  assert(modeCoreEventType(ModeEventType::kModeFinished) ==
         EventType::kModeFinished);

  EventBus bus;
  Recorder recorder = {};
  bus.subscribe(EventType::kModeChanged, recordEvent, &recorder);

  const auto result = bus.publish(makeModeCoreEvent(ModeEventType::kModeChanged));

  assert(result.deliveredCount == 1);
  assert(result.failedCount == 0);
  assert(recorder.count == 1);
  assert(recorder.events[0].type == EventType::kModeChanged);
  assert(recorder.events[0].stateArea == StateArea::kModes);
}

void testModeEventPayloadContractsCarryFunctionalData() {
  reeflow::modules::modes::ModeChangedEventData changed = {};
  changed.previousMode = NORMAL;
  changed.currentMode = FEEDING;
  changed.startedAtMillis = 100;
  changed.remainingTimeSeconds = 600;

  reeflow::modules::modes::ModeFinishedEventData finished = {};
  finished.finishedMode = FEEDING;
  finished.nextMode = NORMAL;
  finished.reason = ModeFinishReason::kAutomaticTimeout;
  finished.finishedAtMillis = 700;

  assert(changed.previousMode == NORMAL);
  assert(changed.currentMode == FEEDING);
  assert(changed.remainingTimeSeconds == 600);
  assert(finished.finishedMode == FEEDING);
  assert(finished.nextMode == NORMAL);
  assert(finished.reason == ModeFinishReason::kAutomaticTimeout);
}

void testModeBehaviorConfigUsesConfigManagerFeedingTimer() {
  EventBus bus;
  ConfigManager configManager(bus);
  auto timers = configManager.timers();
  timers.feedingDurationSeconds = 321;
  assert(configManager.updateTimers(timers));

  const ModeBehaviorConfig config = makeModeBehaviorConfig(configManager);

  assert(config.feedingDurationSeconds == 321);
  assert(config.feedingReturn == ModeReturnBehavior::kAutomatic);
  assert(config.tpaReturn == ModeReturnBehavior::kManual);
  assert(config.maintenanceReturn == ModeReturnBehavior::kManual);
}

void testModeBehaviorDefaultsMatchPhase7Task() {
  EventBus bus;
  ConfigManager configManager(bus);
  const ModeBehaviorConfig config = makeModeBehaviorConfig(configManager);

  assert(config.normalReleasesModeBlocks);
  assert(config.feedingTurnsOffRecalque);
  assert(config.feedingBlocksAtoAutomation);
  assert(config.tpaTurnsOffRecalque);
  assert(config.tpaTurnsOffAtoPump);
  assert(config.tpaBlocksAtoAutomation);
  assert(config.maintenanceBlocksNonCriticalAutomations);
  assert(config.maintenanceBlocksAtoAutomation);
}

void testNormalDefaultDoesNotTurnRelaysOn() {
  const ModeEffectsRequest request =
      reeflow::modules::modes::makeNoOpModeEffectsRequest(NORMAL, 10);

  assert(request.mode == NORMAL);
  assert(!request.turnOffRecalque);
  assert(!request.turnOffAtoPump);
  assert(!request.preserveManualRelayControl);
}

void testAutomationGateContractAllowsAtoInNormalAndBlocksConfiguredModes() {
  assert(defaultAtoAutomationAllowed(NORMAL));
  assert(!defaultAtoAutomationAllowed(FEEDING));
  assert(!defaultAtoAutomationAllowed(TPA));
  assert(!defaultAtoAutomationAllowed(MAINTENANCE));

  FakeModeAutomationGate gate;
  assert(gate.isAutomationAllowed(ModeAutomation::kAto));

  assert(gate.setAutomationBlocked(ModeAutomation::kAto, true, TPA) ==
         ModeAutomationGateResult::kSuccess);
  assert(!gate.isAutomationAllowed(ModeAutomation::kAto));

  assert(gate.setAutomationBlocked(ModeAutomation::kAto, false, NORMAL) ==
         ModeAutomationGateResult::kSuccess);
  assert(gate.isAutomationAllowed(ModeAutomation::kAto));
  assert(gate.callCount() == 2);
  assert(gate.call(0).mode == TPA);
  assert(gate.call(0).blocked);
}

void testFakeModeStoreSupportsLoadedMissingInvalidAndSaveFailure() {
  FakeModeStore store;
  store.simulateLoadedMode(MAINTENANCE);

  auto loadResult = store.loadMode();
  assert(loadResult.status == ModeStoreLoadStatus::kLoaded);
  assert(loadResult.mode == MAINTENANCE);
  assert(store.loadCount() == 1);

  store.simulateNoSavedMode();
  loadResult = store.loadMode();
  assert(loadResult.status == ModeStoreLoadStatus::kNotFound);

  store.simulateInvalidSavedMode();
  loadResult = store.loadMode();
  assert(loadResult.status == ModeStoreLoadStatus::kInvalidMode);

  assert(store.saveMode(TPA) == ModeStoreSaveResult::kSuccess);
  assert(store.lastSavedMode() == TPA);
  assert(store.saveCount() == 1);

  store.simulateSaveFailure();
  assert(store.saveMode(FEEDING) == ModeStoreSaveResult::kFailure);
  assert(store.lastSavedMode() == FEEDING);
  assert(store.saveCount() == 2);
}

void testStoreRejectsInvalidModeContract() {
  const auto result = makeLoadedModeResult(static_cast<OperationalMode>(255));
  assert(result.status == ModeStoreLoadStatus::kInvalidMode);

  FakeModeStore store;
  assert(store.saveMode(static_cast<OperationalMode>(255)) ==
         ModeStoreSaveResult::kInvalidMode);
}

void testFakeModeEffectsRecordsRequestsAndFailures() {
  FakeModeEffects effects;
  ModeEffectsRequest request = {};
  request.mode = TPA;
  request.requestedAtMillis = 100;
  request.turnOffRecalque = true;
  request.turnOffAtoPump = true;

  assert(effects.applyModeEffects(request) == ModeEffectResult::kSuccess);
  assert(effects.requestCount() == 1);
  assert(effects.request(0).mode == TPA);
  assert(effects.request(0).turnOffRecalque);
  assert(effects.request(0).turnOffAtoPump);

  effects.simulateFailure();
  assert(effects.applyModeEffects(request) == ModeEffectResult::kFailed);
  assert(effects.requestCount() == 2);
}

void testPolicyAndServiceContractsArePresentWithoutBehavior() {
  reeflow::modules::modes::ModePolicyPlan plan = {};
  plan.decision = ModePolicyDecision::kNoChange;
  plan.nextModes.currentMode = NORMAL;

  reeflow::modules::modes::ModeServiceTickResult tick = {};
  tick.result = ModeServiceResult::kSuccess;
  tick.modeChanged = false;

  assert(plan.decision == ModePolicyDecision::kNoChange);
  assert(plan.nextModes.currentMode == NORMAL);
  assert(tick.result == ModeServiceResult::kSuccess);
  assert(!tick.modeChanged);
}

}  // namespace

int main() {
  testCanonicalModesMatchSystemStateModes();
  testLocalModeCommandDoesNotExposeRemoteTransport();
  testModeEventsMapToLocalEventBus();
  testModeEventPayloadContractsCarryFunctionalData();
  testModeBehaviorConfigUsesConfigManagerFeedingTimer();
  testModeBehaviorDefaultsMatchPhase7Task();
  testNormalDefaultDoesNotTurnRelaysOn();
  testAutomationGateContractAllowsAtoInNormalAndBlocksConfiguredModes();
  testFakeModeStoreSupportsLoadedMissingInvalidAndSaveFailure();
  testStoreRejectsInvalidModeContract();
  testFakeModeEffectsRecordsRequestsAndFailures();
  testPolicyAndServiceContractsArePresentWithoutBehavior();
  return 0;
}
