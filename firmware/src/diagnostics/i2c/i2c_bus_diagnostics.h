#pragma once

#include <Arduino.h>

namespace reeflow::diagnostics {

void printI2cDiagnosticTarget(Stream& output);
void runI2cBusDiagnostic(Stream& serial);

}  // namespace reeflow::diagnostics
