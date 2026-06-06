#include <assert.h>
#include <stdint.h>
#include <string.h>

#include <map>
#include <vector>

#include "config/pwm_bringup_config.h"
#include "contracts/hardware_pins.h"
#include "contracts/i2c_contract.h"
#include "contracts/relay_contract.h"
#include "diagnostics/consolidation/hardware_bringup_consolidation.h"
#include "diagnostics/hardware_bringup_identity.h"
#include "diagnostics/i2c/vl6180x_i2c_address_diagnostics.h"
#include "diagnostics/noise/sensor_noise_validation_diagnostics.h"
#include "diagnostics/pinout/pinout_diagnostics.h"
#include "diagnostics/power/five_volt_stability_diagnostics.h"
#include "drivers/i2c/i2c_diagnostic_bus.h"
#include "drivers/initial_safe_state.h"
#include "drivers/onewire/onewire_bus.h"
#include "drivers/pwm/pwm_diagnostic_driver.h"
#include "drivers/relays/relay_diagnostic_driver.h"
#include "drivers/sensors/ds18b20_diagnostic_driver.h"
#include "drivers/sensors/vl6180x_diagnostic_driver.h"

namespace {

using reeflow::drivers::BringupTimer;
using reeflow::drivers::GpioLevel;
using reeflow::drivers::GpioMode;
using reeflow::drivers::GpioPort;
using reeflow::drivers::I2cDiagnosticBus;
using reeflow::drivers::OneWireBus;
using reeflow::drivers::PwmLedcPort;
using reeflow::drivers::Vl6180xDiagnosticSensor;

struct FakeTimer final : BringupTimer {
  void delayMs(unsigned long milliseconds) override {
    delays.push_back(milliseconds);
  }

  std::vector<unsigned long> delays;
};

struct FakeGpio final : GpioPort {
  struct ModeOperation {
    uint8_t gpio;
    GpioMode mode;
  };

  struct WriteOperation {
    uint8_t gpio;
    GpioLevel level;
  };

  void setMode(uint8_t gpio, GpioMode mode) override {
    modes[gpio] = mode;
    modeOperations.push_back({gpio, mode});
  }

  void write(uint8_t gpio, GpioLevel level) override {
    levels[gpio] = level;
    writeOperations.push_back({gpio, level});
    const size_t activeRelays = countActiveRelays();
    if (activeRelays > maxActiveRelays) {
      maxActiveRelays = activeRelays;
    }
  }

  size_t countActiveRelays() const {
    size_t active = 0;
    for (size_t index = 0; index < reeflow::contracts::kHardwareV1PinCount;
         ++index) {
      const reeflow::contracts::HardwarePin& pin =
          reeflow::contracts::kHardwareV1Pins[index];
      if (pin.role != reeflow::contracts::HardwarePinRole::kRelayOutput) {
        continue;
      }

      const auto found = levels.find(pin.gpio);
      if (found != levels.end() && found->second == GpioLevel::kHigh) {
        ++active;
      }
    }

    return active;
  }

  std::map<uint8_t, GpioMode> modes;
  std::map<uint8_t, GpioLevel> levels;
  std::vector<ModeOperation> modeOperations;
  std::vector<WriteOperation> writeOperations;
  size_t maxActiveRelays = 0;
};

struct FakePwm final : PwmLedcPort {
  struct SetupOperation {
    uint8_t channel;
    uint32_t frequencyHz;
    uint8_t resolutionBits;
  };

  struct AttachOperation {
    uint8_t gpio;
    uint8_t channel;
  };

  struct WriteOperation {
    uint8_t channel;
    uint16_t duty;
  };

  void setup(uint8_t ledcChannel, uint32_t frequencyHz,
             uint8_t resolutionBits) override {
    setups.push_back({ledcChannel, frequencyHz, resolutionBits});
  }

  void attachPin(uint8_t gpio, uint8_t ledcChannel) override {
    attachments.push_back({gpio, ledcChannel});
  }

  void write(uint8_t ledcChannel, uint16_t duty) override {
    duties[ledcChannel] = duty;
    writes.push_back({ledcChannel, duty});
    const size_t activeChannels = countActiveChannels();
    if (activeChannels > maxActiveChannels) {
      maxActiveChannels = activeChannels;
    }
  }

  size_t countActiveChannels() const {
    size_t active = 0;
    for (const auto& entry : duties) {
      if (entry.second > 0) {
        ++active;
      }
    }
    return active;
  }

