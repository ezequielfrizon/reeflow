#include "diagnostics/pinout/pinout_diagnostics.h"

#include <Arduino.h>

namespace reeflow::diagnostics {

void printHardwarePinoutTable(Stream& output) {
  output.println();
  output.println("Hardware V1 Pinout Contract");
  output.println("Group | Canonical Name | Signal | GPIO | Observation");

  for (size_t index = 0; index < pinoutDiagnosticRowCount(); ++index) {
    PinoutDiagnosticRow row = {};
    if (!getPinoutDiagnosticRow(index, row) || row.pin == nullptr) {
      continue;
    }

    output.print(row.pin->group);
    output.print(" | ");
    output.print(row.pin->canonicalName);
    output.print(" | ");
    output.print(row.pin->signal);
    output.print(" | GPIO");
    output.print(row.pin->gpio);
    output.print(" | ");
    output.println(row.pin->observation);
  }
}

}  // namespace reeflow::diagnostics
