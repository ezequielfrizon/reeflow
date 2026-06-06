#include "drivers/relays/relay_diagnostic_driver.h"

namespace reeflow::drivers {
namespace {

GpioLevel gpioLogicLevelToPortLevel(contracts::GpioLogicLevel level) {
  switch (level) {
    case contracts::GpioLogicLevel::kLow:
      return GpioLevel::kLow;
    case contracts::GpioLogicLevel::kHigh:
      return GpioLevel::kHigh;
  }

  return GpioLevel::kLow;
}

void applyRelayLevel(const contracts::HardwarePin& pin,
                     contracts::GpioLogicLevel level,
                     GpioPort& gpio) {
  const GpioLevel portLevel = gpioLogicLevelToPortLevel(level);
  gpio.write(pin.gpio, portLevel);
  gpio.setMode(pin.gpio, GpioMode::kOutput);
  gpio.write(pin.gpio, portLevel);
}

}  // namespace

size_t relayDiagnosticCount() {
  size_t count = 0;

  for (size_t index = 0; index < contracts::kHardwareV1PinCount; ++index) {
    if (contracts::kHardwareV1Pins[index].role ==
        contracts::HardwarePinRole::kRelayOutput) {
      ++count;
    }
  }

  return count;
}

bool getRelayDiagnosticTarget(uint8_t physicalIndex, RelayDiagnosticTarget& target) {
  uint8_t currentRelay = 0;

  for (size_t index = 0; index < contracts::kHardwareV1PinCount; ++index) {
    const contracts::HardwarePin& pin = contracts::kHardwareV1Pins[index];

    if (pin.role != contracts::HardwarePinRole::kRelayOutput) {
      continue;
    }

    ++currentRelay;
    if (currentRelay == physicalIndex) {
      target = {physicalIndex, &pin};
      return true;
    }
  }

  target = {};
  return false;
}

void applyAllRelaysOff(GpioPort& gpio) {
  for (size_t index = 0; index < contracts::kHardwareV1PinCount; ++index) {
    const contracts::HardwarePin& pin = contracts::kHardwareV1Pins[index];

    if (pin.role == contracts::HardwarePinRole::kRelayOutput) {
      applyRelayLevel(pin, contracts::kRelayElectricalContract.physicalOffLevel,
                      gpio);
    }
  }
}

RelayDiagnosticResult runIndividualRelayDiagnostic(uint8_t physicalIndex,
                                                   unsigned long holdMs,
                                                   GpioPort& gpio,
                                                   BringupTimer& timer) {
  RelayDiagnosticResult result = {
      false,
      {},
      contracts::kRelayElectricalContract.activation,
      contracts::kRelayElectricalContract.physicalOffLevel,
      contracts::kRelayElectricalContract.physicalOnLevel,
  };

  RelayDiagnosticTarget target = {};
  if (!getRelayDiagnosticTarget(physicalIndex, target) || target.pin == nullptr) {
    applyAllRelaysOff(gpio);
    return result;
  }

  result.executed = true;
  result.target = target;

  applyAllRelaysOff(gpio);
  applyRelayLevel(*target.pin, contracts::kRelayElectricalContract.physicalOnLevel,
                  gpio);
  timer.delayMs(holdMs);
  applyRelayLevel(*target.pin, contracts::kRelayElectricalContract.physicalOffLevel,
                  gpio);
  applyAllRelaysOff(gpio);

  return result;
}

}  // namespace reeflow::drivers
