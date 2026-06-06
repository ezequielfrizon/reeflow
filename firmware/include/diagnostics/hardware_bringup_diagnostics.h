#pragma once

#include <Arduino.h>

namespace reeflow::diagnostics {

const char* hardwareBringupModeName();
void printHardwareBringupBanner(Stream& output);
void printHardwarePinoutTable(Stream& output);
void printRelayElectricalContract(Stream& output);
void printInitialSafeHardwareState(Stream& output, size_t relaysConfigured,
                                   size_t pwmChannelsConfigured,
                                   size_t sensorBusPinsPrepared);
void printRelayDiagnosticCommands(Stream& output);
void printPwmBringupConfiguration(Stream& output);
void printDs18b20DiagnosticTarget(Stream& output);
void printI2cDiagnosticTarget(Stream& output);
void printVl6180xDiagnosticTarget(Stream& output);
void printFiveVoltStabilityChecklist(Stream& output);
void printSensorNoiseValidationProcedure(Stream& output);
void printHardwareBringupConsolidationSummary(Stream& output);
void runHardwareBringupLoop(Stream& serial);

}  // namespace reeflow::diagnostics
