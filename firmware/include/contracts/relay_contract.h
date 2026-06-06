#pragma once

namespace reeflow::contracts {

enum class RelayActivation {
  kActiveHigh,
  kActiveLow,
};

enum class GpioLogicLevel {
  kLow,
  kHigh,
};

struct RelayElectricalContract {
  RelayActivation activation;
  GpioLogicLevel physicalOffLevel;
  GpioLogicLevel physicalOnLevel;
};

constexpr RelayElectricalContract kRelayElectricalContract = {
    RelayActivation::kActiveHigh,
    GpioLogicLevel::kLow,
    GpioLogicLevel::kHigh,
};

}  // namespace reeflow::contracts
