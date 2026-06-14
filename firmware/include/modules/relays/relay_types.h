#pragma once

#include "core/state/system_state.h"

namespace reeflow::modules::relays {

enum class RelayId {
  kRecalque,
  kHeater,
  kAtoPump,
  kReserve,
  kUnknown,
};

constexpr RelayId RECALQUE = RelayId::kRecalque;
constexpr RelayId HEATER = RelayId::kHeater;
constexpr RelayId ATO_PUMP = RelayId::kAtoPump;
constexpr RelayId RESERVE = RelayId::kReserve;

enum class RelayDesiredState {
  kOff,
  kOn,
};

using RelayCommandSource = core::state::RelaySource;

struct RelayCommand {
  RelayId relay;
  RelayDesiredState desiredState;
  RelayCommandSource source;
};

constexpr RelayCommand makeLocalRelayCommand(RelayId relay,
                                             RelayDesiredState desiredState) {
  return {relay, desiredState, RelayCommandSource::kLocal};
}

constexpr RelayCommand makeFailsafeRelayCommand(
    RelayId relay, RelayDesiredState desiredState) {
  return {relay, desiredState, RelayCommandSource::kFailsafe};
}

constexpr RelayCommand makeAutomationRelayCommand(
    RelayId relay, RelayDesiredState desiredState) {
  return {relay, desiredState, RelayCommandSource::kAutomation};
}

constexpr bool isKnownRelay(RelayId relay) {
  return relay == RelayId::kRecalque || relay == RelayId::kHeater ||
         relay == RelayId::kAtoPump || relay == RelayId::kReserve;
}

constexpr bool isLocalFirmwareRelayCommandSource(
    RelayCommandSource source) {
  return source == RelayCommandSource::kLocal ||
         source == RelayCommandSource::kMqtt ||
         source == RelayCommandSource::kAutomation ||
         source == RelayCommandSource::kFailsafe;
}

inline const char* relaySystemStateFieldName(RelayId relay) {
  switch (relay) {
    case RelayId::kRecalque:
      return "relays.recalque";
    case RelayId::kHeater:
      return "relays.heater";
    case RelayId::kAtoPump:
      return "relays.atoPump";
    case RelayId::kReserve:
      return "relays.reserve";
    case RelayId::kUnknown:
      return "";
  }

  return "";
}

inline const char* relayDisplayName(RelayId relay) {
  switch (relay) {
    case RelayId::kRecalque:
      return "Recalque";
    case RelayId::kHeater:
      return "Aquecedor";
    case RelayId::kAtoPump:
      return "ATO";
    case RelayId::kReserve:
      return "Reserva";
    case RelayId::kUnknown:
      return "";
  }

  return "";
}

}  // namespace reeflow::modules::relays
