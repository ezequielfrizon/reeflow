#pragma once

#include <stdint.h>

#include "modules/modes/mode_automation_gate.h"
#include "modules/modes/mode_types.h"
#include "modules/relays/relay_service.h"

namespace reeflow::modules::modes {

enum class ModeEffectResult {
  kSuccess,
  kRejected,
  kFailed,
};

struct ModeEffectsRequest {
  OperationalMode mode;
  uint32_t requestedAtMillis;
  bool turnOffRecalque;
  bool turnOffAtoPump;
  bool releaseModeBlocks;
  bool preserveManualRelayControl;
  bool updateAtoAutomationGate;
  bool blockAtoAutomation;
};

class ModeEffects {
 public:
  virtual ~ModeEffects() = default;
  virtual ModeEffectResult applyModeEffects(
      const ModeEffectsRequest& request) = 0;
};

class RelayModeEffects final : public ModeEffects {
 public:
  RelayModeEffects(relays::RelayService& relayService,
                   ModeAutomationGate& automationGate);

  ModeEffectResult applyModeEffects(
      const ModeEffectsRequest& request) override;

 private:
  ModeEffectResult applyRelayOff(relays::RelayId relay,
                                 relays::RelayCommandSource source);
  ModeEffectResult applyAtoGate(const ModeEffectsRequest& request);

  relays::RelayService& relayService_;
  ModeAutomationGate& automationGate_;
};

constexpr ModeEffectsRequest makeNoOpModeEffectsRequest(
    OperationalMode mode, uint32_t requestedAtMillis) {
  return {mode, requestedAtMillis, false, false, false, false, false, false};
}

}  // namespace reeflow::modules::modes
