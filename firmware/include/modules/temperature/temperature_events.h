#pragma once

#include "core/events/event_bus.h"

namespace reeflow::modules::temperature {

enum class TemperatureEventType {
  kTemperatureUpdated,
  kTemperatureHigh,
  kTemperatureLow,
  kTemperatureSensorOffline,
  kTemperatureSensorRecovered,
};

const char* temperatureEventName(TemperatureEventType eventType);
core::events::EventType temperatureCoreEventType(
    TemperatureEventType eventType);

}  // namespace reeflow::modules::temperature
