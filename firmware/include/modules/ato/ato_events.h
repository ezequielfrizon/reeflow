#pragma once

#include "core/events/event_bus.h"

namespace reeflow::modules::ato {

enum class AtoEventType {
  kAtoStart,
  kAtoStop,
  kAtoTimeout,
  kAtoSensorOffline,
  kAtoRecovered,
};

const char* atoEventName(AtoEventType eventType);
core::events::EventType atoCoreEventType(AtoEventType eventType);

}  // namespace reeflow::modules::ato
