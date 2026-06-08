#include "drivers/relays/gpio_relay_controller.h"

#include "contracts/hardware_pins.h"
#include "contracts/relay_contract.h"

namespace reeflow::drivers {
namespace {

using modules::relays::RelayCommandResult;
using modules::relays::RelayDesiredState;
using modules::relays::RelayId;

const contracts::HardwarePin* relayHardwarePin(RelayId relay) {
  switch (relay) {
    case RelayId::kRecalque:
      return &contracts::kHardwareV1Pins[3];
    case RelayId::kHeater:
      return &contracts::kHardwareV1Pins[4];
    case RelayId::kAtoPump:
      return &contracts::kHardwareV1Pins[5];
    case RelayId::kReserve:
      return &contracts::kHardwareV1Pins[6];
    case RelayId::kUnknown:
      return nullptr;
  }

  return nullptr;
}

constexpr bool isRelayPin(const contracts::HardwarePin& pin) {
  return pin.role == contracts::HardwarePinRole::kRelayOutput;
}

constexpr bool isValidRelayElectricalContract() {
  return contracts::kRelayElectricalContract.physicalOffLevel !=
         contracts::kRelayElectricalContract.physicalOnLevel;
}

constexpr contracts::GpioLogicLevel physicalLevelFor(
    RelayDesiredState desiredState) {
  return desiredState == RelayDesiredState::kOn
             ? contracts::kRelayElectricalContract.physicalOnLevel
             : contracts::kRelayElectricalContract.physicalOffLevel;
}

constexpr GpioLevel toDriverGpioLevel(contracts::GpioLogicLevel level) {
  return level == contracts::GpioLogicLevel::kHigh ? GpioLevel::kHigh
                                                   : GpioLevel::kLow;
}

RelayCommandResult applyRelayState(GpioPort& gpioPort, RelayId relay,
                                   RelayDesiredState desiredState) {
  const contracts::HardwarePin* pin = relayHardwarePin(relay);
  if (pin == nullptr || !isRelayPin(*pin)) {
    return RelayCommandResult::kUnknownRelay;
  }

  if (!isValidRelayElectricalContract()) {
    return RelayCommandResult::kInvalidContract;
  }

  gpioPort.setMode(pin->gpio, GpioMode::kOutput);
  gpioPort.write(pin->gpio, toDriverGpioLevel(physicalLevelFor(desiredState)));
  return RelayCommandResult::kSuccess;
}

}  // namespace

GpioRelayController::GpioRelayController(GpioPort& gpioPort)
    : gpioPort_(gpioPort) {}

RelayCommandResult GpioRelayController::setRelay(
    RelayId relay, RelayDesiredState desiredState) {
  return applyRelayState(gpioPort_, relay, desiredState);
}

RelayCommandResult GpioRelayController::allOff() {
  constexpr RelayId kRelays[] = {
      RelayId::kRecalque,
      RelayId::kHeater,
      RelayId::kAtoPump,
      RelayId::kReserve,
  };

  for (const RelayId relay : kRelays) {
    const RelayCommandResult result =
        applyRelayState(gpioPort_, relay, RelayDesiredState::kOff);
    if (result != RelayCommandResult::kSuccess) {
      return result;
    }
  }

  return RelayCommandResult::kSuccess;
}

}  // namespace reeflow::drivers
