#include "diagnostics/i2c/i2c_bus_diagnostics.h"

#include "contracts/hardware_pins.h"
#include "contracts/i2c_contract.h"
#include "diagnostics/i2c/vl6180x_i2c_address_diagnostics.h"
#include "diagnostics/sensors/vl6180x_diagnostics.h"
#include "drivers/i2c/i2c_diagnostic_bus.h"

namespace reeflow::diagnostics {
namespace {

struct I2cDiagnosticPins {
  const contracts::HardwarePin* sda;
  const contracts::HardwarePin* scl;
};

I2cDiagnosticPins i2cPins() {
  I2cDiagnosticPins pins = {};

  for (size_t index = 0; index < contracts::kHardwareV1PinCount; ++index) {
    const contracts::HardwarePin& pin = contracts::kHardwareV1Pins[index];
    if (pin.role != contracts::HardwarePinRole::kI2cBus) {
      continue;
    }

    if (pin.gpio == 21) {
      pins.sda = &pin;
    } else if (pin.gpio == 22) {
      pins.scl = &pin;
    }
  }

  return pins;
}

void printI2cAddressValue(Stream& output, uint8_t address) {
  output.print("0x");
  if (address < 0x10) {
    output.print("0");
  }
  output.print(address, HEX);
}

}  // namespace

void printI2cDiagnosticTarget(Stream& output) {
  const I2cDiagnosticPins pins = i2cPins();

  output.println();
  output.println("I2C Bus Diagnostic Target");
  if (pins.sda == nullptr || pins.scl == nullptr) {
    output.println("Status: READ_ERROR");
    output.println("Reason: I2C SDA/SCL GPIOs not found in hardware contract.");
    return;
  }

  output.print("SDA GPIO: ");
  output.println(pins.sda->gpio);
  output.print("SCL GPIO: ");
  output.println(pins.scl->gpio);
  output.print("Expected VL6180X address: ");
  printI2cAddressValue(output, contracts::kVl6180xI2cContract.expectedAddress);
  output.println();
  output.println("VL6180X physical detection: Pendente para Hardware Validation");
  output.println("VL6180X physical address confirmation: Pendente para Hardware Validation");
  output.println("VL6180X physical read: Pendente para Hardware Validation");
  output.println("SDA/SCL physical verification: Pendente para Hardware Validation");
}

void runI2cBusDiagnostic(Stream& serial) {
  const I2cDiagnosticPins pins = i2cPins();

  serial.println();
  serial.println("I2C Bus Bring-Up Diagnostic");
  if (pins.sda == nullptr || pins.scl == nullptr) {
    serial.println("Status: READ_ERROR");
    serial.println("I2C SDA/SCL GPIOs not found in hardware contract.");
    return;
  }

  const drivers::I2cScanResult result =
      drivers::runI2cDiagnosticScan(pins.sda->gpio, pins.scl->gpio);

  serial.print("SDA GPIO: ");
  serial.println(result.sdaGpio);
  serial.print("SCL GPIO: ");
  serial.println(result.sclGpio);
  serial.print("Devices found: ");
  serial.println(result.deviceCount);

  if (result.deviceCount == 0) {
    serial.println("No I2C devices reported by scanner.");
  } else {
    for (size_t index = 0; index < result.deviceCount; ++index) {
      serial.print("Address: ");
      printI2cAddressValue(serial, result.addresses[index]);
      serial.println();
    }
  }

  if (result.overflow) {
    serial.println("Scanner address list truncated.");
  }

  const Vl6180xI2cAddressComparison comparison =
      compareVl6180xI2cAddress(result);

  serial.print("Expected VL6180X address: ");
  printI2cAddressValue(serial, comparison.expectedAddress);
  serial.println();
  serial.print("VL6180X address comparison: ");
  serial.println(vl6180xI2cAddressStatusToText(comparison.status));
  serial.print("VL6180X found address: ");
  if (comparison.hasFoundAddress) {
    printI2cAddressValue(serial, comparison.foundAddress);
    serial.println();
  } else {
    serial.println("not found");
  }
  serial.println("VL6180X physical detection: Pendente para Hardware Validation");
  serial.println("VL6180X physical address confirmation: Pendente para Hardware Validation");
  serial.println("SDA/SCL physical verification: Pendente para Hardware Validation");

  runVl6180xDiagnostic(serial);
}

}  // namespace reeflow::diagnostics
