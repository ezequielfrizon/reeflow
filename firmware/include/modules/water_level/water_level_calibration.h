#pragma once

#include <stdint.h>

#include "config/config_manager.h"
#include "modules/water_level/water_level_config.h"
#include "modules/water_level/water_level_sensor.h"

namespace reeflow::modules::water_level {

struct WaterLevelCalibrationResult {
  bool hasCalibratedLevel;
  uint16_t calibratedLevel;
};

WaterLevelCalibrationResult calibrateWaterLevelReading(
    const WaterLevelSensorReading& sensorReading,
    const config::CalibrationsConfig& calibrations,
    const WaterLevelModuleConfig& moduleConfig =
        makeDefaultWaterLevelModuleConfig());

}  // namespace reeflow::modules::water_level
