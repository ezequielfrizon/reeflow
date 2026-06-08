#include "modules/modes/mode_events.h"

namespace reeflow::modules::modes {

const char* modeEventName(ModeEventType eventType) {
  switch (eventType) {
    case ModeEventType::kModeChanged:
      return "MODE_CHANGED";
    case ModeEventType::kModeStarted:
      return "MODE_STARTED";
    case ModeEventType::kModeFinished:
      return "MODE_FINISHED";
  }

  return "UNKNOWN_MODE_EVENT";
}

core::events::EventType modeCoreEventType(ModeEventType eventType) {
  switch (eventType) {
    case ModeEventType::kModeChanged:
      return core::events::EventType::kModeChanged;
    case ModeEventType::kModeStarted:
      return core::events::EventType::kModeStarted;
    case ModeEventType::kModeFinished:
      return core::events::EventType::kModeFinished;
  }

  return core::events::EventType::kModeChanged;
}

core::events::Event makeModeCoreEvent(ModeEventType eventType) {
  core::events::Event event = {};
  event.type = modeCoreEventType(eventType);
  event.stateArea = core::events::StateArea::kModes;
  return event;
}

}  // namespace reeflow::modules::modes
