#pragma once

#include <Arduino.h>

namespace reeflow::diagnostics {

void printPwmBringupConfiguration(Stream& output);
void runPwmBringupDiagnostic(Stream& serial, unsigned long holdMs);

}  // namespace reeflow::diagnostics
