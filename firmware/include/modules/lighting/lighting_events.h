#pragma once

#include <stdint.h>

#include "core/events/event_bus.h"
#include "modules/lighting/lighting_types.h"

namespace reeflow::modules::lighting {

enum class LightingEventType {
  kLightingProfileChanged,
  kLightingStarted,
  kLightingStopped,
};

struct LightingEvent {
  LightingEventType type;
  LightingMode mode;
  LightingChannelMask affectedChannels;
  char profile[kLightingProfileNameStorageLength];
  uint32_t occurredAtMillis;
};

const char* lightingEventName(LightingEventType eventType);
core::events::EventType lightingCoreEventType(LightingEventType eventType);

}  // namespace reeflow::modules::lighting
