#include "modules/ato/ato_service.h"

#include "core/state/system_state.h"
#include "modules/ato/ato_events.h"

namespace reeflow::modules::ato {
namespace {

using relays::ATO_PUMP;
using relays::RelayCommand;
using relays::RelayCommandResult;
using relays::RelayCommandSource;
using relays::RelayDesiredState;

bool relayCommandForPumpCommand(AtoPumpCommand pumpCommand,
                                RelayCommand* command) {
  if (command == nullptr) {
    return false;
  }

  switch (pumpCommand) {
    case AtoPumpCommand::kNone:
      return false;
    case AtoPumpCommand::kTurnOn:
      *command = {ATO_PUMP, RelayDesiredState::kOn,
                  RelayCommandSource::kAutomation};
      return true;
    case AtoPumpCommand::kTurnOffAutomation:
      *command = {ATO_PUMP, RelayDesiredState::kOff,
                  RelayCommandSource::kAutomation};
      return true;
    case AtoPumpCommand::kTurnOffFailsafe:
      *command = {ATO_PUMP, RelayDesiredState::kOff,
                  RelayCommandSource::kFailsafe};
      return true;
  }

  return false;
}

bool atoEventForDecision(AtoDecision decision, AtoEventType* eventType) {
  if (eventType == nullptr) {
    return false;
  }

  switch (decision) {
    case AtoDecision::kStartRefill:
      *eventType = AtoEventType::kAtoStart;
      return true;
    case AtoDecision::kStopAtMaximumLevel:
      *eventType = AtoEventType::kAtoStop;
      return true;
    case AtoDecision::kStopTimeout:
      *eventType = AtoEventType::kAtoTimeout;
      return true;
    case AtoDecision::kBlockSensorOffline:
      *eventType = AtoEventType::kAtoSensorOffline;
      return true;
    case AtoDecision::kRecover:
      *eventType = AtoEventType::kAtoRecovered;
      return true;
    case AtoDecision::kNoAction:
    case AtoDecision::kBlockCooldown:
    case AtoDecision::kBlockInvalidConfig:
    case AtoDecision::kReconcilePumpDivergence:
    case AtoDecision::kDisable:
      return false;
  }

  return false;
}

}  // namespace

AtoService::AtoService(relays::RelayService& relayService,
                       const config::ConfigManager& configManager,
                       const core::platform::TimeSource& timeSource,
                       core::events::EventBus& eventBus,
                       AtoModuleConfig moduleConfig)
    : relayService_(relayService),
      configManager_(configManager),
      timeSource_(timeSource),
      eventBus_(eventBus),
      moduleConfig_(moduleConfig) {}

AtoServiceResult AtoService::evaluateOnce() {
  const core::state::SystemState& state = core::state::currentSystemState();
  AtoPolicyInput input = {};
  input.waterLevel = state.waterLevel;
  input.ato = state.ato;
  input.atoPumpRelay = state.relays.atoPump;
  input.config = configManager_.ato();
  input.nowMillis = timeSource_.uptimeMillis();

  const AtoEvaluationResult evaluation =
      evaluateAtoPolicy(input, moduleConfig_);

  const AtoServiceResult relayResult = applyRelayCommand(evaluation);
  if (relayResult != AtoServiceResult::kSuccess) {
    return relayResult;
  }

  if (evaluation.metadata.updatesAtoState) {
    core::state::updateAtoState(evaluation.nextAto);
  }

  if (evaluation.metadata.emitsEvent) {
    publishEventForDecision(evaluation.decision);
  }

  return AtoServiceResult::kSuccess;
}

AtoServiceResult AtoService::applyRelayCommand(
    const AtoEvaluationResult& evaluation) {
  RelayCommand command = {};
  if (!relayCommandForPumpCommand(evaluation.metadata.pumpCommand, &command)) {
    return AtoServiceResult::kSuccess;
  }

  const RelayCommandResult result = relayService_.applyCommand(command);
  if (result != RelayCommandResult::kSuccess) {
    return AtoServiceResult::kRelayCommandFailed;
  }

  return AtoServiceResult::kSuccess;
}

void AtoService::publishEventForDecision(AtoDecision decision) {
  AtoEventType eventType = AtoEventType::kAtoStart;
  if (!atoEventForDecision(decision, &eventType)) {
    return;
  }

  core::events::Event event = {};
  event.type = atoCoreEventType(eventType);
  event.stateArea = core::events::StateArea::kAto;
  eventBus_.publish(event);
}

bool runAtoServiceTask(void* context) {
  if (context == nullptr) {
    return false;
  }

  AtoService* service = static_cast<AtoService*>(context);
  return service->evaluateOnce() == AtoServiceResult::kSuccess;
}

}  // namespace reeflow::modules::ato
