#include <assert.h>

#include "config/config_manager.h"
#include "core/events/event_bus.h"
#include "core/state/system_state.h"
#include "fakes/fake_relay_controller.h"
#include "fakes/fake_mode_automation_gate.h"
#include "fakes/fake_time_source.h"
#include "modules/ato/ato_service.h"
#include "modules/modes/mode_automation_gate.h"
#include "modules/modes/mode_types.h"
#include "modules/relays/relay_service.h"
#include "modules/relays/relay_types.h"

namespace {

using reeflow::config::AtoConfig;
using reeflow::config::ConfigManager;
using reeflow::core::events::Event;
using reeflow::core::events::EventBus;
using reeflow::core::events::EventType;
using reeflow::core::state::AtoState;
using reeflow::core::state::AtoStatus;
using reeflow::core::state::RelayEntryState;
using reeflow::core::state::RelaySource;
using reeflow::core::state::RelaysState;
using reeflow::core::state::SystemState;
using reeflow::core::state::WaterLevelState;
using reeflow::core::state::WaterLevelStatus;
using reeflow::modules::ato::AtoService;
using reeflow::modules::ato::AtoServiceResult;
using reeflow::modules::ato::makeDefaultAtoModuleConfig;
using reeflow::modules::modes::ModeAutomation;
using reeflow::modules::relays::ATO_PUMP;
using reeflow::modules::relays::RelayCommandResult;
using reeflow::modules::relays::RelayDesiredState;
using reeflow::modules::relays::RelayId;
using reeflow::modules::relays::RelayService;
using reeflow::test::fakes::FakeRelayController;
using reeflow::test::fakes::FakeModeAutomationGate;
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
  FakeRelayController relayController;
  FakeTimeSource timeSource;
  EventBus eventBus;
  ConfigManager configManager;
  RelayService relayService;
  FakeModeAutomationGate modeGate;
  AtoService atoService;
  EventRecorder atoStartEvents;
  EventRecorder atoStopEvents;
  EventRecorder atoTimeoutEvents;
  EventRecorder atoSensorOfflineEvents;
  EventRecorder atoRecoveredEvents;

  Fixture()
      : configManager(eventBus),
        relayService(relayController, timeSource, eventBus),
        atoService(relayService, configManager, timeSource, eventBus,
                   makeDefaultAtoModuleConfig(), &modeGate),
        atoStartEvents({}),
        atoStopEvents({}),
        atoTimeoutEvents({}),
        atoSensorOfflineEvents({}),
        atoRecoveredEvents({}) {
    reeflow::core::state::resetSystemState();
    reeflow::core::state::setSystemStateEventBus(eventBus);
    eventBus.subscribe(EventType::kAtoStart, recordEvent, &atoStartEvents);
    eventBus.subscribe(EventType::kAtoStop, recordEvent, &atoStopEvents);
    eventBus.subscribe(EventType::kAtoTimeout, recordEvent,
                       &atoTimeoutEvents);
    eventBus.subscribe(EventType::kAtoSensorOffline, recordEvent,
                       &atoSensorOfflineEvents);
    eventBus.subscribe(EventType::kAtoRecovered, recordEvent,
                       &atoRecoveredEvents);
  }
};

AtoConfig enabledAtoConfig(ConfigManager& configManager) {
  AtoConfig config = configManager.ato();
  config.enabled = true;
  config.minimumLevel = 20;
  config.maximumLevel = 80;
  config.timeoutMillis = 60000;
  config.cooldownMillis = 300000;
  return config;
}

void blockAtoByMode(Fixture& fixture) {
  assert(fixture.modeGate.setAutomationBlocked(
             ModeAutomation::kAto, true,
             reeflow::modules::modes::TPA) ==
         reeflow::modules::modes::ModeAutomationGateResult::kSuccess);
}

void enableAto(Fixture& fixture) {
  assert(fixture.configManager.updateAto(
      enabledAtoConfig(fixture.configManager)));
}

void setWaterLevel(uint16_t level, WaterLevelStatus status) {
  WaterLevelState waterLevel =
      reeflow::core::state::currentSystemState().waterLevel;
  waterLevel.currentLevel = level;
  waterLevel.minimumLevel = 20;
  waterLevel.maximumLevel = 80;
  waterLevel.status = status;
  waterLevel.lastUpdate = 100;
  reeflow::core::state::updateWaterLevelState(waterLevel);
}

