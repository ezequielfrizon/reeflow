#include "resilience/resilience_scenarios.h"

#include <string.h>

namespace reeflow::resilience {
namespace {

ResilienceStepDefinition step(ResilienceStepKind kind,
                              SimulatedFailure failure,
                              ExpectedRecovery recovery,
                              const char* description) {
  ResilienceStepDefinition definition = {};
  definition.kind = kind;
  definition.failure = failure;
  definition.expectedRecovery = recovery;
  strncpy(definition.description, description,
          kStepDescriptionMaxLength - 1);
  definition.description[kStepDescriptionMaxLength - 1] = '\0';
  return definition;
}

ResilienceScenarioDefinition scenario(
    ResilienceScenarioId id, const char* name, SimulatedFailure failure,
    ExpectedRecovery recovery, ResilienceSeverity severity,
    const ResilienceStepDefinition* steps, size_t stepCount) {
  ResilienceScenarioDefinition definition = {};
  definition.id = id;
  strncpy(definition.name, name, kScenarioNameMaxLength - 1);
  definition.name[kScenarioNameMaxLength - 1] = '\0';
  definition.primaryFailure = failure;
  definition.expectedRecovery = recovery;
  definition.severity = severity;
  definition.stepCount = stepCount > kMaxScenarioSteps ? kMaxScenarioSteps
                                                       : stepCount;
  for (size_t index = 0; index < definition.stepCount; ++index) {
    definition.steps[index] = steps[index];
  }
  return definition;
}

const ResilienceStepDefinition kUnexpectedRebootSteps[] = {
    step(ResilienceStepKind::kArrange, SimulatedFailure::kNone,
         ExpectedRecovery::kSafeBoot, "prepare in-memory firmware context"),
    step(ResilienceStepKind::kInjectFailure,
         SimulatedFailure::kUnexpectedReboot, ExpectedRecovery::kSafeBoot,
         "mark unexpected reboot without physical reset"),
    step(ResilienceStepKind::kObserveState, SimulatedFailure::kNone,
         ExpectedRecovery::kSafeBoot, "inspect safe System State after boot"),
    step(ResilienceStepKind::kReport, SimulatedFailure::kNone,
         ExpectedRecovery::kSafeBoot, "record deterministic boot report"),
};

const ResilienceStepDefinition kPowerLossSteps[] = {
    step(ResilienceStepKind::kArrange, SimulatedFailure::kNone,
         ExpectedRecovery::kSafeBoot, "prepare persisted local state"),
    step(ResilienceStepKind::kInjectFailure, SimulatedFailure::kPowerCycle,
         ExpectedRecovery::kSafeBoot, "replace context with simulated boot"),
    step(ResilienceStepKind::kObserveState, SimulatedFailure::kNone,
         ExpectedRecovery::kSafeBoot, "verify volatile state is not reused"),
    step(ResilienceStepKind::kReport, SimulatedFailure::kNone,
         ExpectedRecovery::kSafeBoot, "record deterministic power report"),
};

const ResilienceStepDefinition kWifiLossSteps[] = {
    step(ResilienceStepKind::kArrange, SimulatedFailure::kNone,
         ExpectedRecovery::kConnectivityRecovered, "start with Wi-Fi online"),
    step(ResilienceStepKind::kInjectFailure,
         SimulatedFailure::kWifiDisconnected,
         ExpectedRecovery::kConnectivityRecovered, "simulate Wi-Fi loss"),
    step(ResilienceStepKind::kRunScheduler, SimulatedFailure::kNone,
         ExpectedRecovery::kConnectivityRecovered, "run local scheduler cycles"),
    step(ResilienceStepKind::kRecover, SimulatedFailure::kNone,
         ExpectedRecovery::kConnectivityRecovered, "simulate Wi-Fi recovery"),
};

const ResilienceStepDefinition kMqttLossSteps[] = {
    step(ResilienceStepKind::kArrange, SimulatedFailure::kNone,
         ExpectedRecovery::kConnectivityRecovered, "start with MQTT online"),
    step(ResilienceStepKind::kInjectFailure,
         SimulatedFailure::kMqttDisconnected,
         ExpectedRecovery::kConnectivityRecovered, "simulate MQTT loss"),
    step(ResilienceStepKind::kRunScheduler, SimulatedFailure::kNone,
         ExpectedRecovery::kConnectivityRecovered, "run local scheduler cycles"),
    step(ResilienceStepKind::kRecover, SimulatedFailure::kNone,
         ExpectedRecovery::kConnectivityRecovered, "simulate MQTT recovery"),
};

const ResilienceStepDefinition kTemperatureOfflineSteps[] = {
    step(ResilienceStepKind::kArrange, SimulatedFailure::kNone,
         ExpectedRecovery::kSensorRecovered, "start with temperature online"),
    step(ResilienceStepKind::kInjectFailure,
         SimulatedFailure::kTemperatureSensorOffline,
         ExpectedRecovery::kSensorRecovered, "simulate DS18B20 offline"),
    step(ResilienceStepKind::kObserveEvents, SimulatedFailure::kNone,
         ExpectedRecovery::kSensorRecovered, "capture local sensor events"),
    step(ResilienceStepKind::kRecover, SimulatedFailure::kNone,
         ExpectedRecovery::kSensorRecovered, "simulate temperature recovery"),
};

const ResilienceStepDefinition kWaterLevelOfflineSteps[] = {
    step(ResilienceStepKind::kArrange, SimulatedFailure::kNone,
         ExpectedRecovery::kSensorRecovered, "start with level sensor online"),
    step(ResilienceStepKind::kInjectFailure,
         SimulatedFailure::kWaterLevelSensorOffline,
         ExpectedRecovery::kSensorRecovered, "simulate VL6180X offline"),
    step(ResilienceStepKind::kObserveEvents, SimulatedFailure::kNone,
         ExpectedRecovery::kSensorRecovered, "capture local level events"),
    step(ResilienceStepKind::kRecover, SimulatedFailure::kNone,
         ExpectedRecovery::kSensorRecovered, "simulate level recovery"),
};

const ResilienceStepDefinition kAtoStuckSteps[] = {
    step(ResilienceStepKind::kArrange, SimulatedFailure::kNone,
         ExpectedRecovery::kAtoFailsafe, "start with ATO allowed locally"),
    step(ResilienceStepKind::kInjectFailure, SimulatedFailure::kAtoStuck,
         ExpectedRecovery::kAtoFailsafe, "simulate level not recovering"),
    step(ResilienceStepKind::kRunScheduler, SimulatedFailure::kNone,
         ExpectedRecovery::kAtoFailsafe, "run timeout and cooldown cycles"),
    step(ResilienceStepKind::kObserveState, SimulatedFailure::kNone,
         ExpectedRecovery::kAtoFailsafe, "inspect ATO fail-safe state"),
};

const ResilienceScenarioDefinition kScenarios[] = {
    scenario(ResilienceScenarioId::kUnexpectedReboot, "unexpected reboot",
             SimulatedFailure::kUnexpectedReboot, ExpectedRecovery::kSafeBoot,
             ResilienceSeverity::kCritical, kUnexpectedRebootSteps,
             sizeof(kUnexpectedRebootSteps) / sizeof(kUnexpectedRebootSteps[0])),
    scenario(ResilienceScenarioId::kSimulatedPowerLoss,
             "simulated power loss", SimulatedFailure::kPowerCycle,
             ExpectedRecovery::kSafeBoot, ResilienceSeverity::kCritical,
             kPowerLossSteps,
             sizeof(kPowerLossSteps) / sizeof(kPowerLossSteps[0])),
    scenario(ResilienceScenarioId::kWifiLoss, "Wi-Fi loss",
             SimulatedFailure::kWifiDisconnected,
             ExpectedRecovery::kConnectivityRecovered,
             ResilienceSeverity::kWarning, kWifiLossSteps,
             sizeof(kWifiLossSteps) / sizeof(kWifiLossSteps[0])),
    scenario(ResilienceScenarioId::kMqttLoss, "MQTT loss",
             SimulatedFailure::kMqttDisconnected,
             ExpectedRecovery::kConnectivityRecovered,
             ResilienceSeverity::kWarning, kMqttLossSteps,
             sizeof(kMqttLossSteps) / sizeof(kMqttLossSteps[0])),
    scenario(ResilienceScenarioId::kTemperatureSensorOffline,
             "temperature sensor offline",
             SimulatedFailure::kTemperatureSensorOffline,
             ExpectedRecovery::kSensorRecovered, ResilienceSeverity::kWarning,
             kTemperatureOfflineSteps,
             sizeof(kTemperatureOfflineSteps) /
                 sizeof(kTemperatureOfflineSteps[0])),
    scenario(ResilienceScenarioId::kWaterLevelSensorOffline,
             "water level sensor offline",
             SimulatedFailure::kWaterLevelSensorOffline,
             ExpectedRecovery::kSensorRecovered, ResilienceSeverity::kWarning,
             kWaterLevelOfflineSteps,
             sizeof(kWaterLevelOfflineSteps) /
                 sizeof(kWaterLevelOfflineSteps[0])),
    scenario(ResilienceScenarioId::kAtoStuck, "ATO stuck",
             SimulatedFailure::kAtoStuck, ExpectedRecovery::kAtoFailsafe,
             ResilienceSeverity::kCritical, kAtoStuckSteps,
             sizeof(kAtoStuckSteps) / sizeof(kAtoStuckSteps[0])),
};

}  // namespace

size_t resilienceScenarioCount() {
  return sizeof(kScenarios) / sizeof(kScenarios[0]);
}

const ResilienceScenarioDefinition& resilienceScenarioAt(size_t index) {
  return kScenarios[index];
}

const ResilienceScenarioDefinition* findResilienceScenario(
    ResilienceScenarioId id) {
  for (size_t index = 0; index < resilienceScenarioCount(); ++index) {
    if (kScenarios[index].id == id) {
      return &kScenarios[index];
    }
  }
  return nullptr;
}

const char* resilienceScenarioName(ResilienceScenarioId id) {
  const ResilienceScenarioDefinition* scenario = findResilienceScenario(id);
  return scenario == nullptr ? "unknown" : scenario->name;
}

}  // namespace reeflow::resilience
