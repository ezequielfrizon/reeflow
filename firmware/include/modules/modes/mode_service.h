#pragma once

#include "config/config_manager.h"
#include "core/events/event_bus.h"
#include "core/platform/time_source.h"
#include "modules/modes/mode_policy.h"
#include "modules/modes/mode_store.h"

namespace reeflow::modules::modes {

enum class ModeServiceResult {
  kSuccess,
  kInvalidCommand,
  kInvalidConfig,
  kEffectsFailed,
  kAutomationGateFailed,
  kStoreFailed,
};

struct ModeServiceTickResult {
  ModeServiceResult result;
  bool modeChanged;
};

class ModeService {
 public:
  ModeService(const config::ConfigManager& configManager,
              const core::platform::TimeSource& timeSource,
              core::events::EventBus& eventBus, ModeEffects& effects,
              ModeStore& store);

  ModeServiceResult requestMode(const ModeCommand& command);
  ModeServiceTickResult evaluateOnce();
  ModeServiceResult restoreModeFromStore();

 private:
  ModeServiceTickResult applyPlan(const ModePolicyPlan& plan);
  ModeServiceResult applyRestoredMode(OperationalMode mode);
  ModeServiceResult saveModeIfNeeded(const ModePolicyPlan& plan);
  void publishModeEvents(const ModePolicyPlan& plan);
  ModePolicyInput makePolicyInput(bool hasCommand,
                                  const ModeCommand& command) const;

  const config::ConfigManager& configManager_;
  const core::platform::TimeSource& timeSource_;
  core::events::EventBus& eventBus_;
  ModeEffects& effects_;
  ModeStore& store_;
};

bool runModeServiceTask(void* context);

}  // namespace reeflow::modules::modes
