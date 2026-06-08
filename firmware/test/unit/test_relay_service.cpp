#include <assert.h>

#include "core/events/event_bus.h"
#include "core/state/system_state.h"
#include "fakes/fake_relay_controller.h"
#include "fakes/fake_time_source.h"
#include "modules/relays/relay_service.h"
#include "modules/relays/relay_types.h"

namespace {

using reeflow::core::events::Event;
using reeflow::core::events::EventBus;
using reeflow::core::events::EventType;
using reeflow::core::state::AtoState;
using reeflow::core::state::AtoStatus;
using reeflow::core::state::RelaySource;
using reeflow::core::state::RelaysState;
using reeflow::core::state::SystemState;
using reeflow::modules::relays::RelayCommand;
using reeflow::modules::relays::RelayCommandResult;
using reeflow::modules::relays::RelayDesiredState;
using reeflow::modules::relays::RelayId;
using reeflow::modules::relays::RelayService;
using reeflow::test::fakes::FakeRelayController;
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
  FakeRelayController controller;
  FakeTimeSource timeSource;
  EventBus eventBus;
  RelayService service;
  EventRecorder relayOnEvents;
  EventRecorder relayOffEvents;

  Fixture()
      : service(controller, timeSource, eventBus),
        relayOnEvents({}),
        relayOffEvents({}) {
    reeflow::core::state::resetSystemState();
    eventBus.subscribe(EventType::kRelayOn, recordEvent, &relayOnEvents);
    eventBus.subscribe(EventType::kRelayOff, recordEvent, &relayOffEvents);
  }
};

void setRelayState(RelayId relay, bool enabled, uint32_t lastChanged,
                   RelaySource source) {
  RelaysState relays = reeflow::core::state::currentSystemState().relays;
  reeflow::core::state::RelayEntryState* entry = nullptr;

  switch (relay) {
    case RelayId::kRecalque:
      entry = &relays.recalque;
      break;
    case RelayId::kHeater:
      entry = &relays.heater;
      break;
    case RelayId::kAtoPump:
      entry = &relays.atoPump;
      break;
    case RelayId::kReserve:
      entry = &relays.reserve;
      break;
    case RelayId::kUnknown:
      break;
  }

  assert(entry != nullptr);
  entry->enabled = enabled;
  entry->lastChanged = lastChanged;
  entry->source = source;
  reeflow::core::state::updateRelaysState(relays);
}

void assertRelayEntry(RelayId relay, bool expectedEnabled,
                      uint32_t expectedLastChanged,
                      RelaySource expectedSource) {
  const RelaysState& relays =
      reeflow::core::state::currentSystemState().relays;
  const reeflow::core::state::RelayEntryState* entry = nullptr;

  switch (relay) {
    case RelayId::kRecalque:
      entry = &relays.recalque;
      break;
    case RelayId::kHeater:
      entry = &relays.heater;
      break;
    case RelayId::kAtoPump:
      entry = &relays.atoPump;
      break;
    case RelayId::kReserve:
      entry = &relays.reserve;
      break;
    case RelayId::kUnknown:
      break;
  }

  assert(entry != nullptr);
  assert(entry->enabled == expectedEnabled);
  assert(entry->lastChanged == expectedLastChanged);
  assert(entry->source == expectedSource);
}

void assertUnrelatedBlocksMatch(const SystemState& before) {
  const SystemState& after = reeflow::core::state::currentSystemState();

  assert(after.temperature.currentTemperature ==
         before.temperature.currentTemperature);
  assert(after.temperature.status == before.temperature.status);
  assert(after.waterLevel.currentLevel == before.waterLevel.currentLevel);
  assert(after.waterLevel.status == before.waterLevel.status);
  assert(after.lighting.mode == before.lighting.mode);
  assert(after.modes.currentMode == before.modes.currentMode);
  assert(after.ato.enabled == before.ato.enabled);
  assert(after.ato.status == before.ato.status);
  assert(after.ato.pumpRunning == before.ato.pumpRunning);
  assert(after.ato.lastActivation == before.ato.lastActivation);
  assert(after.ato.lastCompletion == before.ato.lastCompletion);
  assert(after.ato.timeoutCounter == before.ato.timeoutCounter);
  assert(after.network.wifiConnected == before.network.wifiConnected);
  assert(after.alerts.activeAlertCount == before.alerts.activeAlertCount);
  assert(after.systemHealth.uptime == before.systemHealth.uptime);
}

