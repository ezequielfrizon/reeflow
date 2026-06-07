#include <assert.h>
#include <string.h>

#include "config/config_manager.h"
#include "core/state/system_state.h"
#include "modules/water_level/water_level_calibration.h"
#include "modules/water_level/water_level_config.h"
#include "modules/water_level/water_level_events.h"
#include "modules/water_level/water_level_sensor.h"
#include "modules/water_level/water_level_state_evaluator.h"

namespace {

using reeflow::config::AtoConfig;
using reeflow::config::CalibrationsConfig;
using reeflow::core::state::WaterLevelStatus;
using reeflow::modules::water_level::WaterLevelEventType;
using reeflow::modules::water_level::WaterLevelStateEvaluationInput;
using reeflow::modules::water_level::calibrateWaterLevelReading;
using reeflow::modules::water_level::evaluateWaterLevelState;
using reeflow::modules::water_level::makeDefaultWaterLevelModuleConfig;
using reeflow::modules::water_level::makeOutOfRangeWaterLevelReading;
using reeflow::modules::water_level::makeSensorNotFoundWaterLevelReading;
using reeflow::modules::water_level::makeValidWaterLevelReading;
using reeflow::modules::water_level::makeWaterLevelReadError;
using reeflow::modules::water_level::waterLevelEventName;
using reeflow::modules::water_level::waterLevelReadIntervalElapsed;

AtoConfig defaultAtoConfig() {
  return reeflow::config::makeDefaultReeflowConfig().ato;
}

CalibrationsConfig defaultCalibrations() {
  return reeflow::config::makeDefaultReeflowConfig().calibrations;
}

WaterLevelStateEvaluationInput inputWithReading(
    reeflow::modules::water_level::WaterLevelSensorReading reading) {
  WaterLevelStateEvaluationInput input = {};
  input.sensorReading = reading;
  input.atoConfig = defaultAtoConfig();
  input.calibrations = defaultCalibrations();
  input.moduleConfig = makeDefaultWaterLevelModuleConfig();
  input.nowMillis = 100000;
  input.monitorStartedAtMillis = 70000;
  input.hasLastValidReading = true;
  input.lastValidReadingAtMillis = 95000;
  return input;
}

void testCalibrationWithOffsetZero() {
  const auto result = calibrateWaterLevelReading(
      makeValidWaterLevelReading(50), defaultCalibrations());

  assert(result.hasCalibratedLevel);
  assert(result.calibratedLevel == 50);
}

void testCalibrationWithPositiveOffset() {
  CalibrationsConfig calibrations = defaultCalibrations();
  calibrations.waterLevelOffset = 12;

  const auto result =
      calibrateWaterLevelReading(makeValidWaterLevelReading(50),
                                 calibrations);

  assert(result.hasCalibratedLevel);
  assert(result.calibratedLevel == 62);
}

void testCalibrationWithNegativeOffset() {
  CalibrationsConfig calibrations = defaultCalibrations();
  calibrations.waterLevelOffset = -15;

  const auto result =
      calibrateWaterLevelReading(makeValidWaterLevelReading(50),
                                 calibrations);

  assert(result.hasCalibratedLevel);
  assert(result.calibratedLevel == 35);
}

void testCalibrationProtectsUnderflow() {
  CalibrationsConfig calibrations = defaultCalibrations();
  calibrations.waterLevelOffset = -1000;

  const auto result =
      calibrateWaterLevelReading(makeValidWaterLevelReading(5),
                                 calibrations);

  assert(result.hasCalibratedLevel);
  assert(result.calibratedLevel == 0);
}

void testCalibrationProtectsOverflow() {
  CalibrationsConfig calibrations = defaultCalibrations();
  calibrations.waterLevelOffset = 1000;

  const auto result =
      calibrateWaterLevelReading(makeValidWaterLevelReading(95),
                                 calibrations);

  assert(result.hasCalibratedLevel);
  assert(result.calibratedLevel == 100);
}

void testCalibrationRejectsImpossibleReading() {
  const auto result = calibrateWaterLevelReading(
      makeOutOfRangeWaterLevelReading(120), defaultCalibrations());

  assert(!result.hasCalibratedLevel);
}

void testNormalWithinDefaultLimits() {
  const auto evaluation =
      evaluateWaterLevelState(inputWithReading(makeValidWaterLevelReading(50)));

  assert(evaluation.status == WaterLevelStatus::kNormal);
  assert(evaluation.statusIsAuthoritative);
  assert(evaluation.hasValidReading);
  assert(evaluation.currentLevel == 50);
}

void testLowBelowConfiguredMinimum() {
  const auto evaluation =
      evaluateWaterLevelState(inputWithReading(makeValidWaterLevelReading(19)));

  assert(evaluation.status == WaterLevelStatus::kLow);
  assert(evaluation.statusIsAuthoritative);
}

void testHighAboveConfiguredMaximum() {
  const auto evaluation =
      evaluateWaterLevelState(inputWithReading(makeValidWaterLevelReading(81)));

  assert(evaluation.status == WaterLevelStatus::kHigh);
  assert(evaluation.statusIsAuthoritative);
}

void testFailureBeforeTimeoutDoesNotBecomeOffline() {
  WaterLevelStateEvaluationInput input =
      inputWithReading(makeWaterLevelReadError());
  input.nowMillis = 120000;
  input.lastValidReadingAtMillis = 91000;

  const auto evaluation = evaluateWaterLevelState(input);

  assert(!evaluation.statusIsAuthoritative);
  assert(evaluation.status != WaterLevelStatus::kSensorOffline);
  assert(evaluation.sensorFailure);
  assert(!evaluation.offlineTimeoutElapsed);
}

void testFailureExactlyAtTimeoutDoesNotBecomeOffline() {
  WaterLevelStateEvaluationInput input =
      inputWithReading(makeSensorNotFoundWaterLevelReading());
  input.nowMillis = 121000;
  input.lastValidReadingAtMillis = 91000;

  const auto evaluation = evaluateWaterLevelState(input);

  assert(!evaluation.statusIsAuthoritative);
  assert(evaluation.status != WaterLevelStatus::kSensorOffline);
  assert(!evaluation.offlineTimeoutElapsed);
}

void testFailureAfterTimeoutBecomesOffline() {
  WaterLevelStateEvaluationInput input =
      inputWithReading(makeSensorNotFoundWaterLevelReading());
  input.nowMillis = 121001;
  input.lastValidReadingAtMillis = 91000;

  const auto evaluation = evaluateWaterLevelState(input);

  assert(evaluation.status == WaterLevelStatus::kSensorOffline);
  assert(evaluation.statusIsAuthoritative);
  assert(evaluation.offlineTimeoutElapsed);
}

void testOfflineWhenNoValidReadingSinceBoot() {
  WaterLevelStateEvaluationInput input =
      inputWithReading(makeWaterLevelReadError());
  input.hasLastValidReading = false;
  input.monitorStartedAtMillis = 1000;
  input.nowMillis = 31001;

  const auto evaluation = evaluateWaterLevelState(input);

  assert(evaluation.status == WaterLevelStatus::kSensorOffline);
  assert(evaluation.statusIsAuthoritative);
  assert(evaluation.offlineTimeoutElapsed);
}

void testConceptualRecoveryAfterOfflineWithValidReading() {
  WaterLevelStateEvaluationInput input =
      inputWithReading(makeValidWaterLevelReading(50));
  input.hasLastValidReading = false;
  input.monitorStartedAtMillis = 1000;
  input.nowMillis = 40000;

  const auto evaluation = evaluateWaterLevelState(input);

  assert(evaluation.status == WaterLevelStatus::kNormal);
  assert(evaluation.statusIsAuthoritative);
  assert(evaluation.hasValidReading);
  assert(!evaluation.offlineTimeoutElapsed);
}

void testCustomLimitsFromInMemoryConfig() {
  WaterLevelStateEvaluationInput input =
      inputWithReading(makeValidWaterLevelReading(39));
  input.atoConfig.minimumLevel = 40;
  input.atoConfig.maximumLevel = 60;

  const auto evaluation = evaluateWaterLevelState(input);

  assert(evaluation.status == WaterLevelStatus::kLow);
}

void testCalibrationOffsetAffectsEvaluatedLevel() {
  WaterLevelStateEvaluationInput input =
      inputWithReading(makeValidWaterLevelReading(78));
  input.calibrations.waterLevelOffset = 5;

  const auto evaluation = evaluateWaterLevelState(input);

  assert(evaluation.currentLevel == 83);
  assert(evaluation.status == WaterLevelStatus::kHigh);
}

void testOutOfRangeReadingCanBecomeOfflineAfterTimeout() {
  WaterLevelStateEvaluationInput input =
      inputWithReading(makeOutOfRangeWaterLevelReading(120));
  input.nowMillis = 121001;
  input.lastValidReadingAtMillis = 91000;

  const auto evaluation = evaluateWaterLevelState(input);

  assert(evaluation.sensorFailure);
  assert(evaluation.status == WaterLevelStatus::kSensorOffline);
}

void testInvalidAtoLimitsDoNotProduceAuthoritativeStatus() {
  WaterLevelStateEvaluationInput input =
      inputWithReading(makeValidWaterLevelReading(50));
  input.atoConfig.minimumLevel = 80;
  input.atoConfig.maximumLevel = 20;

  const auto evaluation = evaluateWaterLevelState(input);

  assert(!evaluation.configurationValid);
  assert(!evaluation.statusIsAuthoritative);
  assert(evaluation.hasValidReading);
  assert(evaluation.currentLevel == 50);
}

void testReadIntervalUsesModuleContract() {
  const auto moduleConfig = makeDefaultWaterLevelModuleConfig();

  assert(waterLevelReadIntervalElapsed(moduleConfig, 1000, false, 0));
  assert(!waterLevelReadIntervalElapsed(moduleConfig, 5999, true, 1000));
  assert(waterLevelReadIntervalElapsed(moduleConfig, 6000, true, 1000));

  auto custom = moduleConfig;
  custom.readIntervalMillis = 2500;
  assert(!waterLevelReadIntervalElapsed(custom, 3499, true, 1000));
  assert(waterLevelReadIntervalElapsed(custom, 3500, true, 1000));
}

void testLocalWaterLevelEventContracts() {
  assert(strcmp(waterLevelEventName(WaterLevelEventType::kWaterLevelUpdated),
                "WATER_LEVEL_UPDATED") == 0);
  assert(strcmp(waterLevelEventName(WaterLevelEventType::kWaterLevelLow),
                "WATER_LEVEL_LOW") == 0);
  assert(strcmp(waterLevelEventName(WaterLevelEventType::kWaterLevelHigh),
                "WATER_LEVEL_HIGH") == 0);
  assert(strcmp(waterLevelEventName(
                    WaterLevelEventType::kWaterLevelSensorOffline),
                "WATER_LEVEL_SENSOR_OFFLINE") == 0);
  assert(strcmp(waterLevelEventName(
                    WaterLevelEventType::kWaterLevelSensorRecovered),
                "WATER_LEVEL_SENSOR_RECOVERED") == 0);
}

}  // namespace

int main() {
  testCalibrationWithOffsetZero();
  testCalibrationWithPositiveOffset();
  testCalibrationWithNegativeOffset();
  testCalibrationProtectsUnderflow();
  testCalibrationProtectsOverflow();
  testCalibrationRejectsImpossibleReading();
  testNormalWithinDefaultLimits();
  testLowBelowConfiguredMinimum();
  testHighAboveConfiguredMaximum();
  testFailureBeforeTimeoutDoesNotBecomeOffline();
  testFailureExactlyAtTimeoutDoesNotBecomeOffline();
  testFailureAfterTimeoutBecomesOffline();
  testOfflineWhenNoValidReadingSinceBoot();
  testConceptualRecoveryAfterOfflineWithValidReading();
  testCustomLimitsFromInMemoryConfig();
  testCalibrationOffsetAffectsEvaluatedLevel();
  testOutOfRangeReadingCanBecomeOfflineAfterTimeout();
  testInvalidAtoLimitsDoNotProduceAuthoritativeStatus();
  testReadIntervalUsesModuleContract();
  testLocalWaterLevelEventContracts();
  return 0;
}
