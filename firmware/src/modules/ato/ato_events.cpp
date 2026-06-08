#include "modules/ato/ato_events.h"

namespace reeflow::modules::ato {

const char* atoEventName(AtoEventType eventType) {
  switch (eventType) {
    case AtoEventType::kAtoStart:
      return "ATO_START";
    case AtoEventType::kAtoStop:
      return "ATO_STOP";
    case AtoEventType::kAtoTimeout:
      return "ATO_TIMEOUT";
    case AtoEventType::kAtoSensorOffline:
      return "ATO_SENSOR_OFFLINE";
    case AtoEventType::kAtoRecovered:
      return "ATO_RECOVERED";
  }

  return "UNKNOWN_ATO_EVENT";
}

core::events::EventType atoCoreEventType(AtoEventType eventType) {
  switch (eventType) {
    case AtoEventType::kAtoStart:
      return core::events::EventType::kAtoStart;
    case AtoEventType::kAtoStop:
      return core::events::EventType::kAtoStop;
    case AtoEventType::kAtoTimeout:
      return core::events::EventType::kAtoTimeout;
    case AtoEventType::kAtoSensorOffline:
      return core::events::EventType::kAtoSensorOffline;
    case AtoEventType::kAtoRecovered:
      return core::events::EventType::kAtoRecovered;
  }

  return core::events::EventType::kAtoStart;
}

}  // namespace reeflow::modules::ato
