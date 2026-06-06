#pragma once

#include <stdint.h>

namespace reeflow::modules::water_level {

constexpr uint16_t kWaterLevelCanonicalMinimum = 0;
constexpr uint16_t kWaterLevelCanonicalMaximum = 100;
constexpr uint32_t kDefaultWaterLevelReadIntervalMillis = 5000;
constexpr uint32_t kDefaultWaterLevelSensorOfflineTimeoutMillis = 30000;

struct WaterLevelModuleConfig {
  uint32_t readIntervalMillis;
  uint32_t sensorOfflineTimeoutMillis;
  uint16_t canonicalMinimumLevel;
  uint16_t canonicalMaximumLevel;
};

constexpr WaterLevelModuleConfig makeDefaultWaterLevelModuleConfig() {
  return {kDefaultWaterLevelReadIntervalMillis,
          kDefaultWaterLevelSensorOfflineTimeoutMillis,
          kWaterLevelCanonicalMinimum,
          kWaterLevelCanonicalMaximum};
}

constexpr bool isCanonicalWaterLevel(int16_t logicalLevel) {
  return logicalLevel >= static_cast<int16_t>(kWaterLevelCanonicalMinimum) &&
         logicalLevel <= static_cast<int16_t>(kWaterLevelCanonicalMaximum);
}

}  // namespace reeflow::modules::water_level
