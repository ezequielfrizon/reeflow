#include "app/hardware_bringup_app.h"

#include <Arduino.h>

#include "diagnostics/hardware_bringup_diagnostics.h"
#include "drivers/initial_safe_state.h"

namespace reeflow::app {
namespace {

constexpr unsigned long kSerialBaudRate = 115200;

}  // namespace

void setupHardwareBringupApp() {
  const drivers::InitialSafeStateResult safeStateResult =
      drivers::applyInitialSafeHardwareState();

  Serial.begin(kSerialBaudRate);
  delay(100);

  diagnostics::printHardwareBringupBanner(Serial);
  diagnostics::printHardwarePinoutTable(Serial);
  diagnostics::printRelayElectricalContract(Serial);
  diagnostics::printInitialSafeHardwareState(Serial,
                                             safeStateResult.relaysConfigured,
                                             safeStateResult.pwmChannelsConfigured,
                                             safeStateResult.sensorBusPinsPrepared);
  diagnostics::printRelayDiagnosticCommands(Serial);
  diagnostics::printPwmBringupConfiguration(Serial);
  diagnostics::printDs18b20DiagnosticTarget(Serial);
  diagnostics::printI2cDiagnosticTarget(Serial);
  diagnostics::printVl6180xDiagnosticTarget(Serial);
  diagnostics::printFiveVoltStabilityChecklist(Serial);
  diagnostics::printSensorNoiseValidationProcedure(Serial);
  diagnostics::printHardwareBringupConsolidationSummary(Serial);
}

void loopHardwareBringupApp() {
  diagnostics::runHardwareBringupLoop(Serial);
}

}  // namespace reeflow::app
