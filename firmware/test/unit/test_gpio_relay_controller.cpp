#include <assert.h>

#include "contracts/hardware_pins.h"
#include "contracts/relay_contract.h"
#include "drivers/relays/gpio_relay_controller.h"
#include "fakes/fake_gpio_port.h"
#include "modules/relays/relay_controller.h"
#include "modules/relays/relay_types.h"

namespace {

using reeflow::drivers::GpioLevel;
using reeflow::drivers::GpioMode;
using reeflow::drivers::GpioRelayController;
using reeflow::modules::relays::RelayCommandResult;
using reeflow::modules::relays::RelayDesiredState;
using reeflow::modules::relays::RelayId;
using reeflow::test::fakes::FakeGpioPort;

constexpr uint8_t relayGpioAt(size_t hardwarePinIndex) {
  return reeflow::contracts::kHardwareV1Pins[hardwarePinIndex].gpio;
}

void assertSingleRelayWrite(RelayId relay, uint8_t expectedGpio,
                            RelayDesiredState desiredState,
                            GpioLevel expectedLevel) {
  FakeGpioPort gpio;
  GpioRelayController controller(gpio);

  const auto result = controller.setRelay(relay, desiredState);

  assert(result == RelayCommandResult::kSuccess);
  assert(gpio.modeOperationCount() == 1);
  assert(gpio.modeOperation(0).gpio == expectedGpio);
  assert(gpio.modeOperation(0).mode == GpioMode::kOutput);
  assert(gpio.writeOperationCount() == 1);
  assert(gpio.writeOperation(0).gpio == expectedGpio);
  assert(gpio.writeOperation(0).level == expectedLevel);
}

void testRelayPinsComeFromOfficialHardwareContract() {
  assert(relayGpioAt(3) == 16);
  assert(relayGpioAt(4) == 17);
  assert(relayGpioAt(5) == 18);
  assert(relayGpioAt(6) == 19);
}

void testRelayElectricalContractIsActiveHigh() {
  assert(reeflow::contracts::kRelayElectricalContract.activation ==
         reeflow::contracts::RelayActivation::kActiveHigh);
  assert(reeflow::contracts::kRelayElectricalContract.physicalOffLevel ==
         reeflow::contracts::GpioLogicLevel::kLow);
  assert(reeflow::contracts::kRelayElectricalContract.physicalOnLevel ==
         reeflow::contracts::GpioLogicLevel::kHigh);
}

void testRecalqueUsesGpio16() {
  assertSingleRelayWrite(RelayId::kRecalque, relayGpioAt(3),
                         RelayDesiredState::kOn, GpioLevel::kHigh);
}

void testHeaterUsesGpio17() {
  assertSingleRelayWrite(RelayId::kHeater, relayGpioAt(4),
                         RelayDesiredState::kOn, GpioLevel::kHigh);
}

void testAtoUsesGpio18() {
  assertSingleRelayWrite(RelayId::kAtoPump, relayGpioAt(5),
                         RelayDesiredState::kOn, GpioLevel::kHigh);
}

void testReserveUsesGpio19() {
  assertSingleRelayWrite(RelayId::kReserve, relayGpioAt(6),
                         RelayDesiredState::kOn, GpioLevel::kHigh);
}

void testOffWritesLowWithActiveHighContract() {
  assertSingleRelayWrite(RelayId::kRecalque, relayGpioAt(3),
                         RelayDesiredState::kOff, GpioLevel::kLow);
}

void testAllOffWritesLowToAllRelaysInDeterministicOrder() {
  FakeGpioPort gpio;
  GpioRelayController controller(gpio);

  const auto result = controller.allOff();

  assert(result == RelayCommandResult::kSuccess);
  assert(gpio.modeOperationCount() == 4);
  assert(gpio.writeOperationCount() == 4);

  for (size_t index = 0; index < 4; ++index) {
    const uint8_t expectedGpio = relayGpioAt(index + 3);
    assert(gpio.modeOperation(index).gpio == expectedGpio);
    assert(gpio.modeOperation(index).mode == GpioMode::kOutput);
    assert(gpio.writeOperation(index).gpio == expectedGpio);
    assert(gpio.writeOperation(index).level == GpioLevel::kLow);
  }
}

void testUnknownRelayIsRejectedWithoutGpioWrites() {
  FakeGpioPort gpio;
  GpioRelayController controller(gpio);

  const auto result =
      controller.setRelay(RelayId::kUnknown, RelayDesiredState::kOn);

  assert(result == RelayCommandResult::kUnknownRelay);
  assert(gpio.modeOperationCount() == 0);
  assert(gpio.writeOperationCount() == 0);
}

}  // namespace

int main() {
  testRelayPinsComeFromOfficialHardwareContract();
  testRelayElectricalContractIsActiveHigh();
  testRecalqueUsesGpio16();
  testHeaterUsesGpio17();
  testAtoUsesGpio18();
  testReserveUsesGpio19();
  testOffWritesLowWithActiveHighContract();
  testAllOffWritesLowToAllRelaysInDeterministicOrder();
  testUnknownRelayIsRejectedWithoutGpioWrites();
  return 0;
}
