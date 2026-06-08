#include "modules/lighting/lighting_events.h"

namespace reeflow::modules::lighting {

const char* lightingEventName(LightingEventType eventType) {
  switch (eventType) {
    case LightingEventType::kLightingProfileChanged:
      return "LIGHTING_PROFILE_CHANGED";
    case LightingEventType::kLightingStarted:
      return "LIGHTING_STARTED";
    case LightingEventType::kLightingStopped:
      return "LIGHTING_STOPPED";
  }

  return "UNKNOWN_LIGHTING_EVENT";
}

core::events::EventType lightingCoreEventType(
    LightingEventType eventType) {
  switch (eventType) {
    case LightingEventType::kLightingProfileChanged:
      return core::events::EventType::kLightingProfileChanged;
    case LightingEventType::kLightingStarted:
      return core::events::EventType::kLightingStarted;
    case LightingEventType::kLightingStopped:
      return core::events::EventType::kLightingStopped;
  }

  return core::events::EventType::kLightingProfileChanged;
}

}  // namespace reeflow::modules::lighting
