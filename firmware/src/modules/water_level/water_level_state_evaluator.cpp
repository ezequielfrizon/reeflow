#include "modules/water_level/water_level_state_evaluator.h"

namespace reeflow::modules::water_level {
namespace {

bool elapsedMoreThan(uint32_t nowMillis, uint32_t startedAtMillis,
                     uint32_t intervalMillis) {
  return static_cast<uint32_t>(nowMillis - startedAtMillis) > intervalMillis;
}

bool atoLevelLimitsAreValid(const config::AtoConfig& atoConfig,
                            const WaterLevelModuleConfig& moduleConfig) {
  return atoConfig.minimumLevel < atoConfig.maximumLevel &&
         atoConfig.minimumLevel >= moduleConfig.canonicalMinimumLevel &&
         atoConfig.maximumLevel <= moduleConfig.canonicalMaximumLevel;
}

core::state::WaterLevelStatus classifyValidWaterLevel(
    uint16_t calibratedLevel, const config::AtoConfig& atoConfig) {
  if (calibratedLevel < atoConfig.minimumLevel) {
    return core::state::WaterLevelStatus::kLow;
  }

  if (calibratedLevel > atoConfig.maximumLevel) {
    return core::state::WaterLevelStatus::kHigh;
  }

  return core::state::WaterLevelStatus::kNormal;
}

bool isSensorFailure(WaterLevelSensorReadStatus status) {
  return status == WaterLevelSensorReadStatus::kSensorNotFound ||
         status == WaterLevelSensorReadStatus::kReadError ||
         status == WaterLevelSensorReadStatus::kOutOfRange;
}

}  // namespace

WaterLevelStateEvaluation evaluateWaterLevelState(
    const WaterLevelStateEvaluationInput& input) {
  WaterLevelStateEvaluation evaluation = {};
  evaluation.status = core::state::WaterLevelStatus::kNormal;
  evaluation.configurationValid =
      atoLevelLimitsAreValid(input.atoConfig, input.moduleConfig);
  evaluation.sensorFailure = isSensorFailure(input.sensorReading.status);

  const WaterLevelCalibrationResult calibration = calibrateWaterLevelReading(
      input.sensorReading, input.calibrations, input.moduleConfig);

  if (calibration.hasCalibratedLevel) {
    evaluation.hasValidReading = true;
    evaluation.currentLevel = calibration.calibratedLevel;

    if (evaluation.configurationValid) {
      evaluation.status =
          classifyValidWaterLevel(calibration.calibratedLevel,
                                  input.atoConfig);
      evaluation.statusIsAuthoritative = true;
    }

    return evaluation;
  }

  const uint32_t offlineReferenceMillis =
      input.hasLastValidReading ? input.lastValidReadingAtMillis
                                : input.monitorStartedAtMillis;
  evaluation.offlineTimeoutElapsed =
      elapsedMoreThan(input.nowMillis, offlineReferenceMillis,
                      input.moduleConfig.sensorOfflineTimeoutMillis);

  if (evaluation.offlineTimeoutElapsed) {
    evaluation.status = core::state::WaterLevelStatus::kSensorOffline;
    evaluation.statusIsAuthoritative = true;
  }

  return evaluation;
}

bool waterLevelReadIntervalElapsed(
    const WaterLevelModuleConfig& moduleConfig, uint32_t nowMillis,
    bool hasLastReadAttempt, uint32_t lastReadAttemptAtMillis) {
  if (!hasLastReadAttempt) {
    return true;
  }

  return static_cast<uint32_t>(nowMillis - lastReadAttemptAtMillis) >=
         moduleConfig.readIntervalMillis;
}

}  // namespace reeflow::modules::water_level