void testLocalOnUpdatesOnlyTargetRelayAfterControllerSuccess() {
  Fixture fixture;
  fixture.timeSource.setUptimeMillis(1234);

  const auto result =
      fixture.service.setLocalRelay(RelayId::kRecalque, RelayDesiredState::kOn);

  assert(result == RelayCommandResult::kSuccess);
  assert(fixture.controller.recordedCallCount() == 1);
  assert(fixture.controller.recordedCall(0).relay == RelayId::kRecalque);
  assert(fixture.controller.recordedCall(0).desiredState ==
         RelayDesiredState::kOn);
  assertRelayEntry(RelayId::kRecalque, true, 1234, RelaySource::kLocal);
  assertRelayEntry(RelayId::kHeater, false, 0, RelaySource::kLocal);
  assertRelayEntry(RelayId::kAtoPump, false, 0, RelaySource::kLocal);
  assertRelayEntry(RelayId::kReserve, false, 0, RelaySource::kLocal);
  assert(fixture.relayOnEvents.count == 1);
  assert(fixture.relayOnEvents.events[0].type == EventType::kRelayOn);
  assert(fixture.relayOffEvents.count == 0);
}

void testLocalOffUpdatesTargetRelayAfterControllerSuccess() {
  Fixture fixture;
  setRelayState(RelayId::kHeater, true, 100, RelaySource::kLocal);
  fixture.timeSource.setUptimeMillis(200);

  const auto result =
      fixture.service.setLocalRelay(RelayId::kHeater, RelayDesiredState::kOff);

  assert(result == RelayCommandResult::kSuccess);
  assert(fixture.controller.recordedCallCount() == 1);
  assert(fixture.controller.recordedCall(0).relay == RelayId::kHeater);
  assert(fixture.controller.recordedCall(0).desiredState ==
         RelayDesiredState::kOff);
  assertRelayEntry(RelayId::kHeater, false, 200, RelaySource::kLocal);
  assert(fixture.relayOnEvents.count == 0);
  assert(fixture.relayOffEvents.count == 1);
  assert(fixture.relayOffEvents.events[0].type == EventType::kRelayOff);
}

void testControllerFailurePreservesSystemState() {
  Fixture fixture;
  setRelayState(RelayId::kReserve, false, 70, RelaySource::kLocal);
  fixture.controller.failRelay(RelayId::kReserve);
  fixture.timeSource.setUptimeMillis(300);

  const auto result =
      fixture.service.setLocalRelay(RelayId::kReserve, RelayDesiredState::kOn);

  assert(result == RelayCommandResult::kControllerFailure);
  assertRelayEntry(RelayId::kReserve, false, 70, RelaySource::kLocal);
  assert(fixture.relayOnEvents.count == 0);
  assert(fixture.relayOffEvents.count == 0);
}

void testUnknownRelayIsRejectedWithoutStateChange() {
  Fixture fixture;
  const SystemState before = reeflow::core::state::currentSystemState();

  const auto result =
      fixture.service.setLocalRelay(RelayId::kUnknown, RelayDesiredState::kOn);

  assert(result == RelayCommandResult::kUnknownRelay);
  assert(fixture.controller.recordedCallCount() == 0);
  assertUnrelatedBlocksMatch(before);
  assert(!reeflow::core::state::currentSystemState().relays.recalque.enabled);
  assert(fixture.relayOnEvents.count == 0);
}

