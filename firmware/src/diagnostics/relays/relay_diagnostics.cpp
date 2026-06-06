#include "diagnostics/relays/relay_diagnostics.h"

#include "drivers/relays/relay_diagnostic_driver.h"

namespace reeflow::diagnostics {

const char* relayActivationToText(contracts::RelayActivation activation) {
  switch (activation) {
    case contracts::RelayActivation::kActiveHigh:
      return "ACTIVE_HIGH";
    case contracts::RelayActivation::kActiveLow:
      return "ACTIVE_LOW";
  }

  return "UNKNOWN";
}

const char* gpioLogicLevelToText(contracts::GpioLogicLevel level) {
  switch (level) {
    case contracts::GpioLogicLevel::kLow:
      return "LOW";
    case contracts::GpioLogicLevel::kHigh:
      return "HIGH";
  }

  return "UNKNOWN";
}

void printRelayElectricalContract(Stream& output) {
  const contracts::RelayElectricalContract& relayContract =
      contracts::kRelayElectricalContract;

  output.println();
  output.println("Relay Electrical Contract");
  output.print("Activation: ");
  output.println(relayActivationToText(relayContract.activation));
  output.print("Physical OFF GPIO level: ");
  output.println(gpioLogicLevelToText(relayContract.physicalOffLevel));
  output.print("Physical ON GPIO level: ");
  output.println(gpioLogicLevelToText(relayContract.physicalOnLevel));
}

void printRelayDiagnosticCommands(Stream& output) {
  output.println();
  output.println("Hardware Diagnostic Commands");
  output.println("Send 1, 2, 3 or 4 to test one relay at a time.");
  output.println("Send p to run the PWM bring-up sequence.");
}

void runRelayBringupDiagnostic(Stream& serial, uint8_t relayIndex,
                               unsigned long holdMs) {
  drivers::RelayDiagnosticTarget target = {};
  if (!drivers::getRelayDiagnosticTarget(relayIndex, target) ||
      target.pin == nullptr) {
    serial.println("Relay diagnostic target not found.");
    drivers::applyAllRelaysOff();
    return;
  }

  serial.println();
  serial.println("Relay Individual Diagnostic");
  serial.print("Start: ");
  serial.println(target.pin->canonicalName);
  serial.print("GPIO: ");
  serial.println(target.pin->gpio);
  serial.print("Polarity: ");
  serial.println(relayActivationToText(
      contracts::kRelayElectricalContract.activation));
  serial.print("Expected ON level: ");
  serial.println(gpioLogicLevelToText(
      contracts::kRelayElectricalContract.physicalOnLevel));
  serial.print("Expected OFF level: ");
  serial.println(gpioLogicLevelToText(
      contracts::kRelayElectricalContract.physicalOffLevel));
  serial.println("Expected physical result: only this relay turns ON briefly.");

  const drivers::RelayDiagnosticResult result =
      drivers::runIndividualRelayDiagnostic(relayIndex, holdMs);

  serial.print("End: ");
  serial.println(result.executed ? "relay returned to physical OFF"
                                 : "not executed");
  serial.println("All relays commanded to physical OFF.");
}

}  // namespace reeflow::diagnostics
