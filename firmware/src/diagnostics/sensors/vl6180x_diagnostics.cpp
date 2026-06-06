#include "diagnostics/sensors/vl6180x_diagnostics.h"

#include "contracts/i2c_contract.h"

namespace reeflow::diagnostics {

const char* vl6180xReadStatusToText(
    drivers::Vl6180xDiagnosticReadStatus status) {
  switch (status) {
    case drivers::Vl6180xDiagnosticReadStatus::kValid:
      return "VALID";
    case drivers::Vl6180xDiagnosticReadStatus::kReadError:
      return "READ_ERROR";
  }

  return "READ_ERROR";
}

void printVl6180xDiagnosticTarget(Stream& output) {
  output.println();
  output.println("VL6180X Basic Read Diagnostic Target");
  output.print("Expected I2C address: 0x");
  output.println(contracts::kVl6180xI2cContract.expectedAddress, HEX);
  output.println("Physical read: Pendente para Hardware Validation");
  output.println("Distance variation check: Pendente para Hardware Validation");
  output.println("Idle stability check: Pendente para Hardware Validation");
}

void runVl6180xDiagnostic(Stream& serial) {
  serial.println();
  serial.println("VL6180X Basic Read Diagnostic");
  serial.print("Expected I2C address: 0x");
  serial.println(contracts::kVl6180xI2cContract.expectedAddress, HEX);

  const drivers::Vl6180xDiagnosticReadResult result =
      drivers::runVl6180xDiagnosticRead(
          contracts::kVl6180xI2cContract.expectedAddress);

  serial.print("Status: ");
  serial.println(vl6180xReadStatusToText(result.status));

  if (result.status == drivers::Vl6180xDiagnosticReadStatus::kValid) {
    serial.print("Raw range: ");
    serial.println(result.rawRange);
    serial.print("Converted range mm: ");
    serial.println(result.rangeMm);
  } else {
    serial.println("Physical read: Pendente para Hardware Validation");
  }

  serial.println("Distance variation check: Pendente para Hardware Validation");
  serial.println("Idle stability check: Pendente para Hardware Validation");
}

}  // namespace reeflow::diagnostics
