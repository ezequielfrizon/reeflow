#pragma once

#include <stddef.h>
#include <stdint.h>

#include "modules/lighting/lighting_pwm_controller.h"

namespace reeflow::test::fakes {

class FakeLightingPwmController final
    : public modules::lighting::LightingPwmController {
 public:
  enum class RecordedCallType {
    kConfigureChannel,
    kWriteDuty,
    kAllOff,
  };

  struct RecordedCall {
    RecordedCallType type;
    modules::lighting::LightingChannel channel;
    uint16_t duty;
    modules::lighting::LightingPwmConfig config;
  };

  static constexpr size_t kChannelCount =
      modules::lighting::kLightingFunctionalChannelCount;
  static constexpr size_t kMaxRecordedCalls = 64;

  modules::lighting::LightingPwmResult configureChannel(
      modules::lighting::LightingChannel channel,
      const modules::lighting::LightingPwmConfig& config) override {
    recordCall({RecordedCallType::kConfigureChannel, channel, 0, config});

    const int index = channelIndex(channel);
    if (index < 0) {
      return modules::lighting::LightingPwmResult::kUnknownChannel;
    }

    if (!modules::lighting::validLightingPwmConfig(config)) {
      return modules::lighting::LightingPwmResult::kInvalidConfiguration;
    }

    if (configureResults_[index] !=
        modules::lighting::LightingPwmResult::kSuccess) {
      return configureResults_[index];
    }

    configured_[index] = true;
    channelConfigs_[index] = config;
    return modules::lighting::LightingPwmResult::kSuccess;
  }

  modules::lighting::LightingPwmResult writeDuty(
      modules::lighting::LightingChannel channel, uint16_t duty) override {
    recordCall(
        {RecordedCallType::kWriteDuty, channel, duty,
         modules::lighting::makeDefaultLightingModuleConfig().pwm});

    const int index = channelIndex(channel);
    if (index < 0) {
      return modules::lighting::LightingPwmResult::kUnknownChannel;
    }

    if (!modules::lighting::validLightingDuty(duty)) {
      return modules::lighting::LightingPwmResult::kInvalidDuty;
    }

    if (writeResults_[index] !=
        modules::lighting::LightingPwmResult::kSuccess) {
      return writeResults_[index];
    }

    duties_[index] = duty;
    return modules::lighting::LightingPwmResult::kSuccess;
  }

  modules::lighting::LightingPwmResult allOff() override {
    ++allOffCallCount_;
    recordCall({RecordedCallType::kAllOff,
                modules::lighting::LightingChannel::kUnknown, 0,
                modules::lighting::makeDefaultLightingModuleConfig().pwm});

    if (allOffResult_ != modules::lighting::LightingPwmResult::kSuccess) {
      return allOffResult_;
    }

    for (size_t index = 0; index < kChannelCount; ++index) {
      if (writeResults_[index] !=
          modules::lighting::LightingPwmResult::kSuccess) {
        return writeResults_[index];
      }
    }

    for (size_t index = 0; index < kChannelCount; ++index) {
      duties_[index] = 0;
    }

    return modules::lighting::LightingPwmResult::kSuccess;
  }

  void failChannel(modules::lighting::LightingChannel channel) {
    setWriteResult(channel,
                   modules::lighting::LightingPwmResult::kControllerFailure);
  }

  void setWriteResult(modules::lighting::LightingChannel channel,
                      modules::lighting::LightingPwmResult result) {
    const int index = channelIndex(channel);
    if (index >= 0) {
      writeResults_[index] = result;
    }
  }

  void setConfigureResult(modules::lighting::LightingChannel channel,
                          modules::lighting::LightingPwmResult result) {
    const int index = channelIndex(channel);
    if (index >= 0) {
      configureResults_[index] = result;
    }
  }

  void setAllOffResult(modules::lighting::LightingPwmResult result) {
    allOffResult_ = result;
  }

  bool configured(modules::lighting::LightingChannel channel) const {
    const int index = channelIndex(channel);
    return index >= 0 && configured_[index];
  }

  uint16_t duty(modules::lighting::LightingChannel channel) const {
    const int index = channelIndex(channel);
    if (index < 0) {
      return 0;
    }

    return duties_[index];
  }

  size_t recordedCallCount() const {
    return recordedCallCount_;
  }

  size_t allOffCallCount() const {
    return allOffCallCount_;
  }

  const RecordedCall& recordedCall(size_t index) const {
    return recordedCalls_[index];
  }

 private:
  static int channelIndex(modules::lighting::LightingChannel channel) {
    switch (channel) {
      case modules::lighting::LightingChannel::kWhite:
        return 0;
      case modules::lighting::LightingChannel::kBlue:
        return 1;
      case modules::lighting::LightingChannel::kRoyalBlue:
        return 2;
      case modules::lighting::LightingChannel::kUv:
        return 3;
      case modules::lighting::LightingChannel::kUnknown:
        return -1;
    }

    return -1;
  }

  void recordCall(RecordedCall call) {
    if (recordedCallCount_ < kMaxRecordedCalls) {
      recordedCalls_[recordedCallCount_++] = call;
    }
  }

  bool configured_[kChannelCount] = {};
  uint16_t duties_[kChannelCount] = {};
  modules::lighting::LightingPwmConfig channelConfigs_[kChannelCount] = {};
  modules::lighting::LightingPwmResult configureResults_[kChannelCount] = {
      modules::lighting::LightingPwmResult::kSuccess,
      modules::lighting::LightingPwmResult::kSuccess,
      modules::lighting::LightingPwmResult::kSuccess,
      modules::lighting::LightingPwmResult::kSuccess,
  };
  modules::lighting::LightingPwmResult writeResults_[kChannelCount] = {
      modules::lighting::LightingPwmResult::kSuccess,
      modules::lighting::LightingPwmResult::kSuccess,
      modules::lighting::LightingPwmResult::kSuccess,
      modules::lighting::LightingPwmResult::kSuccess,
  };
  modules::lighting::LightingPwmResult allOffResult_ =
      modules::lighting::LightingPwmResult::kSuccess;
  RecordedCall recordedCalls_[kMaxRecordedCalls] = {};
  size_t recordedCallCount_ = 0;
  size_t allOffCallCount_ = 0;
};

}  // namespace reeflow::test::fakes
