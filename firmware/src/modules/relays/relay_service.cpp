#include "modules/relays/relay_service.h"

namespace reeflow::modules::relays {
namespace {

using core::state::RelayEntryState;
using core::state::RelaysState;

RelayEntryState* relayEntry(RelaysState& relays, RelayId relay) {
  switch (relay) {
    case RelayId::kRecalque:
      return &relays.recalque;
    case RelayId::kHeater:
      return &relays.heater;
    case RelayId::kAtoPump:
      return &relays.atoPump;
    case RelayId::kReserve:
      return &relays.reserve;
    case RelayId::kUnknown:
      return nullptr;
  }

  return nullptr;
}

bool desiredStateIsEnabled(RelayDesiredState desiredState) {
  return desiredState == RelayDesiredState::kOn;
}

RelayEventType eventForDesiredState(RelayDesiredState desiredState) {
  return desiredState == RelayDesiredState::kOn ? RelayEventType::kRelayOn
                                                : RelayEventType::kRelayOff;
}

}  // namespace

RelayService::RelayService(RelayController& controller,
                           const core::platform::TimeSource& timeSource,
                           core::events::EventBus& eventBus)
    : controller_(controller), timeSource_(timeSource), eventBus_(eventBus) {}

RelayCommandResult RelayService::applyCommand(const RelayCommand& command) {
  if (!isLocalFirmwareRelayCommandSource(command.source)) {
    return RelayCommandResult::kInvalidContract;
  }

  RelaysState relays = core::state::currentSystemState().relays;
  return applyIndividualStateChange(relays, command.relay, command.desiredState,
                                    command.source,
                                    timeSource_.uptimeMillis(), true);
}

RelayCommandResult RelayService::setLocalRelay(
    RelayId relay, RelayDesiredState desiredState) {
  return applyCommand(makeLocalRelayCommand(relay, desiredState));
}

RelayCommandResult RelayService::allOff(RelayCommandSource source) {
  if (!isLocalFirmwareRelayCommandSource(source)) {
    return RelayCommandResult::kInvalidContract;
  }

  const RelayCommandResult controllerResult = controller_.allOff();
  if (controllerResult != RelayCommandResult::kSuccess) {
    return controllerResult;
  }

  RelaysState relays = core::state::currentSystemState().relays;
  const uint32_t nowMillis = timeSource_.uptimeMillis();
  const RelayId relayOrder[] = {
      RelayId::kRecalque,
      RelayId::kHeater,
      RelayId::kAtoPump,
      RelayId::kReserve,
  };

  for (const RelayId relay : relayOrder) {
    const RelayCommandResult result = applyIndividualStateChange(
        relays, relay, RelayDesiredState::kOff, source, nowMillis, false);
    if (result != RelayCommandResult::kSuccess) {
      return result;
    }
  }

  return RelayCommandResult::kSuccess;
}

RelayCommandResult RelayService::applyIndividualStateChange(
    RelaysState& relays, RelayId relay, RelayDesiredState desiredState,
    RelayCommandSource source, uint32_t nowMillis, bool allowControllerCall) {
  RelayEntryState* entry = relayEntry(relays, relay);
  if (entry == nullptr) {
    return RelayCommandResult::kUnknownRelay;
  }

  const bool nextEnabled = desiredStateIsEnabled(desiredState);
  if (entry->enabled == nextEnabled) {
    return RelayCommandResult::kSuccess;
  }

  if (allowControllerCall) {
    const RelayCommandResult controllerResult =
        controller_.setRelay(relay, desiredState);
    if (controllerResult != RelayCommandResult::kSuccess) {
      return controllerResult;
    }
  }

  entry->enabled = nextEnabled;
  entry->lastChanged = nowMillis;
  entry->source = source;
  core::state::updateRelaysState(relays);
  publishRelayEvent(eventForDesiredState(desiredState));
  return RelayCommandResult::kSuccess;
}

void RelayService::publishRelayEvent(RelayEventType eventType) {
  core::events::Event event = {};
  event.type = relayCoreEventType(eventType);
  event.stateArea = core::events::StateArea::kRelays;
  eventBus_.publish(event);
}

}  // namespace reeflow::modules::relays
