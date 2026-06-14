#pragma once

#include "alerts/alert_types.h"

namespace reeflow::alerts {

struct AlertCooldownConfig {
  uint32_t defaultCooldownMillis;
  uint32_t perAlertMillis[kAlertCodeCount];
};

AlertCooldownConfig makeDefaultAlertCooldownConfig();
bool validAlertCooldownMillis(uint32_t cooldownMillis);
uint32_t normalizeAlertCooldownMillis(uint32_t cooldownMillis);
uint32_t cooldownForAlert(const AlertCooldownConfig& config, AlertCode code);
bool setAlertCooldown(AlertCooldownConfig& config, AlertCode code,
                      uint32_t cooldownMillis);

}  // namespace reeflow::alerts
