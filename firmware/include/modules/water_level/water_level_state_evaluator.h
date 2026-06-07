#pragma once

#include <stdint.h>

#include "config/config_manager.h"
#include "core/state/system_state.h"
#include "modules/water_level/water_level_calibration.h"
#include "modules/water_level/water_level_config.h"
#include "modules/water_level/water_level_sensor.h"

namespace reeflow::modules::water_level {

struct WaterLevelStateEvaluationInput {
  WaterLevelSensorReading sensorReading;
  config::AtoConfig atoConfig;
  config::CalibrationsConfig calibrations;
  WaterLevelModuleConfig moduleConfig;
  uint32_t nowMillis;
  uint32_t monitorStartedAtMillis;
  bool hasLastValidReading;
  uint32_t lastValidReadingAtMillis;
};

struct WaterLevelStateEvaluation {
  core::state::WaterLevelStatus status;
  bool statusIsAuthoritative;
  bool hasValidReading;
  uint16_t currentLevel;
  bool sensorFailure;
  bool offlineTimeoutElapsed;
  bool configurationValid;
};

WaterLevelStateEvaluation evaluateWaterLevelState(
    const WaterLevelStateEvaluationInput& input);

bool waterLevelReadIntervalElapsed(
    const WaterLevelModuleConfig& moduleConfig, uint32_t nowMillis,
    bool hasLastReadAttempt, uint32_t lastReadAttemptAtMillis);

}  // namespace reeflow::modules::water_level
