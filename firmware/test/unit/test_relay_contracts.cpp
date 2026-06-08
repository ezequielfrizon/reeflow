#include <assert.h>
#include <string.h>

#include "core/state/system_state.h"
#include "fakes/fake_relay_controller.h"
#include "modules/relays/relay_controller.h"
#include "modules/relays/relay_types.h"

namespace {

using reeflow::core::state::RelaySource;
using reeflow::modules::relays::ATO_PUMP;
using reeflow::modules::relays::HEATER;
using reeflow::modules::relays::RECALQUE;
using reeflow::modules::relays::RESERVE;
using reeflow::modules::relays::RelayCommandResult;
using reeflow::modules::relays::RelayDesiredState;
using reeflow::modules::relays::RelayId;
using reeflow::modules::relays::isKnownRelay;
using reeflow::modules::relays::isLocalFirmwareRelayCommandSource;
using reeflow::modules::relays::makeAutomationRelayCommand;
using reeflow::modules::relays::makeFailsafeRelayCommand;
using reeflow::modules::relays::makeLocalRelayCommand;
using reeflow::modules::relays::relayDisplayName;
using reeflow::modules::relays::relaySystemStateFieldName;
using reeflow::test::fakes::FakeRelayController;

void testFunctionalRelayIdentifiersExist() {
  assert(RECALQUE == RelayId::kRecalque);
  assert(HEATER == RelayId::kHeater);
  assert(ATO_PUMP == RelayId::kAtoPump);
  assert(RESERVE == RelayId::kReserve);
  assert(isKnownRelay(RECALQUE));
  assert(isKnownRelay(HEATER));
  assert(isKnownRelay(ATO_PUMP));
  assert(isKnownRelay(RESERVE));
  assert(!isKnownRelay(RelayId::kUnknown));
}

void testFunctionalRelayIdentifiersMapToSystemStateFields() {
  assert(strcmp(relaySystemStateFieldName(RelayId::kRecalque),
                "relays.recalque") == 0);
  assert(strcmp(relaySystemStateFieldName(RelayId::kHeater),
                "relays.heater") == 0);
  assert(strcmp(relaySystemStateFieldName(RelayId::kAtoPump),
                "relays.atoPump") == 0);
  assert(strcmp(relaySystemStateFieldName(RelayId::kReserve),
                "relays.reserve") == 0);

  assert(strcmp(relayDisplayName(RelayId::kRecalque), "Recalque") == 0);
  assert(strcmp(relayDisplayName(RelayId::kHeater), "Aquecedor") == 0);
  assert(strcmp(relayDisplayName(RelayId::kAtoPump), "ATO") == 0);
  assert(strcmp(relayDisplayName(RelayId::kReserve), "Reserva") == 0);
}

void testRelayCommandContractAllowsOnOffAndLocalSources() {
  const auto localOn =
      makeLocalRelayCommand(RelayId::kRecalque, RelayDesiredState::kOn);
  const auto failsafeOff =
      makeFailsafeRelayCommand(RelayId::kHeater, RelayDesiredState::kOff);

  assert(localOn.relay == RelayId::kRecalque);
  assert(localOn.desiredState == RelayDesiredState::kOn);
  assert(localOn.source == RelaySource::kLocal);

  assert(failsafeOff.relay == RelayId::kHeater);
  assert(failsafeOff.desiredState == RelayDesiredState::kOff);
  assert(failsafeOff.source == RelaySource::kFailsafe);

  const auto automationOn =
      makeAutomationRelayCommand(RelayId::kAtoPump, RelayDesiredState::kOn);
  assert(automationOn.relay == RelayId::kAtoPump);
  assert(automationOn.desiredState == RelayDesiredState::kOn);
  assert(automationOn.source == RelaySource::kAutomation);
}

void testLocalFirmwareRelayCommandsAllowAutomationButNotRemoteSources() {
  assert(isLocalFirmwareRelayCommandSource(RelaySource::kLocal));
  assert(isLocalFirmwareRelayCommandSource(RelaySource::kAutomation));
  assert(isLocalFirmwareRelayCommandSource(RelaySource::kFailsafe));
  assert(!isLocalFirmwareRelayCommandSource(RelaySource::kMqtt));
  assert(!isLocalFirmwareRelayCommandSource(RelaySource::kApp));
}

void testFakeRecordsOnCommand() {
  FakeRelayController controller;

  const auto result =
      controller.setRelay(RelayId::kRecalque, RelayDesiredState::kOn);

  assert(result == RelayCommandResult::kSuccess);
  assert(controller.state(RelayId::kRecalque) == RelayDesiredState::kOn);
  assert(controller.recordedCallCount() == 1);
  assert(controller.recordedCall(0).relay == RelayId::kRecalque);
  assert(controller.recordedCall(0).desiredState == RelayDesiredState::kOn);
}

void testFakeRecordsOffCommand() {
  FakeRelayController controller;
  controller.setInitialState(RelayId::kHeater, RelayDesiredState::kOn);

  const auto result =
      controller.setRelay(RelayId::kHeater, RelayDesiredState::kOff);

  assert(result == RelayCommandResult::kSuccess);
  assert(controller.state(RelayId::kHeater) == RelayDesiredState::kOff);
  assert(controller.recordedCallCount() == 1);
  assert(controller.recordedCall(0).relay == RelayId::kHeater);
  assert(controller.recordedCall(0).desiredState == RelayDesiredState::kOff);
}

void testFakeRecordsCommandSequence() {
  FakeRelayController controller;

  assert(controller.setRelay(RelayId::kRecalque, RelayDesiredState::kOn) ==
         RelayCommandResult::kSuccess);
  assert(controller.setRelay(RelayId::kAtoPump, RelayDesiredState::kOn) ==
         RelayCommandResult::kSuccess);
  assert(controller.setRelay(RelayId::kRecalque, RelayDesiredState::kOff) ==
         RelayCommandResult::kSuccess);

  assert(controller.recordedCallCount() == 3);
  assert(controller.recordedCall(0).relay == RelayId::kRecalque);
  assert(controller.recordedCall(1).relay == RelayId::kAtoPump);
  assert(controller.recordedCall(2).relay == RelayId::kRecalque);
  assert(controller.recordedCall(2).desiredState == RelayDesiredState::kOff);
}

void testFakeRecordsAllOff() {
  FakeRelayController controller;
  controller.setInitialState(RelayId::kRecalque, RelayDesiredState::kOn);
  controller.setInitialState(RelayId::kHeater, RelayDesiredState::kOn);
  controller.setInitialState(RelayId::kAtoPump, RelayDesiredState::kOn);
  controller.setInitialState(RelayId::kReserve, RelayDesiredState::kOn);

  const auto result = controller.allOff();

  assert(result == RelayCommandResult::kSuccess);
  assert(controller.allOffCallCount() == 1);
  assert(controller.recordedCallCount() == 1);
  assert(controller.recordedCall(0).type ==
         FakeRelayController::RecordedCallType::kAllOff);
  assert(controller.state(RelayId::kRecalque) == RelayDesiredState::kOff);
  assert(controller.state(RelayId::kHeater) == RelayDesiredState::kOff);
  assert(controller.state(RelayId::kAtoPump) == RelayDesiredState::kOff);
  assert(controller.state(RelayId::kReserve) == RelayDesiredState::kOff);
}

void testFakeSimulatesFailureForSpecificRelay() {
  FakeRelayController controller;
  controller.failRelay(RelayId::kReserve);

  const auto result =
      controller.setRelay(RelayId::kReserve, RelayDesiredState::kOn);

  assert(result == RelayCommandResult::kControllerFailure);
  assert(controller.state(RelayId::kReserve) == RelayDesiredState::kOff);
  assert(controller.recordedCallCount() == 1);
}

void testFakeRejectsUnknownRelay() {
  FakeRelayController controller;

  const auto result =
      controller.setRelay(RelayId::kUnknown, RelayDesiredState::kOn);

  assert(result == RelayCommandResult::kUnknownRelay);
  assert(controller.recordedCallCount() == 1);
}

void testFakeCanReturnInvalidContract() {
  FakeRelayController controller;
  controller.setRelayResult(RelayId::kAtoPump,
                            RelayCommandResult::kInvalidContract);

  const auto result =
      controller.setRelay(RelayId::kAtoPump, RelayDesiredState::kOn);

  assert(result == RelayCommandResult::kInvalidContract);
  assert(controller.state(RelayId::kAtoPump) == RelayDesiredState::kOff);
}

}  // namespace

int main() {
  testFunctionalRelayIdentifiersExist();
  testFunctionalRelayIdentifiersMapToSystemStateFields();
  testRelayCommandContractAllowsOnOffAndLocalSources();
  testLocalFirmwareRelayCommandsAllowAutomationButNotRemoteSources();
  testFakeRecordsOnCommand();
  testFakeRecordsOffCommand();
  testFakeRecordsCommandSequence();
  testFakeRecordsAllOff();
  testFakeSimulatesFailureForSpecificRelay();
  testFakeRejectsUnknownRelay();
  testFakeCanReturnInvalidContract();
  return 0;
}
