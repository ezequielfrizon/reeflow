#include "diagnostics/consolidation/hardware_bringup_consolidation.h"

#include "config/pwm_bringup_config.h"
#include "contracts/i2c_contract.h"
#include "contracts/relay_contract.h"
#include "diagnostics/noise/sensor_noise_validation_diagnostics.h"
#include "diagnostics/power/five_volt_stability_diagnostics.h"
#include "drivers/pwm/pwm_diagnostic_driver.h"
#include "drivers/relays/relay_diagnostic_driver.h"

namespace reeflow::diagnostics {
namespace {

BringupReadinessStatus readinessFromBool(bool ready) {
  return ready ? BringupReadinessStatus::kReady
               : BringupReadinessStatus::kNotImplemented;
}

HardwareBringupConsolidationGroup makeGroup(const char* group, bool ready,
                                            const char* detail) {
  return {
      group,
      readinessFromBool(ready),
      BringupPhysicalValidationStatus::kPendingHardwareValidation,
      detail,
  };
}

}  // namespace

HardwareBringupConsolidationInputs defaultHardwareBringupConsolidationInputs() {
  return {
      true,
      true,
      true,
      true,
      true,
      true,
      true,
      true,
      true,
      true,
  };
}

HardwareBringupConsolidationSummary buildHardwareBringupConsolidationSummary(
    const HardwareBringupConsolidationInputs& inputs) {
  HardwareBringupConsolidationSummary summary = {
      kHardwareBringupConsolidationGroupCount,
      {},
      false,
      false,
  };

  summary.groups[0] = makeGroup(
      "Structure",
      inputs.structureReady,
      "Firmware folders and bring-up app prepared for Phase 1.");
  summary.groups[1] = makeGroup(
      "GPIO and pinout",
      inputs.pinoutContractReady,
      "Hardware V1 pinout contract covers GPIOs, relays, PWM and sensors.");
  summary.groups[2] = makeGroup(
      "Initial safe state",
      inputs.initialSafeStateReady,
      "Relays OFF, PWM duty zero and sensor bus pins prepared.");
  summary.groups[3] = makeGroup(
      "Relay contract",
      inputs.relayContractReady,
      "Relay polarity ACTIVE_HIGH, OFF LOW and ON HIGH.");
  summary.groups[4] = makeGroup(
      "Relay diagnostics",
      inputs.relayDiagnosticReady,
      "Individual relay diagnostics prepared for Relay 1..4.");
  summary.groups[5] = makeGroup(
      "PWM diagnostics",
      inputs.pwmDiagnosticReady,
      "PWM bring-up parameters prepared: 5000 Hz, 8 bits, duty 0/64/128.");
  summary.groups[6] = makeGroup(
      "DS18B20 diagnostic",
      inputs.ds18b20DiagnosticReady,
      "DS18B20 diagnostic prepared with FOUND, NOT_FOUND and READ_ERROR.");
  summary.groups[7] = makeGroup(
      "I2C and VL6180X diagnostics",
      inputs.i2cVl6180xDiagnosticReady,
      "I2C scanner, VL6180X address 0x29 and basic read prepared.");
  summary.groups[8] = makeGroup(
      "5V stability checklist",
      inputs.fiveVoltChecklistReady,
      "5V resting, individual relay and controlled sequence checklist prepared.");
  summary.groups[9] = makeGroup(
      "Sensor noise checklist",
      inputs.sensorNoiseChecklistReady,
      "Before/during/after relay sensor log format prepared.");

  return summary;
}

HardwareBringupConsolidationSummary runHardwareBringupConsolidation(
    drivers::GpioPort& gpio, drivers::PwmLedcPort& pwm,
    const HardwareBringupConsolidationInputs& inputs) {
  HardwareBringupConsolidationSummary summary =
      buildHardwareBringupConsolidationSummary(inputs);

  drivers::applyAllRelaysOff(gpio);
  drivers::applyAllPwmDutyZero(pwm);
  summary.relaysCommandedSafe = true;
  summary.pwmCommandedSafe = true;
  return summary;
}

HardwareBringupConsolidationSummary runHardwareBringupConsolidation(
    drivers::GpioPort& gpio, drivers::PwmLedcPort& pwm) {
  return runHardwareBringupConsolidation(
      gpio, pwm, defaultHardwareBringupConsolidationInputs());
}

const char* bringupReadinessStatusToText(BringupReadinessStatus status) {
  switch (status) {
    case BringupReadinessStatus::kReady:
      return "READY";
    case BringupReadinessStatus::kNotImplemented:
      return "NOT_IMPLEMENTED";
  }

  return "NOT_IMPLEMENTED";
}

const char* bringupPhysicalValidationStatusToText(
    BringupPhysicalValidationStatus status) {
  switch (status) {
    case BringupPhysicalValidationStatus::kPendingHardwareValidation:
      return "PENDING_HARDWARE_VALIDATION";
  }

  return "PENDING_HARDWARE_VALIDATION";
}

}  // namespace reeflow::diagnostics
