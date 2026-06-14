#pragma once

#include "resilience/resilience_types.h"

namespace reeflow::resilience {

class ResilienceReport {
 public:
  void reset();
  bool beginScenario(const ResilienceScenarioDefinition& scenario,
                     uint32_t timestampMillis,
                     const core::state::SystemState& state);
  bool recordEvent(ResilienceScenarioId id, uint32_t timestampMillis,
                   const core::events::Event& event);
  bool recordSnapshot(ResilienceScenarioId id, uint32_t timestampMillis,
                      const core::state::SystemState& state);
  bool completeScenario(ResilienceScenarioId id, ResilienceResult result,
                        uint32_t timestampMillis,
                        const core::state::SystemState& state);
  const ResilienceScenarioReport* scenarioReport(
      ResilienceScenarioId id) const;
  ResilienceReportSummary summary() const;

 private:
  ResilienceScenarioReport* mutableScenarioReport(ResilienceScenarioId id);

  ResilienceScenarioReport reports_[kMaxReportScenarios] = {};
  size_t reportCount_ = 0;
};

const char* resilienceResultName(ResilienceResult result);
const char* resilienceSeverityName(ResilienceSeverity severity);
const char* expectedRecoveryText(ExpectedRecovery recovery);
bool buildResilienceFinalReport(const ResilienceReport& report,
                                char* output,
                                size_t outputSize);

}  // namespace reeflow::resilience
