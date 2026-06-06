#include "modules/temperature/temperature_state_evaluator.h"

namespace reeflow::modules::temperature {
namespace {

bool elapsedMoreThan(uint32_t nowMillis, uint32_t startedAtMillis,
                     uint32_t intervalMillis) {
  return static_cast<uint32_t>(nowMillis - startedAtMillis) > intervalMillis;
}

core::state::TemperatureStatus classifyValidTemperature(
    float temperatureCelsius, const config::TemperatureConfig& config) {
  if (temperatureCelsius > config.maxTemperature) {
    return core::state::TemperatureStatus::kHigh;
  }

  if (temperatureCelsius < config.minTemperature) {
    return core::state::TemperatureStatus::kLow;
  }

  return core::state::TemperatureStatus::kNormal;
}

bool isSensorFailure(TemperatureSensorReadStatus status) {
  return status == TemperatureSensorReadStatus::kSensorNotFound ||
         status == TemperatureSensorReadStatus::kReadError;
}

}  // namespace

TemperatureStateEvaluation evaluateTemperatureState(
    const TemperatureStateEvaluationInput& input) {
  TemperatureStateEvaluation evaluation = {};
  evaluation.status = core::state::TemperatureStatus::kNormal;
  evaluation.temperatureCelsius = input.sensorReading.temperatureCelsius;
  evaluation.sensorFailure = isSensorFailure(input.sensorReading.status);

  if (input.sensorReading.status == TemperatureSensorReadStatus::kValid) {
    evaluation.status =
        classifyValidTemperature(input.sensorReading.temperatureCelsius,
                                 input.config);
    evaluation.statusIsAuthoritative = true;
    evaluation.hasValidReading = true;
    return evaluation;
  }

  const uint32_t offlineReferenceMillis =
      input.hasLastValidReading ? input.lastValidReadingAtMillis
                                : input.monitorStartedAtMillis;
  evaluation.offlineTimeoutElapsed =
      elapsedMoreThan(input.nowMillis, offlineReferenceMillis,
                      input.config.sensorOfflineTimeoutMillis);

  if (evaluation.offlineTimeoutElapsed) {
    evaluation.status = core::state::TemperatureStatus::kSensorOffline;
    evaluation.statusIsAuthoritative = true;
  }

  return evaluation;
}

bool temperatureReadIntervalElapsed(const config::TemperatureConfig& config,
                                    uint32_t nowMillis,
                                    bool hasLastReadAttempt,
                                    uint32_t lastReadAttemptAtMillis) {
  if (!hasLastReadAttempt) {
    return true;
  }

  return static_cast<uint32_t>(nowMillis - lastReadAttemptAtMillis) >=
         config.readIntervalMillis;
}

}  // namespace reeflow::modules::temperature
