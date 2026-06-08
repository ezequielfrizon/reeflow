#include <assert.h>

#include "core/events/event_bus.h"
#include "core/state/system_state.h"
#include "fakes/fake_relay_controller.h"
#include "fakes/fake_time_source.h"
#include "modules/modes/mode_automation_gate.h"
#include "modules/modes/mode_effects.h"
#include "modules/relays/relay_service.h"

namespace {

using reeflow::core::events::EventBus;
using reeflow::core::state::RelaySource;
using reeflow::modules::modes::FEEDING;
using reeflow::modules::modes::MAINTENANCE;
using reeflow::modules::modes::NORMAL;
using reeflow::modules::modes::TPA;
using reeflow::modules::modes::LocalModeAutomationGate;
using reeflow::modules::modes::ModeAutomation;
using reeflow::modules::modes::ModeEffectResult;
using reeflow::modules::modes::ModeEffectsRequest;
using reeflow::modules::modes::RelayModeEffects;
using reeflow::modules::relays::ATO_PUMP;
using reeflow::modules::relays::RECALQUE;
using reeflow::modules::relays::RelayDesiredState;
using reeflow::modules::relays::RelayService;
using reeflow::test::fakes::FakeRelayController;
using reeflow::test::fakes::FakeTimeSource;

struct Fixture {
  FakeRelayController relayController;
  FakeTimeSource timeSource;
  EventBus eventBus;
  RelayService relayService;
  LocalModeAutomationGate gate;
  RelayModeEffects effects;

  Fixture()
      : relayService(relayController, timeSource, eventBus),
        effects(relayService, gate) {
    reeflow::core::state::resetSystemState();
    reeflow::core::state::setSystemStateEventBus(eventBus);
    timeSource.setUptimeMillis(1000);
  }
};

ModeEffectsRequest requestFor(reeflow::modules::modes::OperationalMode mode) {
  return reeflow::modules::modes::makeNoOpModeEffectsRequest(mode, 1000);
}

void setRelayOn(Fixture& fixture, reeflow::modules::relays::RelayId relay) {
  fixture.relayController.setInitialState(relay, RelayDesiredState::kOn);
  auto relays = reeflow::core::state::currentSystemState().relays;
  if (relay == RECALQUE) {
    relays.recalque.enabled = true;
    relays.recalque.source = RelaySource::kAutomation;
  } else if (relay == ATO_PUMP) {
    relays.atoPump.enabled = true;
    relays.atoPump.source = RelaySource::kAutomation;
  }
  reeflow::core::state::updateRelaysState(relays);
}

void testFeedingTurnsOffRecalqueWithAutomationSource() {
  Fixture fixture;
  setRelayOn(fixture, RECALQUE);
  ModeEffectsRequest request = requestFor(FEEDING);
  request.turnOffRecalque = true;
  request.updateAtoAutomationGate = true;
  request.blockAtoAutomation = true;

  const auto result = fixture.effects.applyModeEffects(request);

  assert(result == ModeEffectResult::kSuccess);
  assert(fixture.relayController.recordedCallCount() == 1);
  assert(fixture.relayController.recordedCall(0).relay == RECALQUE);
  assert(fixture.relayController.recordedCall(0).desiredState ==
         RelayDesiredState::kOff);
  assert(!reeflow::core::state::currentSystemState().relays.recalque.enabled);
  assert(reeflow::core::state::currentSystemState().relays.recalque.source ==
         RelaySource::kAutomation);
  assert(!fixture.gate.isAutomationAllowed(ModeAutomation::kAto));
}

void testTpaTurnsOffRecalqueAndAtoPumpWithSafeSources() {
  Fixture fixture;
  setRelayOn(fixture, RECALQUE);
  setRelayOn(fixture, ATO_PUMP);
  ModeEffectsRequest request = requestFor(TPA);
  request.turnOffRecalque = true;
  request.turnOffAtoPump = true;
  request.updateAtoAutomationGate = true;
  request.blockAtoAutomation = true;

  const auto result = fixture.effects.applyModeEffects(request);

  assert(result == ModeEffectResult::kSuccess);
  assert(fixture.relayController.recordedCallCount() == 2);
  assert(fixture.relayController.recordedCall(0).relay == RECALQUE);
  assert(fixture.relayController.recordedCall(1).relay == ATO_PUMP);
  assert(reeflow::core::state::currentSystemState().relays.recalque.source ==
         RelaySource::kAutomation);
  assert(reeflow::core::state::currentSystemState().relays.atoPump.source ==
         RelaySource::kFailsafe);
  assert(!fixture.gate.isAutomationAllowed(ModeAutomation::kAto));
}

void testNormalReleasesGateWithoutTurningRelaysOn() {
  Fixture fixture;
  assert(fixture.gate.setAutomationBlocked(ModeAutomation::kAto, true, TPA) ==
         reeflow::modules::modes::ModeAutomationGateResult::kSuccess);
  ModeEffectsRequest request = requestFor(NORMAL);
  request.releaseModeBlocks = true;

  const auto result = fixture.effects.applyModeEffects(request);

  assert(result == ModeEffectResult::kSuccess);
  assert(fixture.gate.isAutomationAllowed(ModeAutomation::kAto));
  assert(fixture.relayController.recordedCallCount() == 0);
}

void testMaintenanceCanBlockAtoWithoutRelayCommand() {
  Fixture fixture;
  ModeEffectsRequest request = requestFor(MAINTENANCE);
  request.preserveManualRelayControl = true;
  request.updateAtoAutomationGate = true;
  request.blockAtoAutomation = true;

  const auto result = fixture.effects.applyModeEffects(request);

  assert(result == ModeEffectResult::kSuccess);
  assert(!fixture.gate.isAutomationAllowed(ModeAutomation::kAto));
  assert(fixture.relayController.recordedCallCount() == 0);
}

void testRelayFailureIsPropagated() {
  Fixture fixture;
  setRelayOn(fixture, RECALQUE);
  fixture.relayController.failRelay(RECALQUE);
  ModeEffectsRequest request = requestFor(FEEDING);
  request.turnOffRecalque = true;
  request.updateAtoAutomationGate = true;
  request.blockAtoAutomation = true;

  const auto result = fixture.effects.applyModeEffects(request);

  assert(result == ModeEffectResult::kFailed);
  assert(reeflow::core::state::currentSystemState().relays.recalque.enabled);
  assert(fixture.gate.isAutomationAllowed(ModeAutomation::kAto));
}

}  // namespace

int main() {
  testFeedingTurnsOffRecalqueWithAutomationSource();
  testTpaTurnsOffRecalqueAndAtoPumpWithSafeSources();
  testNormalReleasesGateWithoutTurningRelaysOn();
  testMaintenanceCanBlockAtoWithoutRelayCommand();
  testRelayFailureIsPropagated();
  return 0;
}
