#pragma once

#include "config/config_manager.h"
#include "core/events/event_bus.h"
#include "core/platform/time_source.h"
#include "modules/ato/ato_config.h"
#include "modules/ato/ato_policy.h"
#include "modules/modes/mode_automation_gate.h"
#include "modules/relays/relay_service.h"

namespace reeflow::modules::ato {

enum class AtoServiceResult {
  kSuccess,
  kRelayCommandFailed,
  kInvalidContract,
};

class AtoService {
 public:
  AtoService(relays::RelayService& relayService,
             const config::ConfigManager& configManager,
             const core::platform::TimeSource& timeSource,
             core::events::EventBus& eventBus,
             AtoModuleConfig moduleConfig = makeDefaultAtoModuleConfig(),
             modes::ModeAutomationGate* modeAutomationGate = nullptr);

  AtoServiceResult evaluateOnce();

 private:
  AtoServiceResult applyRelayCommand(const AtoEvaluationResult& evaluation);
  AtoServiceResult applyModeGate(const core::state::SystemState& state);
  void publishEventForDecision(AtoDecision decision);

  relays::RelayService& relayService_;
  const config::ConfigManager& configManager_;
  const core::platform::TimeSource& timeSource_;
  core::events::EventBus& eventBus_;
  AtoModuleConfig moduleConfig_;
  modes::ModeAutomationGate* modeAutomationGate_;
};

bool runAtoServiceTask(void* context);

}  // namespace reeflow::modules::ato
