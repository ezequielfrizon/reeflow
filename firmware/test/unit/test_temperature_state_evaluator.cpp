#include <assert.h>
#include <string.h>

#include "config/config_manager.h"
#include "core/state/system_state.h"
#include "modules/temperature/temperature_events.h"
#include "modules/temperature/temperature_sensor.h"
#include "modules/temperature/temperature_state_evaluator.h"

namespace {

using reeflow::config::TemperatureConfig;
using reeflow::core::state::TemperatureStatus;
using reeflow::modules::temperature::TemperatureEventType;
using reeflow::modules::temperature::TemperatureStateEvaluationInput;
using reeflow::modules::temperature::evaluateTemperatureState;
using reeflow::modules::temperature::makeSensorNotFoundReading;
using reeflow::modules::temperature::makeTemperatureConversionPending;
using reeflow::modules::temperature::makeTemperatureReadError;
using reeflow::modules::temperature::makeValidTemperatureReading;
using reeflow::modules::temperature::temperatureEventName;
using reeflow::modules::temperature::temperatureReadIntervalElapsed;

TemperatureConfig defaultTemperatureConfig() {
  return reeflow::config::makeDefaultReeflowConfig().temperature;
}

TemperatureStateEvaluationInput inputWithReading(
    reeflow::modules::temperature::TemperatureSensorReading reading) {
  TemperatureStateEvaluationInput input = {};
  input.sensorReading = reading;
  input.config = defaultTemperatureConfig();
  input.nowMillis = 100000;
  input.monitorStartedAtMillis = 70000;
  input.hasLastValidReading = true;
  input.lastValidReadingAtMillis = 95000;
  return input;
}

void testNormalWithinDefaultLimits() {
  const auto evaluation =
      evaluateTemperatureState(inputWithReading(makeValidTemperatureReading(26.5F)));

  assert(evaluation.status == TemperatureStatus::kNormal);
  assert(evaluation.statusIsAuthoritative);
  assert(evaluation.hasValidReading);
  assert(evaluation.temperatureCelsius == 26.5F);
}

void testHighMapsAltaConceptToCanonicalHigh() {
  const auto evaluation =
      evaluateTemperatureState(inputWithReading(makeValidTemperatureReading(28.1F)));

  assert(evaluation.status == TemperatureStatus::kHigh);
  assert(evaluation.statusIsAuthoritative);
}

void testLowMapsBaixaConceptToCanonicalLow() {
  const auto evaluation =
      evaluateTemperatureState(inputWithReading(makeValidTemperatureReading(24.9F)));

  assert(evaluation.status == TemperatureStatus::kLow);
  assert(evaluation.statusIsAuthoritative);
}

void testFailureBeforeTimeoutDoesNotBecomeOffline() {
  TemperatureStateEvaluationInput input =
      inputWithReading(makeTemperatureReadError());
  input.nowMillis = 120000;
  input.lastValidReadingAtMillis = 91000;

  const auto evaluation = evaluateTemperatureState(input);

  assert(!evaluation.statusIsAuthoritative);
  assert(evaluation.status != TemperatureStatus::kSensorOffline);
  assert(evaluation.sensorFailure);
  assert(!evaluation.offlineTimeoutElapsed);
}

void testFailureExactlyAtTimeoutDoesNotBecomeOffline() {
  TemperatureStateEvaluationInput input =
      inputWithReading(makeSensorNotFoundReading());
  input.nowMillis = 121000;
  input.lastValidReadingAtMillis = 91000;

  const auto evaluation = evaluateTemperatureState(input);

  assert(!evaluation.statusIsAuthoritative);
  assert(evaluation.status != TemperatureStatus::kSensorOffline);
  assert(!evaluation.offlineTimeoutElapsed);
}

void testFailureAfterTimeoutBecomesOffline() {
  TemperatureStateEvaluationInput input =
      inputWithReading(makeSensorNotFoundReading());
  input.nowMillis = 121001;
  input.lastValidReadingAtMillis = 91000;

  const auto evaluation = evaluateTemperatureState(input);

  assert(evaluation.status == TemperatureStatus::kSensorOffline);
  assert(evaluation.statusIsAuthoritative);
  assert(evaluation.offlineTimeoutElapsed);
}

void testOfflineWhenNoValidReadingSinceBoot() {
  TemperatureStateEvaluationInput input =
      inputWithReading(makeTemperatureReadError());
  input.hasLastValidReading = false;
  input.monitorStartedAtMillis = 1000;
  input.nowMillis = 31001;

  const auto evaluation = evaluateTemperatureState(input);

  assert(evaluation.status == TemperatureStatus::kSensorOffline);
  assert(evaluation.statusIsAuthoritative);
  assert(evaluation.offlineTimeoutElapsed);
}

void testConceptualRecoveryAfterOfflineWithValidReading() {
  TemperatureStateEvaluationInput input =
      inputWithReading(makeValidTemperatureReading(26.0F));
  input.hasLastValidReading = false;
  input.monitorStartedAtMillis = 1000;
  input.nowMillis = 40000;

  const auto evaluation = evaluateTemperatureState(input);

  assert(evaluation.status == TemperatureStatus::kNormal);
  assert(evaluation.statusIsAuthoritative);
  assert(evaluation.hasValidReading);
  assert(!evaluation.offlineTimeoutElapsed);
}

void testCustomLimitsFromInMemoryConfig() {
  TemperatureStateEvaluationInput input =
      inputWithReading(makeValidTemperatureReading(27.6F));
  input.config.minTemperature = 26.0F;
  input.config.maxTemperature = 27.5F;
  input.config.targetTemperature = 26.8F;

  const auto evaluation = evaluateTemperatureState(input);

  assert(evaluation.status == TemperatureStatus::kHigh);
}

void testCustomOfflineTimeout() {
  TemperatureStateEvaluationInput input =
      inputWithReading(makeSensorNotFoundReading());
  input.config.sensorOfflineTimeoutMillis = 10000;
  input.nowMillis = 20101;
  input.lastValidReadingAtMillis = 10000;

  const auto evaluation = evaluateTemperatureState(input);

  assert(evaluation.status == TemperatureStatus::kSensorOffline);
  assert(evaluation.offlineTimeoutElapsed);
}

void testConversionPendingUsesOfflineTimeoutButIsNotFailure() {
  TemperatureStateEvaluationInput input =
      inputWithReading(makeTemperatureConversionPending());
  input.nowMillis = 120000;
  input.lastValidReadingAtMillis = 91000;

  const auto evaluation = evaluateTemperatureState(input);

  assert(!evaluation.sensorFailure);
  assert(!evaluation.statusIsAuthoritative);
  assert(!evaluation.offlineTimeoutElapsed);
}

void testReadIntervalUsesConfiguredValue() {
  const TemperatureConfig config = defaultTemperatureConfig();

  assert(temperatureReadIntervalElapsed(config, 1000, false, 0));
  assert(!temperatureReadIntervalElapsed(config, 5999, true, 1000));
  assert(temperatureReadIntervalElapsed(config, 6000, true, 1000));

  TemperatureConfig custom = config;
  custom.readIntervalMillis = 2500;
  assert(!temperatureReadIntervalElapsed(custom, 3499, true, 1000));
  assert(temperatureReadIntervalElapsed(custom, 3500, true, 1000));
}

void testLocalTemperatureEventContracts() {
  assert(strcmp(temperatureEventName(
                    TemperatureEventType::kTemperatureUpdated),
                "TEMPERATURE_UPDATED") == 0);
  assert(strcmp(temperatureEventName(TemperatureEventType::kTemperatureHigh),
                "TEMPERATURE_HIGH") == 0);
  assert(strcmp(temperatureEventName(TemperatureEventType::kTemperatureLow),
                "TEMPERATURE_LOW") == 0);
  assert(strcmp(temperatureEventName(
                    TemperatureEventType::kTemperatureSensorOffline),
                "TEMPERATURE_SENSOR_OFFLINE") == 0);
  assert(strcmp(temperatureEventName(
                    TemperatureEventType::kTemperatureSensorRecovered),
                "TEMPERATURE_SENSOR_RECOVERED") == 0);
}

}  // namespace

int main() {
  testNormalWithinDefaultLimits();
  testHighMapsAltaConceptToCanonicalHigh();
  testLowMapsBaixaConceptToCanonicalLow();
  testFailureBeforeTimeoutDoesNotBecomeOffline();
  testFailureExactlyAtTimeoutDoesNotBecomeOffline();
  testFailureAfterTimeoutBecomesOffline();
  testOfflineWhenNoValidReadingSinceBoot();
  testConceptualRecoveryAfterOfflineWithValidReading();
  testCustomLimitsFromInMemoryConfig();
  testCustomOfflineTimeout();
  testConversionPendingUsesOfflineTimeoutButIsNotFailure();
  testReadIntervalUsesConfiguredValue();
  testLocalTemperatureEventContracts();
  return 0;
}
