#include "modules/temperature/temperature_events.h"

namespace reeflow::modules::temperature {

const char* temperatureEventName(TemperatureEventType eventType) {
  switch (eventType) {
    case TemperatureEventType::kTemperatureUpdated:
      return "TEMPERATURE_UPDATED";
    case TemperatureEventType::kTemperatureHigh:
      return "TEMPERATURE_HIGH";
    case TemperatureEventType::kTemperatureLow:
      return "TEMPERATURE_LOW";
    case TemperatureEventType::kTemperatureSensorOffline:
      return "TEMPERATURE_SENSOR_OFFLINE";
    case TemperatureEventType::kTemperatureSensorRecovered:
      return "TEMPERATURE_SENSOR_RECOVERED";
  }

  return "UNKNOWN_TEMPERATURE_EVENT";
}

core::events::EventType temperatureCoreEventType(
    TemperatureEventType eventType) {
  switch (eventType) {
    case TemperatureEventType::kTemperatureUpdated:
      return core::events::EventType::kTemperatureUpdated;
    case TemperatureEventType::kTemperatureHigh:
      return core::events::EventType::kTemperatureHigh;
    case TemperatureEventType::kTemperatureLow:
      return core::events::EventType::kTemperatureLow;
    case TemperatureEventType::kTemperatureSensorOffline:
      return core::events::EventType::kTemperatureSensorOffline;
    case TemperatureEventType::kTemperatureSensorRecovered:
      return core::events::EventType::kTemperatureSensorRecovered;
  }

  return core::events::EventType::kTemperatureUpdated;
}

}  // namespace reeflow::modules::temperature
