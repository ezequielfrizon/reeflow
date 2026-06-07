#include "modules/water_level/water_level_events.h"

namespace reeflow::modules::water_level {

const char* waterLevelEventName(WaterLevelEventType eventType) {
  switch (eventType) {
    case WaterLevelEventType::kWaterLevelUpdated:
      return "WATER_LEVEL_UPDATED";
    case WaterLevelEventType::kWaterLevelLow:
      return "WATER_LEVEL_LOW";
    case WaterLevelEventType::kWaterLevelHigh:
      return "WATER_LEVEL_HIGH";
    case WaterLevelEventType::kWaterLevelSensorOffline:
      return "WATER_LEVEL_SENSOR_OFFLINE";
    case WaterLevelEventType::kWaterLevelSensorRecovered:
      return "WATER_LEVEL_SENSOR_RECOVERED";
  }

  return "UNKNOWN_WATER_LEVEL_EVENT";
}

core::events::EventType waterLevelCoreEventType(
    WaterLevelEventType eventType) {
  switch (eventType) {
    case WaterLevelEventType::kWaterLevelUpdated:
      return core::events::EventType::kWaterLevelUpdated;
    case WaterLevelEventType::kWaterLevelLow:
      return core::events::EventType::kWaterLevelLow;
    case WaterLevelEventType::kWaterLevelHigh:
      return core::events::EventType::kWaterLevelHigh;
    case WaterLevelEventType::kWaterLevelSensorOffline:
      return core::events::EventType::kWaterLevelSensorOffline;
    case WaterLevelEventType::kWaterLevelSensorRecovered:
      return core::events::EventType::kWaterLevelSensorRecovered;
  }

  return core::events::EventType::kWaterLevelUpdated;
}

}  // namespace reeflow::modules::water_level
