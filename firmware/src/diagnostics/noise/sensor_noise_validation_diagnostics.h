#pragma once

#include <stddef.h>
#include <stdint.h>

#include "drivers/io/bringup_timer.h"
#include "drivers/io/gpio_interface.h"
#include "drivers/io/pwm_ledc_interface.h"
#include "drivers/onewire/onewire_bus.h"
#include "drivers/sensors/ds18b20_diagnostic_driver.h"
#include "drivers/sensors/vl6180x_diagnostic_driver.h"

class Stream;

namespace reeflow::diagnostics {

enum class SensorNoiseSnapshotPhase {
  kBeforeRelay,
  kDuringRelay,
  kAfterRelay,
};

struct SensorNoiseSnapshot {
  uint8_t relayIndex;
  SensorNoiseSnapshotPhase phase;
  drivers::Ds18b20DiagnosticResult ds18b20;
  drivers::Vl6180xDiagnosticReadResult vl6180x;
  bool communicationLossObserved;
  bool resetObserved;
  bool grossOscillationObserved;
};

constexpr size_t kSensorNoiseRelayCount = 4;
constexpr size_t kSensorNoiseSnapshotsPerRelay = 3;
constexpr size_t kSensorNoiseMaxSnapshots =
    kSensorNoiseRelayCount * kSensorNoiseSnapshotsPerRelay;

struct SensorNoiseValidationResult {
  size_t relayCycles;
  size_t snapshotsCollected;
  bool allRelayCyclesExecuted;
  bool relaysReturnedOff;
  bool pwmReturnedToDutyZero;
  SensorNoiseSnapshot snapshots[kSensorNoiseMaxSnapshots];
};

const char* sensorNoiseSnapshotPhaseToText(SensorNoiseSnapshotPhase phase);
SensorNoiseValidationResult runSensorNoiseValidationSequence(
    drivers::GpioPort& gpio, drivers::PwmLedcPort& pwm,
    drivers::BringupTimer& timer, drivers::OneWireBus& oneWire,
    drivers::Vl6180xDiagnosticSensor& vl6180x, unsigned long relayHoldMs,
    unsigned long ds18b20ConversionWaitMs);
SensorNoiseValidationResult runSensorNoiseValidationSequence(
    unsigned long relayHoldMs);
void printSensorNoiseValidationProcedure(Stream& output);
void runSensorNoiseValidationProcedure(Stream& serial, unsigned long relayHoldMs);

}  // namespace reeflow::diagnostics
