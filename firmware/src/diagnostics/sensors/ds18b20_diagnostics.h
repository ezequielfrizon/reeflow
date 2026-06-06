#pragma once

#include <Arduino.h>

namespace reeflow::diagnostics {

void printDs18b20DiagnosticTarget(Stream& output);
void runDs18b20Diagnostic(Stream& serial);

}  // namespace reeflow::diagnostics
