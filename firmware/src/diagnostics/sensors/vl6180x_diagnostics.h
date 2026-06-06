#pragma once

#include <Arduino.h>

#include "drivers/sensors/vl6180x_diagnostic_driver.h"

namespace reeflow::diagnostics {

const char* vl6180xReadStatusToText(
    drivers::Vl6180xDiagnosticReadStatus status);
void printVl6180xDiagnosticTarget(Stream& output);
void runVl6180xDiagnostic(Stream& serial);

}  // namespace reeflow::diagnostics
