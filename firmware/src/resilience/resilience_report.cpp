#include "resilience/resilience_report.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "resilience/resilience_scenarios.h"

namespace reeflow::resilience {
namespace {

void copyExpectedRecovery(char* target, ExpectedRecovery recovery) {
  strncpy(target, expectedRecoveryText(recovery),
          kExpectedRecoveryMaxLength - 1);
  target[kExpectedRecoveryMaxLength - 1] = '\0';
}

bool appendText(char* output, size_t outputSize, size_t& used,
                const char* text) {
  const size_t length = strlen(text);
  if (output == nullptr || outputSize == 0 || used + length >= outputSize) {
    return false;
  }

  memcpy(output + used, text, length);
  used += length;
  output[used] = '\0';
  return true;
}

bool appendFormatted(char* output, size_t outputSize, size_t& used,
                     const char* format, ...) {
  if (output == nullptr || outputSize == 0 || used >= outputSize) {
    return false;
  }

  va_list args;
  va_start(args, format);
  const int written =
      vsnprintf(output + used, outputSize - used, format, args);
  va_end(args);

  if (written < 0 || static_cast<size_t>(written) >= outputSize - used) {
    return false;
  }

  used += static_cast<size_t>(written);
  return true;
}

bool isAlertEvent(core::events::EventType type) {
  return type == core::events::EventType::kAlertRaised ||
         type == core::events::EventType::kAlertRecovered ||
         type == core::events::EventType::kAlertCooldownSuppressed ||
         type == core::events::EventType::kAlertHistoryRecorded;
}

size_t alertEventCount(const ResilienceScenarioReport& report) {
  size_t count = 0;
  for (size_t index = 0; index < report.observedEventCount; ++index) {
    if (isAlertEvent(report.observedEvents[index].type)) {
      count += 1;
    }
  }
  return count;
}

}  // namespace

void ResilienceReport::reset() {
  reportCount_ = 0;
  for (size_t index = 0; index < kMaxReportScenarios; ++index) {
    reports_[index] = {};
  }
}

bool ResilienceReport::beginScenario(
    const ResilienceScenarioDefinition& scenario, uint32_t timestampMillis,
    const core::state::SystemState& state) {
  if (mutableScenarioReport(scenario.id) != nullptr) {
    return false;
  }

  if (reportCount_ >= kMaxReportScenarios) {
    return false;
  }

  ResilienceScenarioReport& report = reports_[reportCount_++];
  report = {};
  report.id = scenario.id;
  report.result = ResilienceResult::kNotRun;
  report.severity = scenario.severity;
  report.startedAtMillis = timestampMillis;
  report.initialSnapshot = {timestampMillis, state};
  report.finalSnapshot = {timestampMillis, state};
  copyExpectedRecovery(report.expectedRecovery, scenario.expectedRecovery);
  return true;
}

bool ResilienceReport::recordEvent(ResilienceScenarioId id,
                                   uint32_t timestampMillis,
                                   const core::events::Event& event) {
  ResilienceScenarioReport* report = mutableScenarioReport(id);
  if (report == nullptr ||
      report->observedEventCount >= kMaxScenarioObservedEvents) {
    return false;
  }

  ResilienceObservedEvent& observed =
      report->observedEvents[report->observedEventCount++];
  observed.timestampMillis = timestampMillis;
  observed.type = event.type;
  observed.stateArea = event.stateArea;
  observed.sequence = event.sequence;
  return true;
}

bool ResilienceReport::recordSnapshot(
    ResilienceScenarioId id, uint32_t timestampMillis,
    const core::state::SystemState& state) {
  ResilienceScenarioReport* report = mutableScenarioReport(id);
  if (report == nullptr) {
    return false;
  }

  report->finalSnapshot = {timestampMillis, state};
  return true;
}

bool ResilienceReport::completeScenario(
    ResilienceScenarioId id, ResilienceResult result, uint32_t timestampMillis,
    const core::state::SystemState& state) {
  ResilienceScenarioReport* report = mutableScenarioReport(id);
  if (report == nullptr) {
    return false;
  }

  report->result = result;
  report->finishedAtMillis = timestampMillis;
  report->finalSnapshot = {timestampMillis, state};
  return true;
}

const ResilienceScenarioReport* ResilienceReport::scenarioReport(
    ResilienceScenarioId id) const {
  for (size_t index = 0; index < reportCount_; ++index) {
    if (reports_[index].id == id) {
      return &reports_[index];
    }
  }
  return nullptr;
}

ResilienceReportSummary ResilienceReport::summary() const {
  ResilienceReportSummary summary = {};
  summary.scenarioCount = reportCount_;
  for (size_t index = 0; index < reportCount_; ++index) {
    switch (reports_[index].result) {
      case ResilienceResult::kPassed:
        summary.passedCount += 1;
        break;
      case ResilienceResult::kFailed:
        summary.failedCount += 1;
        break;
      case ResilienceResult::kNotRun:
        summary.notRunCount += 1;
        break;
    }
  }
  return summary;
}

ResilienceScenarioReport* ResilienceReport::mutableScenarioReport(
    ResilienceScenarioId id) {
  for (size_t index = 0; index < reportCount_; ++index) {
    if (reports_[index].id == id) {
      return &reports_[index];
    }
  }
  return nullptr;
}

const char* resilienceResultName(ResilienceResult result) {
  switch (result) {
    case ResilienceResult::kNotRun:
      return "not-run";
    case ResilienceResult::kPassed:
      return "passed";
    case ResilienceResult::kFailed:
      return "failed";
  }
  return "unknown";
}

const char* resilienceSeverityName(ResilienceSeverity severity) {
  switch (severity) {
    case ResilienceSeverity::kInfo:
      return "info";
    case ResilienceSeverity::kWarning:
      return "warning";
    case ResilienceSeverity::kCritical:
      return "critical";
  }
  return "unknown";
}

const char* expectedRecoveryText(ExpectedRecovery recovery) {
  switch (recovery) {
    case ExpectedRecovery::kNoRecoveryRequired:
      return "no local recovery required";
    case ExpectedRecovery::kSafeBoot:
      return "safe local boot state";
    case ExpectedRecovery::kConnectivityRecovered:
      return "connectivity status recovered without blocking local automation";
    case ExpectedRecovery::kSensorRecovered:
      return "sensor status recovered without unintended actuator command";
    case ExpectedRecovery::kAtoFailsafe:
      return "ATO fail-safe timeout and cooldown observed locally";
  }
  return "unknown recovery";
}

bool buildResilienceFinalReport(const ResilienceReport& report,
                                char* output,
                                size_t outputSize) {
  if (output == nullptr || outputSize == 0) {
    return false;
  }

  output[0] = '\0';

  const ResilienceReportSummary summary = report.summary();
  if (summary.scenarioCount != resilienceScenarioCount() ||
      summary.failedCount > 0 || summary.notRunCount > 0) {
    return false;
  }

  for (size_t index = 0; index < resilienceScenarioCount(); ++index) {
    const ResilienceScenarioDefinition& scenario = resilienceScenarioAt(index);
    const ResilienceScenarioReport* scenarioReport =
        report.scenarioReport(scenario.id);
    if (scenarioReport == nullptr ||
        scenarioReport->result != ResilienceResult::kPassed) {
      return false;
    }
  }

  size_t used = 0;
  if (!appendFormatted(output, outputSize, used,
                       "Phase 13 resilience final report\n"
                       "scenarios=%u passed=%u failed=%u not-run=%u\n",
                       static_cast<unsigned>(summary.scenarioCount),
                       static_cast<unsigned>(summary.passedCount),
                       static_cast<unsigned>(summary.failedCount),
                       static_cast<unsigned>(summary.notRunCount))) {
    return false;
  }

  for (size_t index = 0; index < resilienceScenarioCount(); ++index) {
    const ResilienceScenarioDefinition& scenario = resilienceScenarioAt(index);
    const ResilienceScenarioReport* scenarioReport =
        report.scenarioReport(scenario.id);
    if (!appendFormatted(
            output, outputSize, used,
            "scenario=%s result=%s severity=%s start=%u finish=%u "
            "events-observed=%u alerts-observed=%u recovery=%s\n",
            scenario.name, resilienceResultName(scenarioReport->result),
            resilienceSeverityName(scenarioReport->severity),
            static_cast<unsigned>(
                scenarioReport->initialSnapshot.timestampMillis),
            static_cast<unsigned>(
                scenarioReport->finalSnapshot.timestampMillis),
            static_cast<unsigned>(scenarioReport->observedEventCount),
            static_cast<unsigned>(alertEventCount(*scenarioReport)),
            scenarioReport->expectedRecovery)) {
      return false;
    }
  }

  return appendText(output, outputSize, used,
                    "validation-limitations=no hardware validation executed; "
                    "Pendente para Hardware Validation\n");
}

}  // namespace reeflow::resilience
