#pragma once

#include "alerts/alert_config.h"

namespace reeflow::alerts {

struct AlertCooldownState {
  bool hasLastRaised[kAlertCodeCount];
  uint32_t lastRaisedAtMillis[kAlertCodeCount];
};

void resetAlertCooldownState(AlertCooldownState& state);
bool alertCooldownSuppresses(const AlertCooldownConfig& config,
                             const AlertCooldownState& state, AlertCode code,
                             uint32_t nowMillis);
void markAlertCooldownRaised(AlertCooldownState& state, AlertCode code,
                             uint32_t nowMillis);

}  // namespace reeflow::alerts
