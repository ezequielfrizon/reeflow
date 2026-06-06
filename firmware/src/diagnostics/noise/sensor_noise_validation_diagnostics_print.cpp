#include "diagnostics/noise/sensor_noise_validation_diagnostics.h"

#include <Arduino.h>

#include "diagnostics/sensors/vl6180x_diagnostics.h"
#include "drivers/io/arduino_hardware_io.h"

namespace reeflow::diagnostics {
namespace {

const char* ds18b20StatusToText(drivers::Ds18b20DiagnosticStatus status) {
  switch (status) {
    case drivers::Ds18b20DiagnosticStatus::kFound:
      return "FOUND";
    case drivers::Ds18b20DiagnosticStatus::kNotFound:
      return "NOT_FOUND";
    case drivers::Ds18b20DiagnosticStatus::kReadError:
      return "READ_ERROR";
  }

  return "READ_ERROR";
}

const char* boolToLogText(bool value) {
  return value ? "YES" : "NO";
}

void printSnapshot(Stream& output, const SensorNoiseSnapshot& snapshot) {
  output.print("Relay ");
  output.print(snapshot.relayIndex);
  output.print(" | ");
  output.print(sensorNoiseSnapshotPhaseToText(snapshot.phase));
  output.print(" | DS18B20 ");
  output.print(ds18b20StatusToText(snapshot.ds18b20.status));
  output.print(" | DS18B20 C ");
  if (snapshot.ds18b20.status == drivers::Ds18b20DiagnosticStatus::kFound) {
    output.print(snapshot.ds18b20.temperatureC);
  } else {
    output.print("n/a");
  }
  output.print(" | VL6180X ");
  output.print(vl6180xReadStatusToText(snapshot.vl6180x.status));
  output.print(" | VL6180X mm ");
  if (snapshot.vl6180x.status == drivers::Vl6180xDiagnosticReadStatus::kValid) {
    output.print(snapshot.vl6180x.rangeMm);
  } else {
    output.print("n/a");
  }
  output.print(" | communication_loss ");
  output.print(boolToLogText(snapshot.communicationLossObserved));
  output.print(" | reset_observed ");
  output.print(boolToLogText(snapshot.resetObserved));
  output.print(" | gross_oscillation ");
  output.println(boolToLogText(snapshot.grossOscillationObserved));
}

}  // namespace

SensorNoiseValidationResult runSensorNoiseValidationSequence(
    unsigned long relayHoldMs) {
  return runSensorNoiseValidationSequence(
      drivers::arduinoGpioPort(), drivers::arduinoPwmLedcPort(),
      drivers::arduinoBringupTimer(), drivers::arduinoOneWireBus(),
      drivers::arduinoVl6180xDiagnosticSensor(), relayHoldMs,
      drivers::kDs18b20DiagnosticConversionWaitMs);
}

void printSensorNoiseValidationProcedure(Stream& output) {
  output.println();
  output.println("Sensor Noise Validation Procedure");
  output.println("Validation status: Pendente para Hardware Validation");
  output.println("Log format:");
  output.println(
      "Relay | Phase | DS18B20 status/C | VL6180X status/mm | "
      "communication_loss | reset_observed | gross_oscillation");
  output.println("Phases: BEFORE_RELAY, DURING_RELAY, AFTER_RELAY");
  output.println("Real sensor noise/interference: Pendente para Hardware Validation");
}

void runSensorNoiseValidationProcedure(Stream& serial, unsigned long relayHoldMs) {
  serial.println();
  serial.println("Sensor Noise Validation Procedure");
  serial.println("Real noise/interference: Pendente para Hardware Validation");
  printSensorNoiseValidationProcedure(serial);

  const SensorNoiseValidationResult result =
      runSensorNoiseValidationSequence(relayHoldMs);

  for (size_t index = 0; index < result.snapshotsCollected; ++index) {
    printSnapshot(serial, result.snapshots[index]);
  }

  serial.print("Relay cycles: ");
  serial.println(result.relayCycles);
  serial.print("Snapshots collected: ");
  serial.println(result.snapshotsCollected);
  serial.print("Sequence status: ");
  serial.println(result.allRelayCyclesExecuted ? "READY" : "INCOMPLETE");
  serial.println("All relays commanded to physical OFF.");
  serial.println("All PWM channels commanded to duty zero.");
  serial.println("Communication loss/reset/gross oscillation real validation: "
                 "Pendente para Hardware Validation");
}

}  // namespace reeflow::diagnostics