void setAto(bool enabled, AtoStatus status, bool pumpRunning,
            uint32_t lastActivation, uint32_t lastCompletion,
            uint32_t timeoutCounter) {
  AtoState ato = reeflow::core::state::currentSystemState().ato;
  ato.enabled = enabled;
  ato.status = status;
  ato.pumpRunning = pumpRunning;
  ato.lastActivation = lastActivation;
  ato.lastCompletion = lastCompletion;
  ato.timeoutCounter = timeoutCounter;
  reeflow::core::state::updateAtoState(ato);
}

void setAtoPumpRelay(bool enabled, uint32_t lastChanged,
                     RelaySource source) {
  RelaysState relays = reeflow::core::state::currentSystemState().relays;
  relays.atoPump.enabled = enabled;
  relays.atoPump.lastChanged = lastChanged;
  relays.atoPump.source = source;
  reeflow::core::state::updateRelaysState(relays);
}

void assertAto(bool enabled, AtoStatus status, bool pumpRunning,
               uint32_t lastActivation, uint32_t lastCompletion,
               uint32_t timeoutCounter) {
  const AtoState& ato = reeflow::core::state::currentSystemState().ato;
  assert(ato.enabled == enabled);
  assert(ato.status == status);
  assert(ato.pumpRunning == pumpRunning);
  assert(ato.lastActivation == lastActivation);
  assert(ato.lastCompletion == lastCompletion);
  assert(ato.timeoutCounter == timeoutCounter);
}

void assertAtoPumpRelay(bool enabled, RelaySource source) {
  const RelayEntryState& relay =
      reeflow::core::state::currentSystemState().relays.atoPump;
  assert(relay.enabled == enabled);
  assert(relay.source == source);
}

void assertUnrelatedBlocksMatch(const SystemState& before) {
  const SystemState& after = reeflow::core::state::currentSystemState();
  assert(after.temperature.status == before.temperature.status);
  assert(after.temperature.currentTemperature ==
         before.temperature.currentTemperature);
  assert(after.waterLevel.currentLevel == before.waterLevel.currentLevel);
  assert(after.waterLevel.status == before.waterLevel.status);
  assert(after.lighting.mode == before.lighting.mode);
  assert(after.modes.currentMode == before.modes.currentMode);
  assert(after.network.wifiConnected == before.network.wifiConnected);
  assert(after.alerts.activeAlertCount == before.alerts.activeAlertCount);
  assert(after.systemHealth.uptime == before.systemHealth.uptime);
}

void testDisabledAtoUpdatesStatusWithoutPumpCommand() {
  Fixture fixture;
  fixture.timeSource.setUptimeMillis(1000);

  const auto result = fixture.atoService.evaluateOnce();

  assert(result == AtoServiceResult::kSuccess);
  assertAto(false, AtoStatus::kDisabled, false, 0, 0, 0);
  assert(fixture.relayController.recordedCallCount() == 0);
  assert(fixture.atoStartEvents.count == 0);
}

void testNormalLevelDoesNotStartPump() {
  Fixture fixture;
  enableAto(fixture);
  setWaterLevel(50, WaterLevelStatus::kNormal);

  const auto result = fixture.atoService.evaluateOnce();

  assert(result == AtoServiceResult::kSuccess);
  assertAto(true, AtoStatus::kNormal, false, 0, 0, 0);
  assert(fixture.relayController.recordedCallCount() == 0);
}

void testLowLevelStartsPumpAndEmitsAtoStart() {
  Fixture fixture;
  enableAto(fixture);
  fixture.timeSource.setUptimeMillis(10000);
  setWaterLevel(19, WaterLevelStatus::kNormal);

  const auto result = fixture.atoService.evaluateOnce();

  assert(result == AtoServiceResult::kSuccess);
  assert(fixture.relayController.recordedCallCount() == 1);
  assert(fixture.relayController.recordedCall(0).relay == ATO_PUMP);
  assert(fixture.relayController.recordedCall(0).desiredState ==
         RelayDesiredState::kOn);
  assertAto(true, AtoStatus::kRefilling, true, 10000, 0, 0);
  assertAtoPumpRelay(true, RelaySource::kAutomation);
  assert(reeflow::core::state::currentSystemState().ato.pumpRunning ==
         reeflow::core::state::currentSystemState().relays.atoPump.enabled);
  assert(fixture.atoStartEvents.count == 1);
}

