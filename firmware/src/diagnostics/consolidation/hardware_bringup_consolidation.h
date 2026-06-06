#pragma once

#include <stddef.h>

#include "drivers/io/gpio_interface.h"
#include "drivers/io/pwm_ledc_interface.h"

class Stream;

namespace reeflow::diagnostics {

enum class BringupReadinessStatus {
  kReady,
  kNotImplemented,
};

enum class BringupPhysicalValidationStatus {
  kPendingHardwareValidation,
};

struct HardwareBringupConsolidationInputs {
  bool structureReady;
  bool pinoutContractReady;
  bool initialSafeStateReady;
  bool relayContractReady;
  bool relayDiagnosticReady;
  bool pwmDiagnosticReady;
  bool ds18b20DiagnosticReady;
  bool i2cVl6180xDiagnosticReady;
  bool fiveVoltChecklistReady;
  bool sensorNoiseChecklistReady;
};

struct HardwareBringupConsolidationGroup {
  const char* group;
  BringupReadinessStatus readiness;
  BringupPhysicalValidationStatus physicalValidation;
  const char* detail;
};

constexpr size_t kHardwareBringupConsolidationGroupCount = 10;

struct HardwareBringupConsolidationSummary {
  size_t groupCount;
  HardwareBringupConsolidationGroup
      groups[kHardwareBringupConsolidationGroupCount];
  bool relaysCommandedSafe;
  bool pwmCommandedSafe;
};

HardwareBringupConsolidationInputs defaultHardwareBringupConsolidationInputs();
HardwareBringupConsolidationSummary buildHardwareBringupConsolidationSummary(
    const HardwareBringupConsolidationInputs& inputs);
HardwareBringupConsolidationSummary runHardwareBringupConsolidation(
    drivers::GpioPort& gpio, drivers::PwmLedcPort& pwm,
    const HardwareBringupConsolidationInputs& inputs);
HardwareBringupConsolidationSummary runHardwareBringupConsolidation(
    drivers::GpioPort& gpio, drivers::PwmLedcPort& pwm);
HardwareBringupConsolidationSummary runHardwareBringupConsolidation();
const char* bringupReadinessStatusToText(BringupReadinessStatus status);
const char* bringupPhysicalValidationStatusToText(
    BringupPhysicalValidationStatus status);
void printHardwareBringupConsolidationSummary(Stream& output);
void runHardwareBringupConsolidation(Stream& serial);

}  // namespace reeflow::diagnostics
