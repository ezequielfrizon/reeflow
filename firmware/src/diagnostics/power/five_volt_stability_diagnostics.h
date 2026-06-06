#pragma once

#include <stddef.h>

#include "drivers/io/bringup_timer.h"
#include "drivers/io/gpio_interface.h"

class Stream;

namespace reeflow::diagnostics {

struct FiveVoltStabilityChecklistItem {
  const char* checkpoint;
  const char* measurementTarget;
  const char* reportField;
};

struct FiveVoltRelayStabilitySequenceResult {
  size_t relaysTested;
  bool allRelayDiagnosticsExecuted;
};

size_t fiveVoltStabilityChecklistItemCount();
bool getFiveVoltStabilityChecklistItem(size_t index,
                                       FiveVoltStabilityChecklistItem& item);
FiveVoltRelayStabilitySequenceResult runFiveVoltRelayStabilitySequence(
    drivers::GpioPort& gpio, drivers::BringupTimer& timer,
    unsigned long holdMs);
FiveVoltRelayStabilitySequenceResult runFiveVoltRelayStabilitySequence(
    unsigned long holdMs);
void printFiveVoltStabilityChecklist(Stream& output);
void runFiveVoltStabilityProcedure(Stream& serial, unsigned long holdMs);

}  // namespace reeflow::diagnostics
