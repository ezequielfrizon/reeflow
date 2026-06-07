#include "modules/water_level/water_level_service.h"

#include "modules/water_level/water_level_state_evaluator.h"

namespace reeflow::modules::water_level {
namespace {

bool statusRequiresTransitionEvent(core::state::WaterLevelStatus status) {
  return status == core::state::WaterLevelStatus::kLow ||
         status == core::state::WaterLevelStatus::kHigh ||
         status == core::state::WaterLevelStatus::kSensorOffline;
}

WaterLevelEventType eventForStatus(core::state::WaterLevelStatus status) {
  switch (status) {
    case core::state::WaterLevelStatus::kLow:
      return WaterLevelEventType::kWaterLevelLow;
    case core::state::WaterLevelStatus::kHigh:
      return WaterLevelEventType::kWaterLevelHigh;
    case core::state::WaterLevelStatus::kSensorOffline:
      return WaterLevelEventType::kWaterLevelSensorOffline;
    case core::state::WaterLevelStatus::kNormal:
      break;
  }

  return WaterLevelEventType::kWaterLevelUpdated;
}

}  // namespace

WaterLevelService::WaterLevelService(
    WaterLevelSensor& sensor, const config::ConfigManager& configManager,
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
      lastAuthoritativeStatus_(core::state::WaterLevelStatus::kNormal) {}

void WaterLevelService::resetMonitor() {
  monitorStartedAtMillis_ = timeSource_.uptimeMillis();
  hasLastValidReading_ = false;
  lastValidReadingAtMillis_ = 0;
  hasLastAuthoritativeStatus_ = false;
  lastAuthoritativeStatus_ = core::state::WaterLevelStatus::kNormal;
}

bool WaterLevelService::runOnce() {
  const uint32_t nowMillis = timeSource_.uptimeMillis();
  WaterLevelStateEvaluationInput input = {};
  input.sensorReading = sensor_.readLevel();
  input.atoConfig = configManager_.ato();
  input.calibrations = configManager_.calibrations();
  input.moduleConfig = makeDefaultWaterLevelModuleConfig();
  input.nowMillis = nowMillis;
  input.monitorStartedAtMillis = monitorStartedAtMillis_;
  input.hasLastValidReading = hasLastValidReading_;
  input.lastValidReadingAtMillis = lastValidReadingAtMillis_;

  const WaterLevelStateEvaluation evaluation =
      evaluateWaterLevelState(input);

  if (!evaluation.statusIsAuthoritative) {
    return false;
  }

  const bool hadPreviousStatus = hasLastAuthoritativeStatus_;
  const core::state::WaterLevelStatus previousStatus =
      lastAuthoritativeStatus_;

  if (evaluation.hasValidReading) {
    updateStateForValidReading(evaluation.currentLevel, evaluation.status,
                               nowMillis);
    hasLastValidReading_ = true;
    lastValidReadingAtMillis_ = nowMillis;
    publishWaterLevelEvent(WaterLevelEventType::kWaterLevelUpdated);
  } else {
    updateStateForOfflineStatus(evaluation.status);
  }

  publishTransitionEvents(previousStatus, evaluation.status,
                          hadPreviousStatus);
  lastAuthoritativeStatus_ = evaluation.status;
  hasLastAuthoritativeStatus_ = true;
  return true;
}

void WaterLevelService::updateStateForValidReading(
    uint16_t currentLevel, core::state::WaterLevelStatus status,
    uint32_t nowMillis) {
  core::state::WaterLevelState waterLevel =
      core::state::currentSystemState().waterLevel;
  const config::AtoConfig& atoConfig = configManager_.ato();

  waterLevel.currentLevel = currentLevel;
  waterLevel.minimumLevel = atoConfig.minimumLevel;
  waterLevel.maximumLevel = atoConfig.maximumLevel;
  waterLevel.status = status;
  waterLevel.lastUpdate = nowMillis;

  core::state::updateWaterLevelState(waterLevel);
}

void WaterLevelService::updateStateForOfflineStatus(
    core::state::WaterLevelStatus status) {
  core::state::WaterLevelState waterLevel =
      core::state::currentSystemState().waterLevel;
  const config::AtoConfig& atoConfig = configManager_.ato();

  waterLevel.minimumLevel = atoConfig.minimumLevel;
  waterLevel.maximumLevel = atoConfig.maximumLevel;
  waterLevel.status = status;

  core::state::updateWaterLevelState(waterLevel);
}

void WaterLevelService::publishWaterLevelEvent(
    WaterLevelEventType eventType) {
  core::events::Event event = {};
  event.type = waterLevelCoreEventType(eventType);
  event.stateArea = core::events::StateArea::kWaterLevel;
  eventBus_.publish(event);
}

void WaterLevelService::publishTransitionEvents(
    core::state::WaterLevelStatus previousStatus,
    core::state::WaterLevelStatus nextStatus, bool hadPreviousStatus) {
  if (hadPreviousStatus &&
      previousStatus == core::state::WaterLevelStatus::kSensorOffline &&
      nextStatus != core::state::WaterLevelStatus::kSensorOffline) {
    publishWaterLevelEvent(WaterLevelEventType::kWaterLevelSensorRecovered);
  }

  if ((!hadPreviousStatus || previousStatus != nextStatus) &&
      statusRequiresTransitionEvent(nextStatus)) {
    publishWaterLevelEvent(eventForStatus(nextStatus));
  }
}

bool runWaterLevelServiceTask(void* context) {
  WaterLevelService* service = static_cast<WaterLevelService*>(context);
  return service != nullptr && service->runOnce();
}

}  // namespace reeflow::modules::water_level
