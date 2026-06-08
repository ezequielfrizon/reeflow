#pragma once

#include <stdint.h>

#include "core/events/event_bus.h"
#include "modules/modes/mode_types.h"

namespace reeflow::modules::modes {

enum class ModeEventType {
  kModeChanged,
  kModeStarted,
  kModeFinished,
};

enum class ModeFinishReason {
  kNone,
  kManual,
  kAutomaticTimeout,
  kRestoredToSafeDefault,
};

struct ModeChangedEventData {
  OperationalMode previousMode;
  OperationalMode currentMode;
  uint32_t startedAtMillis;
  uint32_t remainingTimeSeconds;
};

struct ModeFinishedEventData {
  OperationalMode finishedMode;
  OperationalMode nextMode;
  ModeFinishReason reason;
  uint32_t finishedAtMillis;
};

const char* modeEventName(ModeEventType eventType);
core::events::EventType modeCoreEventType(ModeEventType eventType);
core::events::Event makeModeCoreEvent(ModeEventType eventType);

}  // namespace reeflow::modules::modes