void testModeGateBlocksAtoStartWithoutChangingConfigOrWaterLevel() {
  Fixture fixture;
  enableAto(fixture);
  fixture.timeSource.setUptimeMillis(10000);
  setWaterLevel(19, WaterLevelStatus::kNormal);
  blockAtoByMode(fixture);
  const AtoConfig atoConfigBefore = fixture.configManager.ato();
  const WaterLevelState waterLevelBefore =
      reeflow::core::state::currentSystemState().waterLevel;

  const auto result = fixture.atoService.evaluateOnce();

  assert(result == AtoServiceResult::kSuccess);
  assert(fixture.relayController.recordedCallCount() == 0);
  assertAto(false, AtoStatus::kDisabled, false, 0, 0, 0);
  assert(fixture.configManager.ato().enabled == atoConfigBefore.enabled);
  assert(fixture.configManager.ato().minimumLevel ==
         atoConfigBefore.minimumLevel);
  assert(reeflow::core::state::currentSystemState().waterLevel.currentLevel ==
         waterLevelBefore.currentLevel);
  assert(reeflow::core::state::currentSystemState().waterLevel.status ==
         waterLevelBefore.status);
  assert(fixture.atoStartEvents.count == 0);
}

void testMaximumLevelStopsPumpAndEmitsAtoStop() {
  Fixture fixture;
  enableAto(fixture);
  fixture.timeSource.setUptimeMillis(20000);
  setWaterLevel(80, WaterLevelStatus::kNormal);
  setAto(true, AtoStatus::kRefilling, true, 10000, 0, 0);
  setAtoPumpRelay(true, 10000, RelaySource::kAutomation);
  fixture.relayController.setInitialState(ATO_PUMP, RelayDesiredState::kOn);

  const auto result = fixture.atoService.evaluateOnce();

  assert(result == AtoServiceResult::kSuccess);
  assertAto(true, AtoStatus::kNormal, false, 10000, 20000, 0);
  assertAtoPumpRelay(false, RelaySource::kAutomation);
  assert(fixture.atoStopEvents.count == 1);
}

void testTimeoutStopsPumpByFailsafeAndEmitsAtoTimeout() {
  Fixture fixture;
  enableAto(fixture);
  fixture.timeSource.setUptimeMillis(70000);
  setWaterLevel(30, WaterLevelStatus::kNormal);
  setAto(true, AtoStatus::kRefilling, true, 10000, 0, 0);
  setAtoPumpRelay(true, 10000, RelaySource::kAutomation);
  fixture.relayController.setInitialState(ATO_PUMP, RelayDesiredState::kOn);

  const auto result = fixture.atoService.evaluateOnce();

  assert(result == AtoServiceResult::kSuccess);
  assertAto(true, AtoStatus::kTimeout, false, 10000, 70000, 1);
  assertAtoPumpRelay(false, RelaySource::kFailsafe);
  assert(fixture.atoTimeoutEvents.count == 1);
}

void testModeGateBlockedAtoTurnsRunningPumpOffByFailsafe() {
  Fixture fixture;
  enableAto(fixture);
  fixture.timeSource.setUptimeMillis(30000);
  setWaterLevel(30, WaterLevelStatus::kNormal);
  setAto(true, AtoStatus::kRefilling, true, 10000, 0, 0);
  setAtoPumpRelay(true, 10000, RelaySource::kAutomation);
  fixture.relayController.setInitialState(ATO_PUMP, RelayDesiredState::kOn);
  blockAtoByMode(fixture);
  const WaterLevelState waterLevelBefore =
      reeflow::core::state::currentSystemState().waterLevel;

  const auto result = fixture.atoService.evaluateOnce();

  assert(result == AtoServiceResult::kSuccess);
  assert(fixture.relayController.recordedCallCount() == 1);
  assert(fixture.relayController.recordedCall(0).relay == ATO_PUMP);
  assert(fixture.relayController.recordedCall(0).desiredState ==
         RelayDesiredState::kOff);
  assertAto(true, AtoStatus::kRefilling, false, 10000, 30000, 0);
  assertAtoPumpRelay(false, RelaySource::kFailsafe);
  assert(reeflow::core::state::currentSystemState().waterLevel.currentLevel ==
         waterLevelBefore.currentLevel);
  assert(fixture.atoStartEvents.count == 0);
  assert(fixture.atoStopEvents.count == 0);
  assert(fixture.atoTimeoutEvents.count == 0);
}

