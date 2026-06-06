#pragma once

#include <stddef.h>

#include "contracts/hardware_pins.h"

class Stream;

namespace reeflow::diagnostics {

struct PinoutDiagnosticRow {
  const contracts::HardwarePin* pin;
};

size_t pinoutDiagnosticRowCount();
bool getPinoutDiagnosticRow(size_t index, PinoutDiagnosticRow& row);
void printHardwarePinoutTable(Stream& output);

}  // namespace reeflow::diagnostics
