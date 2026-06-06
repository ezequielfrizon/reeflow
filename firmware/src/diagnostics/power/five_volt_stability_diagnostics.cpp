#include "diagnostics/power/five_volt_stability_diagnostics.h"

#include "drivers/relays/relay_diagnostic_driver.h"

namespace reeflow::diagnostics {
namespace {

constexpr FiveVoltStabilityChecklistItem kChecklist[] = {
    {"Resting 5V bus", "Measure 5V with all relays commanded OFF",
     "5V resting voltage"},
    {"Relay 1 Recalque load", "Measure 5V while Relay 1 is briefly ON",
     "5V during Relay 1"},
    {"Relay 2 Aquecedor load", "Measure 5V while Relay 2 is briefly ON",
     "5V during Relay 2"},
    {"Relay 3 ATO load", "Measure 5V while Relay 3 is briefly ON",
     "5V during Relay 3"},
    {"Relay 4 Reserva load", "Measure 5V while Relay 4 is briefly ON",
     "5V during Relay 4"},
    {"Controlled relay sequence",
     "Measure 5V while relays are activated one at a time",
     "5V during controlled sequence"},
};

}  // namespace

size_t fiveVoltStabilityChecklistItemCount() {
  return sizeof(kChecklist) / sizeof(kChecklist[0]);
}

bool getFiveVoltStabilityChecklistItem(size_t index,
                                       FiveVoltStabilityChecklistItem& item) {
  if (index >= fiveVoltStabilityChecklistItemCount()) {
    item = {};
    return false;
  }

  item = kChecklist[index];
  return true;
}

FiveVoltRelayStabilitySequenceResult runFiveVoltRelayStabilitySequence(
    drivers::GpioPort& gpio, drivers::BringupTimer& timer,
    unsigned long holdMs) {
  FiveVoltRelayStabilitySequenceResult result = {
      0,
      true,
  };

  drivers::applyAllRelaysOff(gpio);

  const size_t relayCount = drivers::relayDiagnosticCount();
  for (uint8_t relayIndex = 1; relayIndex <= relayCount; ++relayIndex) {
    const drivers::RelayDiagnosticResult relayResult =
        drivers::runIndividualRelayDiagnostic(relayIndex, holdMs, gpio, timer);

    if (relayResult.executed) {
      ++result.relaysTested;
    } else {
      result.allRelayDiagnosticsExecuted = false;
    }
  }

  drivers::applyAllRelaysOff(gpio);
  return result;
}

}  // namespace reeflow::diagnostics