void testRepeatedTimeoutDoesNotDuplicateEventOrCounter() {
  Fixture fixture;
  enableAto(fixture);
  fixture.timeSource.setUptimeMillis(80000);
  setWaterLevel(19, WaterLevelStatus::kNormal);
  setAto(true, AtoStatus::kTimeout, false, 10000, 70000, 1);

  const auto result = fixture.atoService.evaluateOnce();

  assert(result == AtoServiceResult::kSuccess);
  assertAto(true, AtoStatus::kTimeout, false, 10000, 70000, 1);
  assert(fixture.atoTimeoutEvents.count == 0);
}

void testSensorOfflineUpdatesAtoAndEmitsEvent() {
  Fixture fixture;
  enableAto(fixture);
  setWaterLevel(50, WaterLevelStatus::kSensorOffline);

  const auto result = fixture.atoService.evaluateOnce();

  assert(result == AtoServiceResult::kSuccess);
  assertAto(true, AtoStatus::kSensorOffline, false, 0, 0, 0);
  assert(fixture.atoSensorOfflineEvents.count == 1);
}

void testSensorOfflineWithPumpOnUsesFailsafeOff() {
  Fixture fixture;
  enableAto(fixture);
  fixture.timeSource.setUptimeMillis(40000);
  setWaterLevel(50, WaterLevelStatus::kSensorOffline);
  setAto(true, AtoStatus::kRefilling, true, 10000, 0, 0);
  setAtoPumpRelay(true, 10000, RelaySource::kAutomation);
  fixture.relayController.setInitialState(ATO_PUMP, RelayDesiredState::kOn);

  const auto result = fixture.atoService.evaluateOnce();

  assert(result == AtoServiceResult::kSuccess);
  assertAto(true, AtoStatus::kSensorOffline, false, 10000, 40000, 0);
  assertAtoPumpRelay(false, RelaySource::kFailsafe);
  assert(fixture.atoSensorOfflineEvents.count == 1);
}

void testInvalidConfigWithPumpOnUsesFailsafeWithoutNewStatus() {
  Fixture fixture;
  AtoConfig config = enabledAtoConfig(fixture.configManager);
  config.maximumLevel = 101;
  assert(fixture.configManager.updateAto(config));
  setWaterLevel(50, WaterLevelStatus::kNormal);
  setAto(true, AtoStatus::kRefilling, true, 10000, 0, 0);
  setAtoPumpRelay(true, 10000, RelaySource::kAutomation);
  fixture.relayController.setInitialState(ATO_PUMP, RelayDesiredState::kOn);

  const auto result = fixture.atoService.evaluateOnce();

  assert(result == AtoServiceResult::kSuccess);
  assertAto(true, AtoStatus::kRefilling, false, 10000, 0, 0);
  assertAtoPumpRelay(false, RelaySource::kFailsafe);
}

void testPumpDivergenceIsReconciledByFailsafe() {
  Fixture fixture;
  enableAto(fixture);
  setWaterLevel(50, WaterLevelStatus::kNormal);
  setAto(true, AtoStatus::kNormal, false, 0, 0, 0);
  setAtoPumpRelay(true, 5000, RelaySource::kAutomation);
  fixture.relayController.setInitialState(ATO_PUMP, RelayDesiredState::kOn);

  const auto result = fixture.atoService.evaluateOnce();

  assert(result == AtoServiceResult::kSuccess);
  assertAto(true, AtoStatus::kNormal, false, 0, fixture.timeSource.uptimeMillis(),
            0);
  assertAtoPumpRelay(false, RelaySource::kFailsafe);
}

