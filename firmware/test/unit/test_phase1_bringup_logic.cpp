#include <assert.h>
#include <stdint.h>
#include <string.h>

#include <map>
#include <vector>

#include "config/pwm_bringup_config.h"
#include "contracts/hardware_pins.h"
#include "contracts/relay_contract.h"
#include "diagnostics/hardware_bringup_identity.h"
#include "diagnostics/pinout/pinout_diagnostics.h"
#include "drivers/initial_safe_state.h"
#include "drivers/pwm/pwm_diagnostic_driver.h"
#include "drivers/relays/relay_diagnostic_driver.h"

namespace {

using reeflow::drivers::BringupTimer;
using reeflow::drivers::GpioLevel;
using reeflow::drivers::GpioMode;
using reeflow::drivers::GpioPort;
using reeflow::drivers::PwmLedcPort;

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

}  // namespace

int main() {
  testModeIdentification();
  testPinoutRows();
  testRelayElectricalContract();
  testInitialSafeState();
  testRelayDiagnostics();
  testPwmConfiguration();
  testPwmDiagnosticSequence();
  return 0;
}
