#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include "contracts/hardware_pins.h"
#include "drivers/io/pwm_ledc_interface.h"
#include "drivers/lighting/ledc_lighting_pwm_controller.h"
#include "modules/lighting/lighting_config.h"
#include "modules/lighting/lighting_pwm_controller.h"
#include "modules/lighting/lighting_types.h"

namespace {

using reeflow::drivers::lighting::LedcLightingPwmController;
using reeflow::modules::lighting::BLUE;
using reeflow::modules::lighting::LightingChannel;
using reeflow::modules::lighting::LightingPwmResult;
using reeflow::modules::lighting::ROYAL_BLUE;
using reeflow::modules::lighting::UV;
using reeflow::modules::lighting::WHITE;
using reeflow::modules::lighting::kLightingMaxDuty;
using reeflow::modules::lighting::makeDefaultLightingModuleConfig;

struct PwmOperation {
  enum class Type {
    kSetup,
    kAttachPin,
    kWrite,
  };

  Type type;
  uint8_t gpio;
  uint8_t ledcChannel;
  uint32_t frequencyHz;
  uint8_t resolutionBits;
  uint16_t duty;
};

class FakePwmLedcPort final : public reeflow::drivers::PwmLedcPort {
 public:
  static constexpr size_t kMaxOperations = 64;

  void setup(uint8_t ledcChannel, uint32_t frequencyHz,
             uint8_t resolutionBits) override {
    record({PwmOperation::Type::kSetup, 0, ledcChannel, frequencyHz,
            resolutionBits, 0});
  }

  void attachPin(uint8_t gpio, uint8_t ledcChannel) override {
    record({PwmOperation::Type::kAttachPin, gpio, ledcChannel, 0, 0, 0});
  }

  void write(uint8_t ledcChannel, uint16_t duty) override {
    record({PwmOperation::Type::kWrite, 0, ledcChannel, 0, 0, duty});
  }

  size_t operationCount() const {
    return operationCount_;
  }

  const PwmOperation& operation(size_t index) const {
    return operations_[index];
  }

  bool observedGpio(uint8_t gpio) const {
    for (size_t index = 0; index < operationCount_; ++index) {
      if (operations_[index].type == PwmOperation::Type::kAttachPin &&
          operations_[index].gpio == gpio) {
        return true;
      }
    }

    return false;
  }

 private:
  void record(PwmOperation operation) {
    if (operationCount_ < kMaxOperations) {
      operations_[operationCount_++] = operation;
    }
  }

