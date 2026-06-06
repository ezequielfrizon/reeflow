#pragma once

#include <stdint.h>

#include "modules/water_level/water_level_config.h"

namespace reeflow::modules::water_level {

enum class WaterLevelSensorReadStatus {
  kValid,
  kSensorNotFound,
  kReadError,
  kOutOfRange,
};

struct WaterLevelSensorReading {
  WaterLevelSensorReadStatus status;
  int16_t logicalLevel;
};

constexpr WaterLevelSensorReading makeValidWaterLevelReading(
    uint16_t logicalLevel) {
  return isCanonicalWaterLevel(static_cast<int16_t>(logicalLevel))
             ? WaterLevelSensorReading{WaterLevelSensorReadStatus::kValid,
                                       static_cast<int16_t>(logicalLevel)}
             : WaterLevelSensorReading{
                   WaterLevelSensorReadStatus::kOutOfRange,
                   static_cast<int16_t>(logicalLevel)};
}

constexpr WaterLevelSensorReading makeSensorNotFoundWaterLevelReading() {
  return {WaterLevelSensorReadStatus::kSensorNotFound, 0};
}

constexpr WaterLevelSensorReading makeWaterLevelReadError() {
  return {WaterLevelSensorReadStatus::kReadError, 0};
}

constexpr WaterLevelSensorReading makeOutOfRangeWaterLevelReading(
    int16_t logicalLevel) {
  return {WaterLevelSensorReadStatus::kOutOfRange, logicalLevel};
}

class WaterLevelSensor {
 public:
  virtual ~WaterLevelSensor() = default;

  virtual WaterLevelSensorReading readLevel() = 0;
};

}  // namespace reeflow::modules::water_level
