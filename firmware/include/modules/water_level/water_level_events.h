#pragma once

#include "core/events/event_bus.h"

namespace reeflow::modules::water_level {

enum class WaterLevelEventType {
  kWaterLevelUpdated,
  kWaterLevelLow,
  kWaterLevelHigh,
  kWaterLevelSensorOffline,
  kWaterLevelSensorRecovered,
};

const char* waterLevelEventName(WaterLevelEventType eventType);
core::events::EventType waterLevelCoreEventType(
    WaterLevelEventType eventType);

}  // namespace reeflow::modules::water_level
