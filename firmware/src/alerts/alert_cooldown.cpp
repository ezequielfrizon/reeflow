#include "alerts/alert_cooldown.h"

namespace reeflow::alerts {

AlertCooldownConfig makeDefaultAlertCooldownConfig() {
  AlertCooldownConfig config = {};
  config.defaultCooldownMillis = kDefaultAlertCooldownMillis;

  for (size_t index = 0; index < kAlertCodeCount; ++index) {
    config.perAlertMillis[index] = kDefaultAlertCooldownMillis;
  }

  return config;
}

bool validAlertCooldownMillis(uint32_t cooldownMillis) {
  return cooldownMillis >= kMinAlertCooldownMillis &&
         cooldownMillis <= kMaxAlertCooldownMillis;
}

uint32_t normalizeAlertCooldownMillis(uint32_t cooldownMillis) {
  if (cooldownMillis < kMinAlertCooldownMillis) {
    return kMinAlertCooldownMillis;
  }

  if (cooldownMillis > kMaxAlertCooldownMillis) {
    return kMaxAlertCooldownMillis;
  }

  return cooldownMillis;
}

uint32_t cooldownForAlert(const AlertCooldownConfig& config, AlertCode code) {
  const size_t index = alertCodeIndex(code);
  if (index >= kAlertCodeCount) {
    return normalizeAlertCooldownMillis(config.defaultCooldownMillis);
  }

  return normalizeAlertCooldownMillis(config.perAlertMillis[index]);
}

bool setAlertCooldown(AlertCooldownConfig& config, AlertCode code,
                      uint32_t cooldownMillis) {
  const size_t index = alertCodeIndex(code);
  if (index >= kAlertCodeCount || !validAlertCooldownMillis(cooldownMillis)) {
    return false;
  }

  config.perAlertMillis[index] = cooldownMillis;
  return true;
}

void resetAlertCooldownState(AlertCooldownState& state) {
  for (size_t index = 0; index < kAlertCodeCount; ++index) {
    state.hasLastRaised[index] = false;
    state.lastRaisedAtMillis[index] = 0;
  }
}

bool alertCooldownSuppresses(const AlertCooldownConfig& config,
                             const AlertCooldownState& state, AlertCode code,
                             uint32_t nowMillis) {
  const size_t index = alertCodeIndex(code);
  if (index >= kAlertCodeCount) {
    return false;
  }

  if (!state.hasLastRaised[index]) {
    return false;
  }

  const uint32_t lastRaisedAtMillis = state.lastRaisedAtMillis[index];
  return nowMillis - lastRaisedAtMillis < cooldownForAlert(config, code);
}

void markAlertCooldownRaised(AlertCooldownState& state, AlertCode code,
                             uint32_t nowMillis) {
  const size_t index = alertCodeIndex(code);
  if (index < kAlertCodeCount) {
    state.hasLastRaised[index] = true;
    state.lastRaisedAtMillis[index] = nowMillis;
  }
}

}  // namespace reeflow::alerts
