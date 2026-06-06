#include "diagnostics/noise/sensor_noise_validation_diagnostics.h"

#include "contracts/hardware_pins.h"
#include "contracts/i2c_contract.h"
#include "contracts/relay_contract.h"
#include "drivers/pwm/pwm_diagnostic_driver.h"
#include "drivers/relays/relay_diagnostic_driver.h"

namespace reeflow::diagnostics {
namespace {

drivers::GpioLevel relayLevelToGpioLevel(contracts::GpioLogicLevel level) {
  switch (level) {
    case contracts::GpioLogicLevel::kLow:
      return drivers::GpioLevel::kLow;
    case contracts::GpioLogicLevel::kHigh:
      return drivers::GpioLevel::kHigh;
  }

  return drivers::GpioLevel::kLow;
}

void applyRelayLevel(const contracts::HardwarePin& pin,
                     contracts::GpioLogicLevel level,
                     drivers::GpioPort& gpio) {
  const drivers::GpioLevel gpioLevel = relayLevelToGpioLevel(level);
  gpio.write(pin.gpio, gpioLevel);
  gpio.setMode(pin.gpio, drivers::GpioMode::kOutput);
  gpio.write(pin.gpio, gpioLevel);
}

uint8_t ds18b20Gpio() {
  for (size_t index = 0; index < contracts::kHardwareV1PinCount; ++index) {
    const contracts::HardwarePin& pin = contracts::kHardwareV1Pins[index];
    if (pin.role == contracts::HardwarePinRole::kSensorBus &&
        pin.gpio == 4) {
      return pin.gpio;
    }
  }

  return 4;
}

bool ds18b20ReadFailed(drivers::Ds18b20DiagnosticStatus status) {
  return status != drivers::Ds18b20DiagnosticStatus::kFound;
}

bool vl6180xReadFailed(drivers::Vl6180xDiagnosticReadStatus status) {
  return status != drivers::Vl6180xDiagnosticReadStatus::kValid;
}

void appendSnapshot(SensorNoiseValidationResult& result, uint8_t relayIndex,
                    SensorNoiseSnapshotPhase phase, drivers::BringupTimer& timer,
                    drivers::OneWireBus& oneWire,
                    drivers::Vl6180xDiagnosticSensor& vl6180x,
                    unsigned long ds18b20ConversionWaitMs) {
  if (result.snapshotsCollected >= kSensorNoiseMaxSnapshots) {
    return;
  }

  SensorNoiseSnapshot& snapshot = result.snapshots[result.snapshotsCollected];
  snapshot.relayIndex = relayIndex;
  snapshot.phase = phase;
  snapshot.ds18b20 = drivers::runDs18b20DiagnosticRead(
      oneWire, timer, ds18b20Gpio(), ds18b20ConversionWaitMs);
  snapshot.vl6180x = drivers::runVl6180xDiagnosticRead(
      vl6180x, contracts::kVl6180xI2cContract.expectedAddress);
  snapshot.communicationLossObserved =
      ds18b20ReadFailed(snapshot.ds18b20.status) ||
      vl6180xReadFailed(snapshot.vl6180x.status);
  snapshot.resetObserved = false;
  snapshot.grossOscillationObserved = false;

  ++result.snapshotsCollected;
}

}  // namespace

const char* sensorNoiseSnapshotPhaseToText(SensorNoiseSnapshotPhase phase) {
  switch (phase) {
    case SensorNoiseSnapshotPhase::kBeforeRelay:
      return "BEFORE_RELAY";
    case SensorNoiseSnapshotPhase::kDuringRelay:
      return "DURING_RELAY";
    case SensorNoiseSnapshotPhase::kAfterRelay:
      return "AFTER_RELAY";
  }

  return "UNKNOWN";
}

SensorNoiseValidationResult runSensorNoiseValidationSequence(
    drivers::GpioPort& gpio, drivers::PwmLedcPort& pwm,
    drivers::BringupTimer& timer, drivers::OneWireBus& oneWire,
    drivers::Vl6180xDiagnosticSensor& vl6180x, unsigned long relayHoldMs,
    unsigned long ds18b20ConversionWaitMs) {
  SensorNoiseValidationResult result = {
      0,
      0,
      true,
      true,
      true,
      {},
  };

  drivers::applyAllRelaysOff(gpio);
  drivers::applyAllPwmDutyZero(pwm);

  for (uint8_t relayIndex = 1; relayIndex <= kSensorNoiseRelayCount;
       ++relayIndex) {
    drivers::RelayDiagnosticTarget target = {};
    if (!drivers::getRelayDiagnosticTarget(relayIndex, target) ||
        target.pin == nullptr) {
      result.allRelayCyclesExecuted = false;
      continue;
    }

    appendSnapshot(result, relayIndex, SensorNoiseSnapshotPhase::kBeforeRelay,
                   timer, oneWire, vl6180x, ds18b20ConversionWaitMs);
    drivers::applyAllRelaysOff(gpio);
    applyRelayLevel(*target.pin,
                    contracts::kRelayElectricalContract.physicalOnLevel, gpio);
    appendSnapshot(result, relayIndex, SensorNoiseSnapshotPhase::kDuringRelay,
                   timer, oneWire, vl6180x, ds18b20ConversionWaitMs);
    timer.delayMs(relayHoldMs);
    applyRelayLevel(*target.pin,
                    contracts::kRelayElectricalContract.physicalOffLevel, gpio);
    drivers::applyAllRelaysOff(gpio);
    appendSnapshot(result, relayIndex, SensorNoiseSnapshotPhase::kAfterRelay,
                   timer, oneWire, vl6180x, ds18b20ConversionWaitMs);
    ++result.relayCycles;
  }

  drivers::applyAllRelaysOff(gpio);
  drivers::applyAllPwmDutyZero(pwm);
  return result;
}

}  // namespace reeflow::diagnostics