void testRecoveryFromTimeoutEmitsAtoRecovered() {
  Fixture fixture;
  enableAto(fixture);
  setWaterLevel(50, WaterLevelStatus::kNormal);
  setAto(true, AtoStatus::kTimeout, false, 10000, 70000, 1);

  const auto result = fixture.atoService.evaluateOnce();

  assert(result == AtoServiceResult::kSuccess);
  assertAto(true, AtoStatus::kNormal, false, 10000, 70000, 1);
  assert(fixture.atoRecoveredEvents.count == 1);
}

void testRecoveryFromSensorOfflineEmitsAtoRecovered() {
  Fixture fixture;
  enableAto(fixture);
  setWaterLevel(50, WaterLevelStatus::kNormal);
  setAto(true, AtoStatus::kSensorOffline, false, 0, 0, 0);

  const auto result = fixture.atoService.evaluateOnce();

  assert(result == AtoServiceResult::kSuccess);
  assertAto(true, AtoStatus::kNormal, false, 0, 0, 0);
  assert(fixture.atoRecoveredEvents.count == 1);
}

void testRelayFailureOnStartPreservesAtoState() {
  Fixture fixture;
  enableAto(fixture);
  setWaterLevel(20, WaterLevelStatus::kNormal);
  fixture.relayController.failRelay(ATO_PUMP);
  const AtoState before = reeflow::core::state::currentSystemState().ato;

  const auto result = fixture.atoService.evaluateOnce();

  assert(result == AtoServiceResult::kRelayCommandFailed);
  const AtoState& after = reeflow::core::state::currentSystemState().ato;
  assert(after.status == before.status);
  assert(after.pumpRunning == before.pumpRunning);
  assert(fixture.atoStartEvents.count == 0);
}

void testRelayFailureOnStopIsReportedAndPreservesAtoState() {
  Fixture fixture;
  enableAto(fixture);
  fixture.timeSource.setUptimeMillis(20000);
  setWaterLevel(80, WaterLevelStatus::kNormal);
  setAto(true, AtoStatus::kRefilling, true, 10000, 0, 0);
  setAtoPumpRelay(true, 10000, RelaySource::kAutomation);
  fixture.relayController.setInitialState(ATO_PUMP, RelayDesiredState::kOn);
  fixture.relayController.failRelay(ATO_PUMP);
  const AtoState before = reeflow::core::state::currentSystemState().ato;

  const auto result = fixture.atoService.evaluateOnce();

  assert(result == AtoServiceResult::kRelayCommandFailed);
  const AtoState& after = reeflow::core::state::currentSystemState().ato;
  assert(after.status == before.status);
  assert(after.pumpRunning == before.pumpRunning);
  assert(fixture.atoStopEvents.count == 0);
}

void testUnrelatedBlocksAreNotCorrupted() {
  Fixture fixture;
  enableAto(fixture);
  setWaterLevel(20, WaterLevelStatus::kNormal);
  const SystemState before = reeflow::core::state::currentSystemState();

  const auto result = fixture.atoService.evaluateOnce();

  assert(result == AtoServiceResult::kSuccess);
  assertUnrelatedBlocksMatch(before);
}

}  // namespace

int main() {
  testDisabledAtoUpdatesStatusWithoutPumpCommand();
  testNormalLevelDoesNotStartPump();
  testLowLevelStartsPumpAndEmitsAtoStart();
  testModeGateBlocksAtoStartWithoutChangingConfigOrWaterLevel();
  testMaximumLevelStopsPumpAndEmitsAtoStop();
  testTimeoutStopsPumpByFailsafeAndEmitsAtoTimeout();
  testModeGateBlockedAtoTurnsRunningPumpOffByFailsafe();
  testRepeatedTimeoutDoesNotDuplicateEventOrCounter();
  testSensorOfflineUpdatesAtoAndEmitsEvent();
  testSensorOfflineWithPumpOnUsesFailsafeOff();
  testInvalidConfigWithPumpOnUsesFailsafeWithoutNewStatus();
  testPumpDivergenceIsReconciledByFailsafe();
  testRecoveryFromTimeoutEmitsAtoRecovered();
  testRecoveryFromSensorOfflineEmitsAtoRecovered();
  testRelayFailureOnStartPreservesAtoState();
  testRelayFailureOnStopIsReportedAndPreservesAtoState();
  testUnrelatedBlocksAreNotCorrupted();
  return 0;
}