void testIdempotentCommandDoesNotCallControllerOrUpdateMetadata() {
  Fixture fixture;
  setRelayState(RelayId::kRecalque, true, 111, RelaySource::kLocal);
  fixture.timeSource.setUptimeMillis(999);

  const RelayCommand command = {
      RelayId::kRecalque,
      RelayDesiredState::kOn,
      RelaySource::kFailsafe,
  };
  const auto result = fixture.service.applyCommand(command);

  assert(result == RelayCommandResult::kSuccess);
  assert(fixture.controller.recordedCallCount() == 0);
  assertRelayEntry(RelayId::kRecalque, true, 111, RelaySource::kLocal);
  assert(fixture.relayOnEvents.count == 0);
  assert(fixture.relayOffEvents.count == 0);
}

void testRemoteSourcesAreRejectedAsInputsForThisPhase() {
  Fixture fixture;
  const RelayCommand command = {
      RelayId::kRecalque,
      RelayDesiredState::kOn,
      RelaySource::kMqtt,
  };

  const auto result = fixture.service.applyCommand(command);

  assert(result == RelayCommandResult::kInvalidContract);
  assert(fixture.controller.recordedCallCount() == 0);
  assertRelayEntry(RelayId::kRecalque, false, 0, RelaySource::kLocal);

  const RelayCommand appCommand = {
      RelayId::kRecalque,
      RelayDesiredState::kOn,
      RelaySource::kApp,
  };
  const auto appResult = fixture.service.applyCommand(appCommand);

  assert(appResult == RelayCommandResult::kInvalidContract);
  assert(fixture.controller.recordedCallCount() == 0);
  assertRelayEntry(RelayId::kRecalque, false, 0, RelaySource::kLocal);
}

void testAutomationSourceIsAcceptedForLocalFirmwareCommand() {
  Fixture fixture;
  fixture.timeSource.setUptimeMillis(444);
  const RelayCommand command = {
      RelayId::kAtoPump,
      RelayDesiredState::kOn,
      RelaySource::kAutomation,
  };

  const auto result = fixture.service.applyCommand(command);

  assert(result == RelayCommandResult::kSuccess);
  assert(fixture.controller.recordedCallCount() == 1);
  assert(fixture.controller.recordedCall(0).relay == RelayId::kAtoPump);
  assertRelayEntry(RelayId::kAtoPump, true, 444,
                   RelaySource::kAutomation);
  assert(fixture.relayOnEvents.count == 1);
}

void testAllOffCommandsControllerAndUpdatesOnlyEnabledRelays() {
  Fixture fixture;
  setRelayState(RelayId::kRecalque, true, 10, RelaySource::kLocal);
  setRelayState(RelayId::kHeater, false, 20, RelaySource::kLocal);
  setRelayState(RelayId::kAtoPump, true, 30, RelaySource::kLocal);
  setRelayState(RelayId::kReserve, true, 40, RelaySource::kLocal);
  fixture.timeSource.setUptimeMillis(500);

  const auto result = fixture.service.allOff(RelaySource::kFailsafe);

  assert(result == RelayCommandResult::kSuccess);
  assert(fixture.controller.allOffCallCount() == 1);
  assertRelayEntry(RelayId::kRecalque, false, 500, RelaySource::kFailsafe);
  assertRelayEntry(RelayId::kHeater, false, 20, RelaySource::kLocal);
  assertRelayEntry(RelayId::kAtoPump, false, 500, RelaySource::kFailsafe);
  assertRelayEntry(RelayId::kReserve, false, 500, RelaySource::kFailsafe);
  assert(fixture.relayOnEvents.count == 0);
  assert(fixture.relayOffEvents.count == 3);
}

