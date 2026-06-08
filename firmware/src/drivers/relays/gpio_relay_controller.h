#pragma once

#include "drivers/io/gpio_interface.h"
#include "modules/relays/relay_controller.h"

namespace reeflow::drivers {

class GpioRelayController final : public modules::relays::RelayController {
 public:
  explicit GpioRelayController(GpioPort& gpioPort);

  modules::relays::RelayCommandResult setRelay(
      modules::relays::RelayId relay,
      modules::relays::RelayDesiredState desiredState) override;
  modules::relays::RelayCommandResult allOff() override;

 private:
  GpioPort& gpioPort_;
};

}  // namespace reeflow::drivers