  PwmOperation operations_[kMaxOperations] = {};
  size_t operationCount_ = 0;
};

constexpr uint8_t hardwareGpioAt(size_t hardwarePinIndex) {
  return reeflow::contracts::kHardwareV1Pins[hardwarePinIndex].gpio;
}

void assertConfiguredChannel(const FakePwmLedcPort& pwm, size_t operationOffset,
                             uint8_t expectedGpio,
                             uint8_t expectedLedcChannel,
                             uint32_t expectedFrequencyHz,
                             uint8_t expectedResolutionBits) {
  assert(pwm.operation(operationOffset).type == PwmOperation::Type::kSetup);
  assert(pwm.operation(operationOffset).ledcChannel == expectedLedcChannel);
  assert(pwm.operation(operationOffset).frequencyHz == expectedFrequencyHz);
  assert(pwm.operation(operationOffset).resolutionBits ==
         expectedResolutionBits);

  assert(pwm.operation(operationOffset + 1).type ==
         PwmOperation::Type::kAttachPin);
  assert(pwm.operation(operationOffset + 1).gpio == expectedGpio);
  assert(pwm.operation(operationOffset + 1).ledcChannel ==
         expectedLedcChannel);

  assert(pwm.operation(operationOffset + 2).type ==
         PwmOperation::Type::kWrite);
  assert(pwm.operation(operationOffset + 2).ledcChannel ==
         expectedLedcChannel);
  assert(pwm.operation(operationOffset + 2).duty == 0);
}

void testHardwareContractExposesOfficialLightingPins() {
  assert(hardwareGpioAt(7) == 25);
  assert(hardwareGpioAt(8) == 26);
  assert(hardwareGpioAt(9) == 27);
  assert(hardwareGpioAt(10) == 14);
  assert(hardwareGpioAt(11) == 13);
}

void testBeginConfiguresOfficialChannelsAndWritesZero() {
  FakePwmLedcPort pwm;
  LedcLightingPwmController controller(pwm);
  const auto config = makeDefaultLightingModuleConfig().pwm;

  assert(controller.begin(config) == LightingPwmResult::kSuccess);
  assert(pwm.operationCount() == 12);

  assertConfiguredChannel(pwm, 0, hardwareGpioAt(7), 0, config.frequencyHz,
                          config.resolutionBits);
  assertConfiguredChannel(pwm, 3, hardwareGpioAt(8), 1, config.frequencyHz,
                          config.resolutionBits);
  assertConfiguredChannel(pwm, 6, hardwareGpioAt(9), 2, config.frequencyHz,
                          config.resolutionBits);
  assertConfiguredChannel(pwm, 9, hardwareGpioAt(10), 3, config.frequencyHz,
                          config.resolutionBits);
}

void testReserveGpio13IsNotAttachedByFunctionalController() {
  FakePwmLedcPort pwm;
  LedcLightingPwmController controller(pwm);

  assert(controller.begin() == LightingPwmResult::kSuccess);
  assert(!pwm.observedGpio(hardwareGpioAt(11)));
}

void testWriteDutyUsesConfiguredLedcChannel() {
  FakePwmLedcPort pwm;
  LedcLightingPwmController controller(pwm);

  assert(controller.begin() == LightingPwmResult::kSuccess);
  assert(controller.writeDuty(BLUE, 127) == LightingPwmResult::kSuccess);

  const PwmOperation& operation = pwm.operation(pwm.operationCount() - 1);
  assert(operation.type == PwmOperation::Type::kWrite);
  assert(operation.ledcChannel == 1);
  assert(operation.duty == 127);
}

void testWriteDutyAutoConfiguresBeforeFirstWrite() {
  FakePwmLedcPort pwm;
  LedcLightingPwmController controller(pwm);

  assert(controller.writeDuty(ROYAL_BLUE, 64) ==
         LightingPwmResult::kSuccess);
  assert(pwm.operationCount() == 4);
  assertConfiguredChannel(pwm, 0, hardwareGpioAt(9), 2,
                          makeDefaultLightingModuleConfig().pwm.frequencyHz,
                          makeDefaultLightingModuleConfig().pwm.resolutionBits);
  assert(pwm.operation(3).type == PwmOperation::Type::kWrite);
  assert(pwm.operation(3).ledcChannel == 2);
  assert(pwm.operation(3).duty == 64);
}

void testAllOffWritesZeroInDeterministicOrder() {
  FakePwmLedcPort pwm;
  LedcLightingPwmController controller(pwm);

  assert(controller.begin() == LightingPwmResult::kSuccess);
  assert(controller.writeDuty(WHITE, 10) == LightingPwmResult::kSuccess);
  assert(controller.writeDuty(BLUE, 20) == LightingPwmResult::kSuccess);
  assert(controller.writeDuty(ROYAL_BLUE, 30) ==
         LightingPwmResult::kSuccess);
  assert(controller.writeDuty(UV, 40) == LightingPwmResult::kSuccess);

  const size_t offset = pwm.operationCount();
  assert(controller.allOff() == LightingPwmResult::kSuccess);
  assert(pwm.operationCount() == offset + 4);

  for (size_t index = 0; index < 4; ++index) {
    assert(pwm.operation(offset + index).type == PwmOperation::Type::kWrite);
    assert(pwm.operation(offset + index).ledcChannel == index);
    assert(pwm.operation(offset + index).duty == 0);
  }
}

void testInvalidDutyAndUnknownChannelAreRejectedWithoutWrite() {
  FakePwmLedcPort pwm;
  LedcLightingPwmController controller(pwm);

  assert(controller.begin() == LightingPwmResult::kSuccess);
  const size_t operationCount = pwm.operationCount();

  assert(controller.writeDuty(WHITE, kLightingMaxDuty + 1) ==
         LightingPwmResult::kInvalidDuty);
  assert(controller.writeDuty(LightingChannel::kUnknown, 10) ==
         LightingPwmResult::kUnknownChannel);
  assert(pwm.operationCount() == operationCount);
}

void testInvalidPwmConfigurationIsRejected() {
  FakePwmLedcPort pwm;
  LedcLightingPwmController controller(pwm);
  auto config = makeDefaultLightingModuleConfig().pwm;
  config.frequencyHz = 0;

  assert(controller.begin(config) ==
         LightingPwmResult::kInvalidConfiguration);
  assert(controller.configureChannel(WHITE, config) ==
         LightingPwmResult::kInvalidConfiguration);
  assert(pwm.operationCount() == 0);
}

}  // namespace

int main() {
  testHardwareContractExposesOfficialLightingPins();
  testBeginConfiguresOfficialChannelsAndWritesZero();
  testReserveGpio13IsNotAttachedByFunctionalController();
  testWriteDutyUsesConfiguredLedcChannel();
  testWriteDutyAutoConfiguresBeforeFirstWrite();
  testAllOffWritesZeroInDeterministicOrder();
  testInvalidDutyAndUnknownChannelAreRejectedWithoutWrite();
  testInvalidPwmConfigurationIsRejected();
  return 0;
}
