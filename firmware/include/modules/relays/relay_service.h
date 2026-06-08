#pragma once

#include "core/events/event_bus.h"
#include "core/platform/time_source.h"
#include "core/state/system_state.h"
#include "modules/relays/relay_controller.h"
#include "modules/relays/relay_events.h"
#include "modules/relays/relay_types.h"

namespace reeflow::modules::relays {

class RelayService {
 public:
  RelayService(RelayController& controller,
               const core::platform::TimeSource& timeSource,
               core::events::EventBus& eventBus);

  RelayCommandResult applyCommand(const RelayCommand& command);
  RelayCommandResult setLocalRelay(RelayId relay,
                                   RelayDesiredState desiredState);
  RelayCommandResult allOff(RelayCommandSource source =
                                RelayCommandSource::kFailsafe);

 private:
  RelayCommandResult applyIndividualStateChange(
      core::state::RelaysState& relays, RelayId relay,
      RelayDesiredState desiredState, RelayCommandSource source,
      uint32_t nowMillis, bool allowControllerCall);
  void publishRelayEvent(RelayEventType eventType);

  RelayController& controller_;
  const core::platform::TimeSource& timeSource_;
  core::events::EventBus& eventBus_;
};

}  // namespace reeflow::modules::relays