  std::vector<SetupOperation> setups;
  std::vector<AttachOperation> attachments;
  std::vector<WriteOperation> writes;
  std::map<uint8_t, uint16_t> duties;
  size_t maxActiveChannels = 0;
};

struct FakeOneWireBus final : OneWireBus {
  bool reset(uint8_t gpio) override {
    resetGpios.push_back(gpio);
    if (resetIndex >= resetResults.size()) {
      return false;
    }
    return resetResults[resetIndex++];
  }

  void writeByte(uint8_t value) override {
    writtenBytes.push_back(value);
  }

  uint8_t readByte() override {
    if (readIndex >= readBytes.size()) {
      return 0;
    }
    return readBytes[readIndex++];
  }

  std::vector<bool> resetResults;
  std::vector<uint8_t> resetGpios;
  std::vector<uint8_t> writtenBytes;
  std::vector<uint8_t> readBytes;
  size_t resetIndex = 0;
  size_t readIndex = 0;
};

struct FakeI2cDiagnosticBus final : I2cDiagnosticBus {
  void begin(uint8_t sdaGpio, uint8_t sclGpio) override {
    beginCalled = true;
    beginSdaGpio = sdaGpio;
    beginSclGpio = sclGpio;
  }

  bool probe(uint8_t address) override {
    probedAddresses.push_back(address);
    for (const uint8_t foundAddress : foundAddresses) {
      if (foundAddress == address) {
        return true;
      }
    }
    return false;
  }

  bool beginCalled = false;
  uint8_t beginSdaGpio = 0;
  uint8_t beginSclGpio = 0;
  std::vector<uint8_t> foundAddresses;
  std::vector<uint8_t> probedAddresses;
};

struct FakeVl6180xDiagnosticSensor final : Vl6180xDiagnosticSensor {
  bool readRangeRaw(uint8_t i2cAddress, uint8_t& rawRange) override {
    requestedAddresses.push_back(i2cAddress);
    if (!readSucceeds) {
      return false;
    }

    rawRange = nextRawRange;
    return true;
  }

