#pragma once

#include <Arduino.h>

#include "contracts/relay_contract.h"

namespace reeflow::diagnostics {

const char* relayActivationToText(contracts::RelayActivation activation);
const char* gpioLogicLevelToText(contracts::GpioLogicLevel level);
void printRelayElectricalContract(Stream& output);
void printRelayDiagnosticCommands(Stream& output);
void runRelayBringupDiagnostic(Stream& serial, uint8_t relayIndex,
                               unsigned long holdMs);

}  // namespace reeflow::diagnostics
