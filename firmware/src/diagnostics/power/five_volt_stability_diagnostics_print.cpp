#include "diagnostics/power/five_volt_stability_diagnostics.h"

#include <Arduino.h>

#include "drivers/io/arduino_hardware_io.h"

namespace reeflow::diagnostics {

FiveVoltRelayStabilitySequenceResult runFiveVoltRelayStabilitySequence(
    unsigned long holdMs) {
  return runFiveVoltRelayStabilitySequence(drivers::arduinoGpioPort(),
                                           drivers::arduinoBringupTimer(),
                                           holdMs);
}

void printFiveVoltStabilityChecklist(Stream& output) {
  output.println();
  output.println("5V Bus Stability Validation Procedure");
  output.println("Validation status: Pendente para Hardware Validation");
  output.println("Checkpoint | Measurement target | Report field");

  for (size_t index = 0; index < fiveVoltStabilityChecklistItemCount(); ++index) {
    FiveVoltStabilityChecklistItem item = {};
    if (!getFiveVoltStabilityChecklistItem(index, item)) {
      continue;
    }

    output.print(item.checkpoint);
    output.print(" | ");
    output.print(item.measurementTarget);
    output.print(" | ");
    output.println(item.reportField);
  }
}

void runFiveVoltStabilityProcedure(Stream& serial, unsigned long holdMs) {
  serial.println();
  serial.println("5V Bus Stability Procedure");
  serial.println("Measure the 5V bus externally at each checkpoint.");
  serial.println("Real measurements: Pendente para Hardware Validation");
  printFiveVoltStabilityChecklist(serial);

  const FiveVoltRelayStabilitySequenceResult result =
      runFiveVoltRelayStabilitySequence(holdMs);

  serial.print("Relays exercised one at a time: ");
  serial.println(result.relaysTested);
  serial.print("Sequence status: ");
  serial.println(result.allRelayDiagnosticsExecuted ? "READY" : "INCOMPLETE");
  serial.println("All relays commanded to physical OFF.");
  serial.println("5V measurement result: Pendente para Hardware Validation");
}

}  // namespace reeflow::diagnostics