  bool readSucceeds = true;
  uint8_t nextRawRange = 0;
  std::vector<uint8_t> requestedAddresses;
};

uint8_t dallasCrc(const uint8_t* values, size_t count) {
  uint8_t crc = 0;

  for (size_t index = 0; index < count; ++index) {
    uint8_t value = values[index];
    for (uint8_t bit = 0; bit < 8; ++bit) {
      const uint8_t mix = (crc ^ value) & 0x01U;
      crc >>= 1;
      if (mix != 0) {
        crc ^= 0x8CU;
      }
      value >>= 1;
    }
  }

  return crc;
}

void appendDs18b20Scratchpad(FakeOneWireBus& bus, float temperatureC) {
  const int16_t raw = static_cast<int16_t>(temperatureC * 16.0F);
  uint8_t scratchpad[9] = {
      static_cast<uint8_t>(raw & 0xFF),
      static_cast<uint8_t>((raw >> 8) & 0xFF),
      0,
      0,
      0,
      0,
      0,
      0,
      0,
  };
  scratchpad[8] = dallasCrc(scratchpad, 8);
  bus.readBytes.insert(bus.readBytes.end(), scratchpad, scratchpad + 9);
}

void testModeIdentification() {
  assert(strcmp(reeflow::diagnostics::hardwareBringupModeName(),
                "Hardware Bring-Up") == 0);
}

void testPinoutRows() {
  assert(reeflow::diagnostics::pinoutDiagnosticRowCount() ==
         reeflow::contracts::kHardwareV1PinCount);

  reeflow::diagnostics::PinoutDiagnosticRow first = {};
  assert(reeflow::diagnostics::getPinoutDiagnosticRow(0, first));
  assert(first.pin != nullptr);
  assert(first.pin->gpio == 4);

  reeflow::diagnostics::PinoutDiagnosticRow invalid = {};
  assert(!reeflow::diagnostics::getPinoutDiagnosticRow(
      reeflow::contracts::kHardwareV1PinCount, invalid));
  assert(invalid.pin == nullptr);
}

void testRelayElectricalContract() {
  assert(reeflow::contracts::kRelayElectricalContract.activation ==
         reeflow::contracts::RelayActivation::kActiveHigh);
  assert(reeflow::contracts::kRelayElectricalContract.physicalOffLevel ==
         reeflow::contracts::GpioLogicLevel::kLow);
  assert(reeflow::contracts::kRelayElectricalContract.physicalOnLevel ==
         reeflow::contracts::GpioLogicLevel::kHigh);
}

void testInitialSafeState() {
  FakeGpio gpio;
  FakePwm pwm;
  const reeflow::drivers::InitialSafeStateResult result =
      reeflow::drivers::applyInitialSafeHardwareState(gpio, pwm);

  assert(result.relaysConfigured == 4);
  assert(result.pwmChannelsConfigured == 5);
  assert(result.sensorBusPinsPrepared == 3);
  assert(pwm.setups.size() == 5);
  assert(pwm.attachments.size() == 5);
  assert(pwm.writes.size() == 5);

  for (size_t index = 0; index < reeflow::contracts::kHardwareV1PinCount;
       ++index) {
    const reeflow::contracts::HardwarePin& pin =
        reeflow::contracts::kHardwareV1Pins[index];
    if (pin.role == reeflow::contracts::HardwarePinRole::kRelayOutput) {
      assert(gpio.modes[pin.gpio] == GpioMode::kOutput);
      assert(gpio.levels[pin.gpio] == GpioLevel::kLow);
    }
  }

  for (const auto& duty : pwm.duties) {
    assert(duty.second == reeflow::config::pwmBringupParameters().dutyZero);
  }
}

void testRelayDiagnostics() {
  for (uint8_t relayIndex = 1; relayIndex <= 4; ++relayIndex) {
    FakeGpio gpio;
    FakeTimer timer;
    const reeflow::drivers::RelayDiagnosticResult result =
        reeflow::drivers::runIndividualRelayDiagnostic(relayIndex, 10, gpio,
                                                       timer);

    assert(result.executed);
    assert(result.target.physicalIndex == relayIndex);
    assert(timer.delays.size() == 1);
    assert(timer.delays[0] == 10);
    assert(gpio.maxActiveRelays == 1);

    for (size_t index = 0; index < reeflow::contracts::kHardwareV1PinCount;
         ++index) {
      const reeflow::contracts::HardwarePin& pin =
          reeflow::contracts::kHardwareV1Pins[index];
      if (pin.role == reeflow::contracts::HardwarePinRole::kRelayOutput) {
        assert(gpio.levels[pin.gpio] == GpioLevel::kLow);
      }
    }
  }
}

void testPwmConfiguration() {
  const reeflow::config::PwmBringupParameters& parameters =
      reeflow::config::pwmBringupParameters();
  assert(parameters.frequencyHz == 5000);
  assert(parameters.resolutionBits == 8);
  assert(parameters.dutyZero == 0);
  assert(parameters.dutyTestMin == 64);
  assert(parameters.dutyTestMax == 128);
  assert(reeflow::config::pwmBringupChannelCount() == 5);
}

void testPwmDiagnosticSequence() {
  FakePwm pwm;
  FakeTimer timer;
  const reeflow::drivers::PwmDiagnosticResult result =
      reeflow::drivers::runPwmBringupDiagnosticSequence(pwm, timer, 20);

  assert(result.channelsTested == 5);
  assert(pwm.setups.size() == 5);
  assert(pwm.attachments.size() == 5);
  assert(timer.delays.size() == 10);
  assert(pwm.maxActiveChannels == 1);

  for (const auto& setup : pwm.setups) {
    assert(setup.frequencyHz == 5000);
    assert(setup.resolutionBits == 8);
  }

  for (const auto& duty : pwm.duties) {
    assert(duty.second == reeflow::config::pwmBringupParameters().dutyZero);
  }
}

void testDs18b20Found() {
  uint8_t scratchpad[9] = {0xA8, 0x01, 0, 0, 0, 0, 0, 0, 0};
  scratchpad[8] = dallasCrc(scratchpad, 8);

  FakeOneWireBus bus;
  bus.resetResults = {true, true};
  bus.readBytes.assign(scratchpad, scratchpad + 9);
  FakeTimer timer;

  const reeflow::drivers::Ds18b20DiagnosticResult result =
      reeflow::drivers::runDs18b20DiagnosticRead(bus, timer, 4, 10);

  assert(result.status == reeflow::drivers::Ds18b20DiagnosticStatus::kFound);
  assert(result.gpio == 4);
  assert(result.temperatureC == 26.5F);
  assert(timer.delays.size() == 1);
  assert(timer.delays[0] == 10);
  assert(bus.writtenBytes.size() == 4);
  assert(bus.writtenBytes[0] == 0xCC);
  assert(bus.writtenBytes[1] == 0x44);
  assert(bus.writtenBytes[2] == 0xCC);
  assert(bus.writtenBytes[3] == 0xBE);
}

void testDs18b20NotFound() {
  FakeOneWireBus bus;
  bus.resetResults = {false};
  FakeTimer timer;

  const reeflow::drivers::Ds18b20DiagnosticResult result =
      reeflow::drivers::runDs18b20DiagnosticRead(bus, timer, 4, 10);

  assert(result.status == reeflow::drivers::Ds18b20DiagnosticStatus::kNotFound);
  assert(result.gpio == 4);
  assert(timer.delays.empty());
  assert(bus.writtenBytes.empty());
}

void testDs18b20ReadError() {
  FakeOneWireBus bus;
  bus.resetResults = {true, false};
  FakeTimer timer;

  const reeflow::drivers::Ds18b20DiagnosticResult result =
      reeflow::drivers::runDs18b20DiagnosticRead(bus, timer, 4, 10);

  assert(result.status == reeflow::drivers::Ds18b20DiagnosticStatus::kReadError);
  assert(result.gpio == 4);
  assert(timer.delays.size() == 1);
}

void testI2cScanWithDeviceFound() {
  FakeI2cDiagnosticBus bus;
  bus.foundAddresses = {0x12};

  const reeflow::drivers::I2cScanResult result =
      reeflow::drivers::runI2cDiagnosticScan(bus, 21, 22);

  assert(bus.beginCalled);
  assert(bus.beginSdaGpio == 21);
  assert(bus.beginSclGpio == 22);
  assert(result.sdaGpio == 21);
  assert(result.sclGpio == 22);
  assert(result.deviceCount == 1);
  assert(result.addresses[0] == 0x12);
  assert(!result.overflow);
  assert(bus.probedAddresses.front() == reeflow::drivers::kI2cScanFirstAddress);
  assert(bus.probedAddresses.back() == reeflow::drivers::kI2cScanLastAddress);
}

void testI2cScanWithNoDevices() {
  FakeI2cDiagnosticBus bus;

  const reeflow::drivers::I2cScanResult result =
      reeflow::drivers::runI2cDiagnosticScan(bus, 21, 22);

  assert(bus.beginCalled);
  assert(result.deviceCount == 0);
  assert(!result.overflow);
}

void testI2cPinsMatchHardwareContract() {
  bool foundSda = false;
  bool foundScl = false;

  for (size_t index = 0; index < reeflow::contracts::kHardwareV1PinCount;
       ++index) {
    const reeflow::contracts::HardwarePin& pin =
        reeflow::contracts::kHardwareV1Pins[index];
    if (pin.role != reeflow::contracts::HardwarePinRole::kI2cBus) {
      continue;
    }

    if (pin.gpio == 21 && strcmp(pin.signal, "I2C SDA") == 0) {
      foundSda = true;
    }
    if (pin.gpio == 22 && strcmp(pin.signal, "I2C SCL") == 0) {
      foundScl = true;
    }
  }

  assert(foundSda);
  assert(foundScl);
}

void testVl6180xExpectedI2cAddressContract() {
  assert(strcmp(reeflow::contracts::kVl6180xI2cContract.canonicalName,
                "VL6180X") == 0);
  assert(reeflow::contracts::kVl6180xI2cContract.expectedAddress == 0x29);
}

void testVl6180xI2cAddressMatch() {
  FakeI2cDiagnosticBus bus;
  bus.foundAddresses = {0x29};

  const reeflow::drivers::I2cScanResult scan =
      reeflow::drivers::runI2cDiagnosticScan(bus, 21, 22);
  const reeflow::diagnostics::Vl6180xI2cAddressComparison comparison =
      reeflow::diagnostics::compareVl6180xI2cAddress(scan);

  assert(comparison.status ==
         reeflow::diagnostics::Vl6180xI2cAddressStatus::kMatch);
  assert(comparison.expectedAddress == 0x29);
  assert(comparison.hasFoundAddress);
  assert(comparison.foundAddress == 0x29);
  assert(strcmp(reeflow::diagnostics::vl6180xI2cAddressStatusToText(
                    comparison.status),
                "MATCH") == 0);
}

void testVl6180xI2cAddressNotFound() {
  FakeI2cDiagnosticBus bus;

  const reeflow::drivers::I2cScanResult scan =
      reeflow::drivers::runI2cDiagnosticScan(bus, 21, 22);
  const reeflow::diagnostics::Vl6180xI2cAddressComparison comparison =
      reeflow::diagnostics::compareVl6180xI2cAddress(scan);

  assert(comparison.status ==
         reeflow::diagnostics::Vl6180xI2cAddressStatus::kNotFound);
  assert(comparison.expectedAddress == 0x29);
  assert(!comparison.hasFoundAddress);
  assert(strcmp(reeflow::diagnostics::vl6180xI2cAddressStatusToText(
                    comparison.status),
                "NOT_FOUND") == 0);
}

void testVl6180xI2cAddressMismatch() {
  FakeI2cDiagnosticBus bus;
  bus.foundAddresses = {0x12};

  const reeflow::drivers::I2cScanResult scan =
      reeflow::drivers::runI2cDiagnosticScan(bus, 21, 22);
  const reeflow::diagnostics::Vl6180xI2cAddressComparison comparison =
      reeflow::diagnostics::compareVl6180xI2cAddress(scan);

  assert(comparison.status ==
         reeflow::diagnostics::Vl6180xI2cAddressStatus::kMismatch);
  assert(comparison.expectedAddress == 0x29);
  assert(comparison.hasFoundAddress);
  assert(comparison.foundAddress == 0x12);
  assert(strcmp(reeflow::diagnostics::vl6180xI2cAddressStatusToText(
                    comparison.status),
                "MISMATCH") == 0);
}

void testVl6180xBasicReadValid() {
  FakeVl6180xDiagnosticSensor sensor;
  sensor.nextRawRange = 42;

  const reeflow::drivers::Vl6180xDiagnosticReadResult result =
      reeflow::drivers::runVl6180xDiagnosticRead(sensor, 0x29);

  assert(result.status ==
         reeflow::drivers::Vl6180xDiagnosticReadStatus::kValid);
  assert(result.i2cAddress == 0x29);
  assert(result.rawRange == 42);
  assert(result.rangeMm == 42);
  assert(sensor.requestedAddresses.size() == 1);
  assert(sensor.requestedAddresses[0] == 0x29);
}

void testVl6180xBasicReadError() {
  FakeVl6180xDiagnosticSensor sensor;
  sensor.readSucceeds = false;

  const reeflow::drivers::Vl6180xDiagnosticReadResult result =
      reeflow::drivers::runVl6180xDiagnosticRead(sensor, 0x29);

  assert(result.status ==
         reeflow::drivers::Vl6180xDiagnosticReadStatus::kReadError);
  assert(result.i2cAddress == 0x29);
  assert(result.rawRange == 0);
  assert(result.rangeMm == 0);
}

void testFiveVoltStabilityChecklist() {
  assert(reeflow::diagnostics::fiveVoltStabilityChecklistItemCount() == 6);

  reeflow::diagnostics::FiveVoltStabilityChecklistItem resting = {};
  assert(reeflow::diagnostics::getFiveVoltStabilityChecklistItem(0, resting));
  assert(strcmp(resting.reportField, "5V resting voltage") == 0);

  reeflow::diagnostics::FiveVoltStabilityChecklistItem sequence = {};
  assert(reeflow::diagnostics::getFiveVoltStabilityChecklistItem(5, sequence));
  assert(strcmp(sequence.reportField, "5V during controlled sequence") == 0);

  reeflow::diagnostics::FiveVoltStabilityChecklistItem invalid = {};
  assert(!reeflow::diagnostics::getFiveVoltStabilityChecklistItem(6, invalid));
  assert(invalid.checkpoint == nullptr);
}

void testFiveVoltStabilitySequenceReturnsRelaysOff() {
  FakeGpio gpio;
  FakeTimer timer;

  const reeflow::diagnostics::FiveVoltRelayStabilitySequenceResult result =
      reeflow::diagnostics::runFiveVoltRelayStabilitySequence(gpio, timer, 10);

  assert(result.relaysTested == 4);
  assert(result.allRelayDiagnosticsExecuted);
  assert(timer.delays.size() == 4);
  assert(gpio.maxActiveRelays == 1);

  for (size_t index = 0; index < reeflow::contracts::kHardwareV1PinCount;
       ++index) {
    const reeflow::contracts::HardwarePin& pin =
        reeflow::contracts::kHardwareV1Pins[index];
    if (pin.role == reeflow::contracts::HardwarePinRole::kRelayOutput) {
      assert(gpio.levels[pin.gpio] == GpioLevel::kLow);
    }
  }
}

void testSensorNoiseValidationNormalReadSequence() {
  FakeGpio gpio;
  FakePwm pwm;
  FakeTimer timer;
  FakeOneWireBus oneWire;
  FakeVl6180xDiagnosticSensor vl6180x;
  vl6180x.nextRawRange = 40;

  for (size_t index = 0;
       index < reeflow::diagnostics::kSensorNoiseMaxSnapshots; ++index) {
    oneWire.resetResults.push_back(true);
    oneWire.resetResults.push_back(true);
    appendDs18b20Scratchpad(oneWire, 26.5F);
  }

  const reeflow::diagnostics::SensorNoiseValidationResult result =
      reeflow::diagnostics::runSensorNoiseValidationSequence(
          gpio, pwm, timer, oneWire, vl6180x, 10, 0);

  assert(result.relayCycles == 4);
  assert(result.snapshotsCollected ==
         reeflow::diagnostics::kSensorNoiseMaxSnapshots);
  assert(result.allRelayCyclesExecuted);
  assert(result.relaysReturnedOff);
  assert(result.pwmReturnedToDutyZero);
  assert(gpio.maxActiveRelays == 1);
  assert(vl6180x.requestedAddresses.size() ==
         reeflow::diagnostics::kSensorNoiseMaxSnapshots);

  for (size_t index = 0; index < result.snapshotsCollected; ++index) {
    assert(result.snapshots[index].ds18b20.status ==
           reeflow::drivers::Ds18b20DiagnosticStatus::kFound);
    assert(result.snapshots[index].vl6180x.status ==
           reeflow::drivers::Vl6180xDiagnosticReadStatus::kValid);
    assert(!result.snapshots[index].communicationLossObserved);
    assert(!result.snapshots[index].resetObserved);
    assert(!result.snapshots[index].grossOscillationObserved);
  }

  assert(result.snapshots[0].phase ==
         reeflow::diagnostics::SensorNoiseSnapshotPhase::kBeforeRelay);
  assert(result.snapshots[1].phase ==
         reeflow::diagnostics::SensorNoiseSnapshotPhase::kDuringRelay);
  assert(result.snapshots[2].phase ==
         reeflow::diagnostics::SensorNoiseSnapshotPhase::kAfterRelay);

  for (size_t index = 0; index < reeflow::contracts::kHardwareV1PinCount;
       ++index) {
    const reeflow::contracts::HardwarePin& pin =
        reeflow::contracts::kHardwareV1Pins[index];
    if (pin.role == reeflow::contracts::HardwarePinRole::kRelayOutput) {
      assert(gpio.levels[pin.gpio] == GpioLevel::kLow);
    }
  }

  for (const auto& duty : pwm.duties) {
    assert(duty.second == reeflow::config::pwmBringupParameters().dutyZero);
  }
}

void testSensorNoiseValidationFailureSequence() {
  FakeGpio gpio;
  FakePwm pwm;
  FakeTimer timer;
  FakeOneWireBus oneWire;
  FakeVl6180xDiagnosticSensor vl6180x;
  vl6180x.readSucceeds = false;

  for (size_t index = 0;
       index < reeflow::diagnostics::kSensorNoiseMaxSnapshots; ++index) {
    oneWire.resetResults.push_back(false);
  }

  const reeflow::diagnostics::SensorNoiseValidationResult result =
      reeflow::diagnostics::runSensorNoiseValidationSequence(
          gpio, pwm, timer, oneWire, vl6180x, 10, 0);

  assert(result.relayCycles == 4);
  assert(result.snapshotsCollected ==
         reeflow::diagnostics::kSensorNoiseMaxSnapshots);
  assert(result.allRelayCyclesExecuted);

  for (size_t index = 0; index < result.snapshotsCollected; ++index) {
    assert(result.snapshots[index].ds18b20.status ==
           reeflow::drivers::Ds18b20DiagnosticStatus::kNotFound);
    assert(result.snapshots[index].vl6180x.status ==
           reeflow::drivers::Vl6180xDiagnosticReadStatus::kReadError);
    assert(result.snapshots[index].communicationLossObserved);
  }

  for (const auto& duty : pwm.duties) {
    assert(duty.second == reeflow::config::pwmBringupParameters().dutyZero);
  }
}

void testHardwareBringupConsolidationReadySummary() {
  FakeGpio gpio;
  FakePwm pwm;

  const reeflow::diagnostics::HardwareBringupConsolidationSummary summary =
      reeflow::diagnostics::runHardwareBringupConsolidation(gpio, pwm);

  assert(summary.groupCount ==
         reeflow::diagnostics::kHardwareBringupConsolidationGroupCount);
  assert(summary.relaysCommandedSafe);
  assert(summary.pwmCommandedSafe);

  for (size_t index = 0; index < summary.groupCount; ++index) {
    assert(summary.groups[index].readiness ==
           reeflow::diagnostics::BringupReadinessStatus::kReady);
    assert(summary.groups[index].physicalValidation ==
           reeflow::diagnostics::BringupPhysicalValidationStatus::
               kPendingHardwareValidation);
    assert(strcmp(reeflow::diagnostics::bringupReadinessStatusToText(
                      summary.groups[index].readiness),
                  "READY") == 0);
    assert(strcmp(reeflow::diagnostics::bringupPhysicalValidationStatusToText(
                      summary.groups[index].physicalValidation),
                  "PENDING_HARDWARE_VALIDATION") == 0);
  }

  for (size_t index = 0; index < reeflow::contracts::kHardwareV1PinCount;
       ++index) {
    const reeflow::contracts::HardwarePin& pin =
        reeflow::contracts::kHardwareV1Pins[index];
    if (pin.role == reeflow::contracts::HardwarePinRole::kRelayOutput) {
      assert(gpio.levels[pin.gpio] == GpioLevel::kLow);
    }
  }

  for (const auto& duty : pwm.duties) {
    assert(duty.second == reeflow::config::pwmBringupParameters().dutyZero);
  }
}

void testHardwareBringupConsolidationSimulatedFailureStillReportsGroups() {
  reeflow::diagnostics::HardwareBringupConsolidationInputs inputs =
      reeflow::diagnostics::defaultHardwareBringupConsolidationInputs();
  inputs.ds18b20DiagnosticReady = false;

  const reeflow::diagnostics::HardwareBringupConsolidationSummary summary =
      reeflow::diagnostics::buildHardwareBringupConsolidationSummary(inputs);

  assert(summary.groupCount ==
         reeflow::diagnostics::kHardwareBringupConsolidationGroupCount);

  size_t readyGroups = 0;
  size_t notImplementedGroups = 0;
  for (size_t index = 0; index < summary.groupCount; ++index) {
    if (summary.groups[index].readiness ==
        reeflow::diagnostics::BringupReadinessStatus::kReady) {
      ++readyGroups;
    }
    if (summary.groups[index].readiness ==
        reeflow::diagnostics::BringupReadinessStatus::kNotImplemented) {
      ++notImplementedGroups;
    }
    assert(summary.groups[index].physicalValidation ==
           reeflow::diagnostics::BringupPhysicalValidationStatus::
               kPendingHardwareValidation);
  }

  assert(readyGroups == summary.groupCount - 1);
  assert(notImplementedGroups == 1);
}

}  // namespace

int main() {
  testModeIdentification();
  testPinoutRows();
  testRelayElectricalContract();
  testInitialSafeState();
  testRelayDiagnostics();
  testPwmConfiguration();
  testPwmDiagnosticSequence();
  testDs18b20Found();
  testDs18b20NotFound();
  testDs18b20ReadError();
  testI2cScanWithDeviceFound();
  testI2cScanWithNoDevices();
  testI2cPinsMatchHardwareContract();
  testVl6180xExpectedI2cAddressContract();
  testVl6180xI2cAddressMatch();
  testVl6180xI2cAddressNotFound();
  testVl6180xI2cAddressMismatch();
  testVl6180xBasicReadValid();
  testVl6180xBasicReadError();
  testFiveVoltStabilityChecklist();
  testFiveVoltStabilitySequenceReturnsRelaysOff();
  testSensorNoiseValidationNormalReadSequence();
  testSensorNoiseValidationFailureSequence();
  testHardwareBringupConsolidationReadySummary();
  testHardwareBringupConsolidationSimulatedFailureStillReportsGroups();
  return 0;
}
