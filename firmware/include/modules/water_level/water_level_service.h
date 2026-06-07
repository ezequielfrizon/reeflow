#pragma once

#include <stdint.h>

#include "config/config_manager.h"
#include "core/events/event_bus.h"
#include "core/platform/time_source.h"
#include "core/state/system_state.h"
#include "modules/water_level/water_level_events.h"
#include "modules/water_level/water_level_sensor.h"

namespace reeflow::modules::water_level {

class WaterLevelService {
 public:
  WaterLevelService(WaterLevelSensor& sensor,
                    const config::ConfigManager& configManager,
                    const core::platform::TimeSource& timeSource,
                    core::events::EventBus& eventBus);

  void resetMonitor();
  bool runOnce();

 private:
  void updateStateForValidReading(uint16_t currentLevel,
                                  core::state::WaterLevelStatus status,
                                  uint32_t nowMillis);
  void updateStateForOfflineStatus(core::state::WaterLevelStatus status);
  void publishWaterLevelEvent(WaterLevelEventType eventType);
  void publishTransitionEvents(core::state::WaterLevelStatus previousStatus,
                               core::state::WaterLevelStatus nextStatus,
                               bool hadPreviousStatus);

  WaterLevelSensor& sensor_;
  const config::ConfigManager& configManager_;
  const core::platform::TimeSource& timeSource_;
  core::events::EventBus& eventBus_;
  uint32_t monitorStartedAtMillis_;
  bool hasLastValidReading_;
  uint32_t lastValidReadingAtMillis_;
  bool hasLastAuthoritativeStatus_;
  core::state::WaterLevelStatus lastAuthoritativeStatus_;
};

bool runWaterLevelServiceTask(void* context);

}  // namespace reeflow::modules::water_level
