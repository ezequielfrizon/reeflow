#include "diagnostics/pinout/pinout_diagnostics.h"

namespace reeflow::diagnostics {

size_t pinoutDiagnosticRowCount() {
  return contracts::kHardwareV1PinCount;
}

bool getPinoutDiagnosticRow(size_t index, PinoutDiagnosticRow& row) {
  if (index >= contracts::kHardwareV1PinCount) {
    row = {};
    return false;
  }

  row = {&contracts::kHardwareV1Pins[index]};
  return true;
}
}  // namespace reeflow::diagnostics
