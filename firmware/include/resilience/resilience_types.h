#pragma once

#include <stddef.h>
#include <stdint.h>

#include "core/events/event_bus.h"
#include "core/state/system_state.h"

namespace reeflow::resilience {

constexpr size_t kScenarioNameMaxLength = 48;
constexpr size_t kStepDescriptionMaxLength = 72;
constexpr size_t kExpectedRecoveryMaxLength = 96;
constexpr size_t kMaxScenarioSteps = 8;
constexpr size_t kMaxScenarioObservedEvents = 12;
constexpr size_t kMaxReportScenarios = 8;

enum class ResilienceScenarioId {
  kUnexpectedReboot,
  kSimulatedPowerLoss,
  kWifiLoss,
  kMqttLoss,
  kTemperatureSensorOffline,
  kWaterLevelSensorOffline,
  kAtoStuck,
};

enum class ResilienceStepKind {
  kArrange,
  kInjectFailure,
  kRunScheduler,
  kRecover,
  kObserveState,
  kObserveEvents,
  kReport,
};

enum class SimulatedFailure {
  kNone,
  kUnexpectedReboot,
  kPowerCycle,
  kWifiDisconnected,
  kMqttDisconnected,
  kTemperatureSensorOffline,
  kWaterLevelSensorOffline,
  kAtoStuck,
};

enum class ExpectedRecovery {
  kNoRecoveryRequired,
  kSafeBoot,
  kConnectivityRecovered,
  kSensorRecovered,
  kAtoFailsafe,
};

enum class ResilienceResult {
  kNotRun,
  kPassed,
  kFailed,
};

enum class ResilienceSeverity {
  kInfo,
  kWarning,
  kCritical,
};

struct ResilienceStepDefinition {
  ResilienceStepKind kind;
  SimulatedFailure failure;
  ExpectedRecovery expectedRecovery;
  char description[kStepDescriptionMaxLength];
};

struct ResilienceScenarioDefinition {
  ResilienceScenarioId id;
  char name[kScenarioNameMaxLength];
  SimulatedFailure primaryFailure;
  ExpectedRecovery expectedRecovery;
  ResilienceSeverity severity;
  ResilienceStepDefinition steps[kMaxScenarioSteps];
  size_t stepCount;
};

struct ResilienceStateSnapshot {
  uint32_t timestampMillis;
  core::state::SystemState state;
};

struct ResilienceObservedEvent {
  uint32_t timestampMillis;
  core::events::EventType type;
  core::events::StateArea stateArea;
  uint32_t sequence;
};

struct ResilienceScenarioReport {
  ResilienceScenarioId id;
  ResilienceResult result;
  ResilienceSeverity severity;
  uint32_t startedAtMillis;
  uint32_t finishedAtMillis;
  ResilienceStateSnapshot initialSnapshot;
  ResilienceStateSnapshot finalSnapshot;
  ResilienceObservedEvent observedEvents[kMaxScenarioObservedEvents];
  size_t observedEventCount;
  char expectedRecovery[kExpectedRecoveryMaxLength];
};

struct ResilienceReportSummary {
  size_t scenarioCount;
  size_t passedCount;
  size_t failedCount;
  size_t notRunCount;
};

}  // namespace reeflow::resilience