void testAllOffWithAllRelaysAlreadyOffDoesNotPublishRelayEvents() {
  Fixture fixture;
  fixture.timeSource.setUptimeMillis(600);

  const auto result = fixture.service.allOff(RelaySource::kFailsafe);

  assert(result == RelayCommandResult::kSuccess);
  assert(fixture.controller.allOffCallCount() == 1);
  assertRelayEntry(RelayId::kRecalque, false, 0, RelaySource::kLocal);
  assertRelayEntry(RelayId::kHeater, false, 0, RelaySource::kLocal);
  assertRelayEntry(RelayId::kAtoPump, false, 0, RelaySource::kLocal);
  assertRelayEntry(RelayId::kReserve, false, 0, RelaySource::kLocal);
  assert(fixture.relayOffEvents.count == 0);
}

void testAllOffControllerFailurePreservesState() {
  Fixture fixture;
  setRelayState(RelayId::kRecalque, true, 10, RelaySource::kLocal);
  fixture.controller.setAllOffResult(
      RelayCommandResult::kControllerFailure);

  const auto result = fixture.service.allOff(RelaySource::kFailsafe);

  assert(result == RelayCommandResult::kControllerFailure);
  assertRelayEntry(RelayId::kRecalque, true, 10, RelaySource::kLocal);
  assert(fixture.relayOffEvents.count == 0);
}

void testManualAtoRelayControlDoesNotAlterAtoBlock() {
  Fixture fixture;
  AtoState ato = reeflow::core::state::currentSystemState().ato;
  ato.enabled = true;
  ato.status = AtoStatus::kNormal;
  ato.pumpRunning = false;
  ato.lastActivation = 11;
  ato.lastCompletion = 22;
  ato.timeoutCounter = 3;
  reeflow::core::state::updateAtoState(ato);
  const AtoState beforeAto = reeflow::core::state::currentSystemState().ato;
  fixture.timeSource.setUptimeMillis(700);

  const auto result =
      fixture.service.setLocalRelay(RelayId::kAtoPump, RelayDesiredState::kOn);

  assert(result == RelayCommandResult::kSuccess);
  assertRelayEntry(RelayId::kAtoPump, true, 700, RelaySource::kLocal);
  const AtoState& afterAto = reeflow::core::state::currentSystemState().ato;
  assert(afterAto.enabled == beforeAto.enabled);
  assert(afterAto.status == beforeAto.status);
  assert(afterAto.pumpRunning == beforeAto.pumpRunning);
  assert(afterAto.lastActivation == beforeAto.lastActivation);
  assert(afterAto.lastCompletion == beforeAto.lastCompletion);
  assert(afterAto.timeoutCounter == beforeAto.timeoutCounter);
}

void testUnrelatedBlocksAreNotChangedByRelayCommand() {
  Fixture fixture;
  const SystemState before = reeflow::core::state::currentSystemState();
  fixture.timeSource.setUptimeMillis(800);

  const auto result =
      fixture.service.setLocalRelay(RelayId::kReserve, RelayDesiredState::kOn);

  assert(result == RelayCommandResult::kSuccess);
  assertUnrelatedBlocksMatch(before);
}

}  // namespace

int main() {
  testLocalOnUpdatesOnlyTargetRelayAfterControllerSuccess();
  testLocalOffUpdatesTargetRelayAfterControllerSuccess();
  testControllerFailurePreservesSystemState();
  testUnknownRelayIsRejectedWithoutStateChange();
  testIdempotentCommandDoesNotCallControllerOrUpdateMetadata();
  testRemoteSourcesAreRejectedAsInputsForThisPhase();
  testAutomationSourceIsAcceptedForLocalFirmwareCommand();
  testAllOffCommandsControllerAndUpdatesOnlyEnabledRelays();
  testAllOffWithAllRelaysAlreadyOffDoesNotPublishRelayEvents();
  testAllOffControllerFailurePreservesState();
  testManualAtoRelayControlDoesNotAlterAtoBlock();
  testUnrelatedBlocksAreNotChangedByRelayCommand();
  return 0;
}
