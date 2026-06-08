#pragma once

#include "core/events/event_bus.h"

namespace reeflow::modules::relays {

enum class RelayEventType {
  kRelayOn,
  kRelayOff,
};

const char* relayEventName(RelayEventType eventType);
core::events::EventType relayCoreEventType(RelayEventType eventType);

}  // namespace reeflow::modules::relays
