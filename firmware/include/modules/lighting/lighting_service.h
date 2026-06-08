#pragma once

#include <stdint.h>

#include "config/config_manager.h"
#include "core/events/event_bus.h"
#include "core/platform/time_source.h"
#include "core/state/system_state.h"
#include "modules/lighting/lighting_config.h"
#include "modules/lighting/lighting_events.h"
#include "modules/lighting/lighting_policy.h"
#include "modules/lighting/lighting_profile.h"
#include "modules/lighting/lighting_pwm_controller.h"
#include "modules/lighting/lighting_types.h"

namespace reeflow::modules::lighting {

enum class LightingServiceResult {
  kSuccess,
  kInvalidCommand,
  kInvalidConfig,
  kInvalidProfile,
  kControllerFailure,
};

struct LightingServiceEvaluationResult {
  LightingServiceResult result;
  LightingPwmResult pwmResult;
  bool stateUpdated;
  bool emittedProfileChanged;
  bool emittedStarted;
  bool emittedStopped;
};

class LightingService {
 public:
  LightingService(LightingPwmController& controller,
                  const config::ConfigManager& configManager,
                  core::events::EventBus& eventBus,
                  const LightingProfile& activeProfile,
                  LightingModuleConfig moduleConfig =
                      makeDefaultLightingModuleConfig());
  LightingService(LightingPwmController& controller,
                  const config::ConfigManager& configManager,
                  const core::platform::TimeSource& timeSource,
                  core::events::EventBus& eventBus,
                  const LightingProfile& activeProfile,
                  LightingModuleConfig moduleConfig =
                      makeDefaultLightingModuleConfig());

  void setActiveProfile(const LightingProfile& profile);
  const LightingProfile& activeProfile() const;

  LightingServiceEvaluationResult initializeSafeState();
  LightingServiceResult requestMode(LightingMode mode,
                                    uint32_t requestedAtMillis);
  LightingServiceResult requestProfile(uint32_t requestedAtMillis);
  LightingServiceResult requestManualDuty(LightingChannel channel,
                                          uint16_t duty,
                                          uint32_t requestedAtMillis);
  void setAcclimationElapsedDays(uint16_t elapsedDays);

  LightingServiceEvaluationResult evaluateOnce();
  LightingServiceEvaluationResult evaluateOnce(
      uint16_t currentMinuteOfDay, const LightingCommand* command = nullptr,
      uint16_t acclimationElapsedDays = 0);

 private:
  uint16_t currentMinuteOfDay() const;
  void queueCommand(const LightingCommand& command);
  bool pendingCommandConverged() const;
  LightingServiceEvaluationResult applyDecision(
      const LightingPolicyDecision& decision, const LightingCommand* command);
  LightingServiceResult serviceResultForDecision(
      const LightingPolicyDecision& decision) const;
  bool applyPwmWrites(const LightingPolicyDecision& decision,
                      LightingPwmResult& pwmResult);
  core::state::LightingState makeNextLightingState(
      const LightingPolicyDecision& decision, const LightingCommand* command)
      const;
  void publishTransitionEvents(const core::state::LightingState& before,
                               const core::state::LightingState& after,
                               bool profileChanged,
                               const LightingCommand* command,
                               LightingServiceEvaluationResult& result);
  bool runtimeProfileChanged(const core::state::LightingState& before,
                             const core::state::LightingState& after,
                             const LightingCommand* command);
  void publishLightingEvent(LightingEventType eventType,
                            const core::state::LightingState& state,
                            LightingChannelMask affectedChannels,
                            const LightingCommand* command);

  LightingPwmController& controller_;
  const config::ConfigManager& configManager_;
  const core::platform::TimeSource* timeSource_;
  core::events::EventBus& eventBus_;
  LightingProfile activeProfile_;
  LightingModuleConfig moduleConfig_;
  uint32_t lastAppliedProfileSignature_;
  bool hasLastAppliedProfileSignature_;
  LightingCommand pendingCommand_;
  bool hasPendingCommand_;
  uint16_t acclimationElapsedDays_;
};

bool runLightingServiceTask(void* context);

}  // namespace reeflow::modules::lighting
