#pragma once

#include <stddef.h>
#include <stdint.h>

#include "contracts/hardware_pins.h"
#include "contracts/relay_contract.h"
#include "drivers/io/bringup_timer.h"
#include "drivers/io/gpio_interface.h"

namespace reeflow::drivers {

struct RelayDiagnosticTarget {
  uint8_t physicalIndex;
  const contracts::HardwarePin* pin;
};

struct RelayDiagnosticResult {
  bool executed;
  RelayDiagnosticTarget target;
  contracts::RelayActivation activation;
  contracts::GpioLogicLevel offLevel;
  contracts::GpioLogicLevel onLevel;
};

size_t relayDiagnosticCount();
bool getRelayDiagnosticTarget(uint8_t physicalIndex, RelayDiagnosticTarget& target);
RelayDiagnosticResult runIndividualRelayDiagnostic(uint8_t physicalIndex,
                                                   unsigned long holdMs,
                                                   GpioPort& gpio,
                                                   BringupTimer& timer);
RelayDiagnosticResult runIndividualRelayDiagnostic(uint8_t physicalIndex,
                                                   unsigned long holdMs);
void applyAllRelaysOff(GpioPort& gpio);
void applyAllRelaysOff();

}  // namespace reeflow::drivers
