#include "diagnostics/sensors/ds18b20_diagnostics.h"

#include "contracts/hardware_pins.h"
#include "drivers/sensors/ds18b20_diagnostic_driver.h"

namespace reeflow::diagnostics {
namespace {

const contracts::HardwarePin* ds18b20Pin() {
  for (size_t index = 0; index < contracts::kHardwareV1PinCount; ++index) {
    const contracts::HardwarePin& pin = contracts::kHardwareV1Pins[index];
    if (pin.role == contracts::HardwarePinRole::kSensorBus &&
        pin.gpio == 4) {
      return &pin;
    }
  }

  return nullptr;
}

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

}  // namespace

void printDs18b20DiagnosticTarget(Stream& output) {
  const contracts::HardwarePin* pin = ds18b20Pin();

  output.println();
  output.println("DS18B20 Diagnostic Target");
  if (pin == nullptr) {
    output.println("Status: READ_ERROR");
    output.println("Reason: DS18B20 GPIO not found in hardware contract.");
    return;
  }

  output.print("Canonical Name: ");
  output.println(pin->canonicalName);
  output.print("Signal: ");
  output.println(pin->signal);
  output.print("GPIO: ");
  output.println(pin->gpio);
  output.println("Physical detection/read: Pendente para Hardware Validation");
}

void runDs18b20Diagnostic(Stream& serial) {
  const contracts::HardwarePin* pin = ds18b20Pin();

  serial.println();
  serial.println("DS18B20 Bring-Up Diagnostic");
  if (pin == nullptr) {
    serial.println("Status: READ_ERROR");
    serial.println("DS18B20 GPIO not found in hardware contract.");
    return;
  }

  const drivers::Ds18b20DiagnosticResult result =
      drivers::runDs18b20DiagnosticRead(pin->gpio);

  serial.print("GPIO: ");
  serial.println(result.gpio);
  serial.print("Status: ");
  serial.println(ds18b20StatusToText(result.status));

  if (result.status == drivers::Ds18b20DiagnosticStatus::kFound) {
    serial.print("Temperature C: ");
    serial.println(result.temperatureC);
  } else {
    serial.println("Physical detection/read: Pendente para Hardware Validation");
  }
}

}  // namespace reeflow::diagnostics
