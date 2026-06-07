#include "modules/water_level/water_level_calibration.h"

#include <stdint.h>

namespace reeflow::modules::water_level {
namespace {

uint16_t clampToModuleRange(int32_t value,
                            const WaterLevelModuleConfig& moduleConfig) {
  if (value < static_cast<int32_t>(moduleConfig.canonicalMinimumLevel)) {
    return moduleConfig.canonicalMinimumLevel;
  }

  if (value > static_cast<int32_t>(moduleConfig.canonicalMaximumLevel)) {
    return moduleConfig.canonicalMaximumLevel;
  }

  return static_cast<uint16_t>(value);
}

}  // namespace

WaterLevelCalibrationResult calibrateWaterLevelReading(
    const WaterLevelSensorReading& sensorReading,
    const config::CalibrationsConfig& calibrations,
    const WaterLevelModuleConfig& moduleConfig) {
  WaterLevelCalibrationResult result = {};

  if (sensorReading.status != WaterLevelSensorReadStatus::kValid ||
      !isCanonicalWaterLevel(sensorReading.logicalLevel)) {
    return result;
  }

  const int32_t adjustedLevel =
      static_cast<int32_t>(sensorReading.logicalLevel) +
      static_cast<int32_t>(calibrations.waterLevelOffset);

  result.hasCalibratedLevel = true;
  result.calibratedLevel = clampToModuleRange(adjustedLevel, moduleConfig);
  return result;
}

}  // namespace reeflow::modules::water_level
