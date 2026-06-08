#include "modules/relays/relay_events.h"

namespace reeflow::modules::relays {

const char* relayEventName(RelayEventType eventType) {
  switch (eventType) {
    case RelayEventType::kRelayOn:
      return "RELAY_ON";
    case RelayEventType::kRelayOff:
      return "RELAY_OFF";
  }

  return "UNKNOWN_RELAY_EVENT";
}

core::events::EventType relayCoreEventType(RelayEventType eventType) {
  switch (eventType) {
    case RelayEventType::kRelayOn:
      return core::events::EventType::kRelayOn;
    case RelayEventType::kRelayOff:
      return core::events::EventType::kRelayOff;
  }

  return core::events::EventType::kRelayOff;
}

}  // namespace reeflow::modules::relays
