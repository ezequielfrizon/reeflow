#pragma once

#include <stdint.h>

#include "config/config_manager.h"
#include "core/events/event_bus.h"
#include "core/platform/time_source.h"
#include "core/state/system_state.h"
#include "modules/temperature/temperature_events.h"
#include "modules/temperature/temperature_sensor.h"

namespace reeflow::modules::temperature {

class TemperatureService {
 public:
  TemperatureService(TemperatureSensor& sensor,
                     const config::ConfigManager& configManager,
                     const core::platform::TimeSource& timeSource,
                     core::events::EventBus& eventBus);

  void resetMonitor();
  bool runOnce();

 private:
  void updateStateForValidReading(float temperatureCelsius,
                                  core::state::TemperatureStatus status,
                                  uint32_t nowMillis);
  void updateStateForOfflineStatus(core::state::TemperatureStatus status);
  void publishTemperatureEvent(TemperatureEventType eventType);
  void publishTransitionEvents(core::state::TemperatureStatus previousStatus,
                               core::state::TemperatureStatus nextStatus,
                               bool hadPreviousStatus);

  TemperatureSensor& sensor_;
  const config::ConfigManager& configManager_;
  const core::platform::TimeSource& timeSource_;
  core::events::EventBus& eventBus_;
  uint32_t monitorStartedAtMillis_;
  bool hasLastValidReading_;
  uint32_t lastValidReadingAtMillis_;
  bool hasLastAuthoritativeStatus_;
  core::state::TemperatureStatus lastAuthoritativeStatus_;
};

bool runTemperatureServiceTask(void* context);

}  // namespace reeflow::modules::temperature
