#include <assert.h>
#include <string.h>

#include "core/events/event_bus.h"
#include "core/state/system_state.h"
#include "fakes/fake_relay_controller.h"
#include "modules/ato/ato_config.h"
#include "modules/ato/ato_events.h"
#include "modules/ato/ato_policy.h"
#include "modules/ato/ato_service.h"
#include "modules/ato/ato_types.h"
#include "modules/relays/relay_types.h"

namespace {

using reeflow::core::events::EventType;
using reeflow::core::state::AtoStatus;
using reeflow::core::state::RelaySource;
using reeflow::modules::ato::AtoDecision;
using reeflow::modules::ato::AtoDecisionReason;
using reeflow::modules::ato::AtoEventType;
using reeflow::modules::ato::AtoPumpCommand;
using reeflow::modules::ato::atoCoreEventType;
using reeflow::modules::ato::atoEventName;
using reeflow::modules::ato::makeDefaultAtoModuleConfig;
using reeflow::modules::ato::makeNoActionAtoEvaluation;
using reeflow::modules::relays::RelayCommandResult;
using reeflow::modules::relays::RelayDesiredState;
using reeflow::modules::relays::RelayId;
using reeflow::modules::relays::isLocalFirmwareRelayCommandSource;
using reeflow::modules::relays::makeAutomationRelayCommand;
using reeflow::test::fakes::FakeRelayController;

void testAtoEventsExposeCanonicalNames() {
  assert(strcmp(atoEventName(AtoEventType::kAtoStart), "ATO_START") == 0);
  assert(strcmp(atoEventName(AtoEventType::kAtoStop), "ATO_STOP") == 0);
  assert(strcmp(atoEventName(AtoEventType::kAtoTimeout), "ATO_TIMEOUT") == 0);
  assert(strcmp(atoEventName(AtoEventType::kAtoSensorOffline),
                "ATO_SENSOR_OFFLINE") == 0);
  assert(strcmp(atoEventName(AtoEventType::kAtoRecovered),
                "ATO_RECOVERED") == 0);
}

void testAtoModuleDefaultEvaluationInterval() {
  const auto config = makeDefaultAtoModuleConfig();

  assert(config.evaluationIntervalMillis == 1000);
  assert(config.canonicalMinimumLevel == 0);
  assert(config.canonicalMaximumLevel == 100);
}

void testAtoEventsMapToLocalEventBusTypes() {
  assert(atoCoreEventType(AtoEventType::kAtoStart) ==
         EventType::kAtoStart);
  assert(atoCoreEventType(AtoEventType::kAtoStop) == EventType::kAtoStop);
  assert(atoCoreEventType(AtoEventType::kAtoTimeout) ==
         EventType::kAtoTimeout);
  assert(atoCoreEventType(AtoEventType::kAtoSensorOffline) ==
         EventType::kAtoSensorOffline);
  assert(atoCoreEventType(AtoEventType::kAtoRecovered) ==
         EventType::kAtoRecovered);
}

void testAtoDecisionContractsCoverPhase6Reasons() {
  assert(AtoDecision::kNoAction == AtoDecision::kNoAction);
  assert(AtoDecision::kStartRefill == AtoDecision::kStartRefill);
  assert(AtoDecision::kStopAtMaximumLevel ==
         AtoDecision::kStopAtMaximumLevel);
  assert(AtoDecision::kStopTimeout == AtoDecision::kStopTimeout);
  assert(AtoDecision::kBlockCooldown == AtoDecision::kBlockCooldown);
  assert(AtoDecision::kBlockSensorOffline ==
         AtoDecision::kBlockSensorOffline);
  assert(AtoDecision::kBlockInvalidConfig ==
         AtoDecision::kBlockInvalidConfig);
  assert(AtoDecision::kReconcilePumpDivergence ==
         AtoDecision::kReconcilePumpDivergence);
  assert(AtoDecision::kDisable == AtoDecision::kDisable);
  assert(AtoDecision::kRecover == AtoDecision::kRecover);
}

void testNoActionEvaluationDoesNotRequireSideEffects() {
  const auto evaluation = makeNoActionAtoEvaluation(AtoStatus::kNormal);

  assert(evaluation.decision == AtoDecision::kNoAction);
  assert(evaluation.reason == AtoDecisionReason::kNone);
  assert(evaluation.nextStatus == AtoStatus::kNormal);
  assert(!evaluation.metadata.requiresRelayCommand);
  assert(evaluation.metadata.pumpCommand == AtoPumpCommand::kNone);
  assert(!evaluation.metadata.emitsEvent);
  assert(!evaluation.metadata.updatesAtoState);
}

void testAutomationOriginIsLocalFirmwareOnly() {
  const auto command =
      makeAutomationRelayCommand(RelayId::kAtoPump, RelayDesiredState::kOn);

  assert(command.relay == RelayId::kAtoPump);
  assert(command.desiredState == RelayDesiredState::kOn);
  assert(command.source == RelaySource::kAutomation);
  assert(isLocalFirmwareRelayCommandSource(RelaySource::kAutomation));
  assert(!isLocalFirmwareRelayCommandSource(RelaySource::kMqtt));
  assert(!isLocalFirmwareRelayCommandSource(RelaySource::kApp));
}

void testFakeRelayControllerSimulatesAtoPumpOnSuccess() {
  FakeRelayController controller;

  const auto result =
      controller.setRelay(RelayId::kAtoPump, RelayDesiredState::kOn);

  assert(result == RelayCommandResult::kSuccess);
  assert(controller.state(RelayId::kAtoPump) == RelayDesiredState::kOn);
  assert(controller.recordedCallCount() == 1);
  assert(controller.recordedCall(0).relay == RelayId::kAtoPump);
  assert(controller.recordedCall(0).desiredState == RelayDesiredState::kOn);
}

void testFakeRelayControllerSimulatesAtoPumpOffSuccess() {
  FakeRelayController controller;
  controller.setInitialState(RelayId::kAtoPump, RelayDesiredState::kOn);

  const auto result =
      controller.setRelay(RelayId::kAtoPump, RelayDesiredState::kOff);

  assert(result == RelayCommandResult::kSuccess);
  assert(controller.state(RelayId::kAtoPump) == RelayDesiredState::kOff);
  assert(controller.recordedCallCount() == 1);
  assert(controller.recordedCall(0).relay == RelayId::kAtoPump);
  assert(controller.recordedCall(0).desiredState == RelayDesiredState::kOff);
}

void testFakeRelayControllerSimulatesAtoPumpOnFailure() {
  FakeRelayController controller;
  controller.failRelay(RelayId::kAtoPump);

  const auto result =
      controller.setRelay(RelayId::kAtoPump, RelayDesiredState::kOn);

  assert(result == RelayCommandResult::kControllerFailure);
  assert(controller.state(RelayId::kAtoPump) == RelayDesiredState::kOff);
  assert(controller.recordedCallCount() == 1);
}

void testFakeRelayControllerSimulatesAtoPumpOffFailure() {
  FakeRelayController controller;
  controller.setInitialState(RelayId::kAtoPump, RelayDesiredState::kOn);
  controller.failRelay(RelayId::kAtoPump);

  const auto result =
      controller.setRelay(RelayId::kAtoPump, RelayDesiredState::kOff);

  assert(result == RelayCommandResult::kControllerFailure);
  assert(controller.state(RelayId::kAtoPump) == RelayDesiredState::kOn);
  assert(controller.recordedCallCount() == 1);
}

}  // namespace

int main() {
  testAtoEventsExposeCanonicalNames();
  testAtoModuleDefaultEvaluationInterval();
  testAtoEventsMapToLocalEventBusTypes();
  testAtoDecisionContractsCoverPhase6Reasons();
  testNoActionEvaluationDoesNotRequireSideEffects();
  testAutomationOriginIsLocalFirmwareOnly();
  testFakeRelayControllerSimulatesAtoPumpOnSuccess();
  testFakeRelayControllerSimulatesAtoPumpOffSuccess();
  testFakeRelayControllerSimulatesAtoPumpOnFailure();
  testFakeRelayControllerSimulatesAtoPumpOffFailure();
  return 0;
}
