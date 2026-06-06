#include "modules/temperature/temperature_service.h"

#include "modules/temperature/temperature_events.h"
#include "modules/temperature/temperature_state_evaluator.h"

namespace reeflow::modules::temperature {
namespace {

bool statusRequiresTransitionEvent(core::state::TemperatureStatus status) {
  return status == core::state::TemperatureStatus::kHigh ||
         status == core::state::TemperatureStatus::kLow ||
         status == core::state::TemperatureStatus::kSensorOffline;
}

TemperatureEventType eventForStatus(core::state::TemperatureStatus status) {
  switch (status) {
    case core::state::TemperatureStatus::kHigh:
      return TemperatureEventType::kTemperatureHigh;
    case core::state::TemperatureStatus::kLow:
      return TemperatureEventType::kTemperatureLow;
    case core::state::TemperatureStatus::kSensorOffline:
      return TemperatureEventType::kTemperatureSensorOffline;
    case core::state::TemperatureStatus::kNormal:
      break;
  }

  return TemperatureEventType::kTemperatureUpdated;
}

}  // namespace

TemperatureService::TemperatureService(
    TemperatureSensor& sensor, const config::ConfigManager& configManager,
    const core::platform::TimeSource& timeSource,
    core::events::EventBus& eventBus)
    : sensor_(sensor),
      configManager_(configManager),
      timeSource_(timeSource),
      eventBus_(eventBus),
      monitorStartedAtMillis_(timeSource.uptimeMillis()),
      hasLastValidReading_(false),
      lastValidReadingAtMillis_(0),
      hasLastAuthoritativeStatus_(false),
      lastAuthoritativeStatus_(core::state::TemperatureStatus::kNormal) {}

void TemperatureService::resetMonitor() {
  monitorStartedAtMillis_ = timeSource_.uptimeMillis();
  hasLastValidReading_ = false;
  lastValidReadingAtMillis_ = 0;
  hasLastAuthoritativeStatus_ = false;
  lastAuthoritativeStatus_ = core::state::TemperatureStatus::kNormal;
}

bool TemperatureService::runOnce() {
  const uint32_t nowMillis = timeSource_.uptimeMillis();
  TemperatureStateEvaluationInput input = {};
  input.sensorReading = sensor_.readTemperature();
  input.config = configManager_.temperature();
  input.nowMillis = nowMillis;
  input.monitorStartedAtMillis = monitorStartedAtMillis_;
  input.hasLastValidReading = hasLastValidReading_;
  input.lastValidReadingAtMillis = lastValidReadingAtMillis_;

  const TemperatureStateEvaluation evaluation =
      evaluateTemperatureState(input);

  if (!evaluation.statusIsAuthoritative) {
    if (input.sensorReading.status ==
        TemperatureSensorReadStatus::kConversionPending) {
      return true;
    }

    return false;
  }

  const bool hadPreviousStatus = hasLastAuthoritativeStatus_;
  const core::state::TemperatureStatus previousStatus =
      lastAuthoritativeStatus_;

  if (evaluation.hasValidReading) {
    updateStateForValidReading(evaluation.temperatureCelsius,
                               evaluation.status, nowMillis);
    hasLastValidReading_ = true;
    lastValidReadingAtMillis_ = nowMillis;
    publishTemperatureEvent(TemperatureEventType::kTemperatureUpdated);
  } else {
    updateStateForOfflineStatus(evaluation.status);
  }

  publishTransitionEvents(previousStatus, evaluation.status,
                          hadPreviousStatus);
  lastAuthoritativeStatus_ = evaluation.status;
  hasLastAuthoritativeStatus_ = true;
  return true;
}

void TemperatureService::updateStateForValidReading(
    float temperatureCelsius, core::state::TemperatureStatus status,
    uint32_t nowMillis) {
  core::state::TemperatureState temperature =
      core::state::currentSystemState().temperature;
  const config::TemperatureConfig& config = configManager_.temperature();

  temperature.currentTemperature = temperatureCelsius;
  temperature.minTemperature = config.minTemperature;
  temperature.maxTemperature = config.maxTemperature;
  temperature.status = status;
  temperature.lastUpdate = nowMillis;

  core::state::updateTemperatureState(temperature);
}

void TemperatureService::updateStateForOfflineStatus(
    core::state::TemperatureStatus status) {
  core::state::TemperatureState temperature =
      core::state::currentSystemState().temperature;
  const config::TemperatureConfig& config = configManager_.temperature();

  temperature.minTemperature = config.minTemperature;
  temperature.maxTemperature = config.maxTemperature;
  temperature.status = status;

  core::state::updateTemperatureState(temperature);
}

void TemperatureService::publishTemperatureEvent(
    TemperatureEventType eventType) {
  core::events::Event event = {};
  event.type = temperatureCoreEventType(eventType);
  event.stateArea = core::events::StateArea::kTemperature;
  eventBus_.publish(event);
}

void TemperatureService::publishTransitionEvents(
    core::state::TemperatureStatus previousStatus,
    core::state::TemperatureStatus nextStatus, bool hadPreviousStatus) {
  if (hadPreviousStatus &&
      previousStatus == core::state::TemperatureStatus::kSensorOffline &&
      nextStatus != core::state::TemperatureStatus::kSensorOffline) {
    publishTemperatureEvent(TemperatureEventType::kTemperatureSensorRecovered);
  }

  if ((!hadPreviousStatus || previousStatus != nextStatus) &&
      statusRequiresTransitionEvent(nextStatus)) {
    publishTemperatureEvent(eventForStatus(nextStatus));
  }
}

bool runTemperatureServiceTask(void* context) {
  TemperatureService* service = static_cast<TemperatureService*>(context);
  return service != nullptr && service->runOnce();
}

}  // namespace reeflow::modules::temperature
