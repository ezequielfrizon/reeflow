#include "diagnostics/consolidation/hardware_bringup_consolidation.h"

#include <Arduino.h>

#include "drivers/io/arduino_hardware_io.h"

namespace reeflow::diagnostics {

HardwareBringupConsolidationSummary runHardwareBringupConsolidation() {
  return runHardwareBringupConsolidation(drivers::arduinoGpioPort(),
                                         drivers::arduinoPwmLedcPort());
}

void printHardwareBringupConsolidationSummary(Stream& output) {
  const HardwareBringupConsolidationSummary summary =
      buildHardwareBringupConsolidationSummary(
          defaultHardwareBringupConsolidationInputs());

  output.println();
  output.println("Phase 1 Bring-Up Consolidation Summary");
  output.println("Group | Readiness | Physical validation | Detail");

  for (size_t index = 0; index < summary.groupCount; ++index) {
    const HardwareBringupConsolidationGroup& group = summary.groups[index];
    output.print(group.group);
    output.print(" | ");
    output.print(bringupReadinessStatusToText(group.readiness));
    output.print(" | ");
    output.print(
        bringupPhysicalValidationStatusToText(group.physicalValidation));
    output.print(" | ");
    output.println(group.detail);
  }
}

void runHardwareBringupConsolidation(Stream& serial) {
  const HardwareBringupConsolidationSummary summary =
      runHardwareBringupConsolidation();

  serial.println();
  serial.println("Phase 1 Bring-Up Consolidation Summary");
  serial.println("Group | Readiness | Physical validation | Detail");

  for (size_t index = 0; index < summary.groupCount; ++index) {
    const HardwareBringupConsolidationGroup& group = summary.groups[index];
    serial.print(group.group);
    serial.print(" | ");
    serial.print(bringupReadinessStatusToText(group.readiness));
    serial.print(" | ");
    serial.print(
        bringupPhysicalValidationStatusToText(group.physicalValidation));
    serial.print(" | ");
    serial.println(group.detail);
  }

  serial.print("Relays safe command: ");
  serial.println(summary.relaysCommandedSafe ? "READY" : "NOT_IMPLEMENTED");
  serial.print("PWM safe command: ");
  serial.println(summary.pwmCommandedSafe ? "READY" : "NOT_IMPLEMENTED");
  serial.println("Physical validations: PENDING_HARDWARE_VALIDATION");
}

}  // namespace reeflow::diagnostics
