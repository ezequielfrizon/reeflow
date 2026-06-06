#pragma once

#include <stdint.h>

#include "config/config_manager.h"
#include "core/state/system_state.h"
#include "modules/temperature/temperature_sensor.h"

namespace reeflow::modules::temperature {

struct TemperatureStateEvaluationInput {
  TemperatureSensorReading sensorReading;
  config::TemperatureConfig config;
  uint32_t nowMillis;
  uint32_t monitorStartedAtMillis;
  bool hasLastValidReading;
  uint32_t lastValidReadingAtMillis;
};

struct TemperatureStateEvaluation {
  core::state::TemperatureStatus status;
  bool statusIsAuthoritative;
  bool hasValidReading;
  float temperatureCelsius;
  bool sensorFailure;
  bool offlineTimeoutElapsed;
};

TemperatureStateEvaluation evaluateTemperatureState(
    const TemperatureStateEvaluationInput& input);

bool temperatureReadIntervalElapsed(const config::TemperatureConfig& config,
                                    uint32_t nowMillis,
                                    bool hasLastReadAttempt,
                                    uint32_t lastReadAttemptAtMillis);

}  // namespace reeflow::modules::temperature
