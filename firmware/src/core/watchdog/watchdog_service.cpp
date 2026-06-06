#include "core/watchdog/watchdog_service.h"

#include <string.h>

#include "core/state/system_state.h"

namespace reeflow::core::watchdog {
namespace {

constexpr const char* kWatchdogOrigin = "watchdog";
constexpr const char* kUnknownRebootReason = "UNKNOWN";

void copyText(char* target, size_t targetLength, const char* source) {
  const char* safeSource = source == nullptr ? "" : source;
  strncpy(target, safeSource, targetLength - 1);
  target[targetLength - 1] = '\0';
}

}  // namespace

WatchdogService::WatchdogService(platform::WatchdogBackend& backend,
                                 platform::TimeSource& timeSource,
                                 logging::Logger& logger)
    : backend_(backend), timeSource_(timeSource), logger_(logger) {}

bool WatchdogService::initialize(uint32_t timeoutMillis) {
  if (timeoutMillis == 0) {
    logger_.error(kWatchdogOrigin, "invalid timeout");
    return false;
  }

  timeoutMillis_ = timeoutMillis;
  backend_.configure(timeoutMillis_);
  initialized_ = true;
  updateSystemHealth(false, kUnknownRebootReason, true);
  logger_.info(kWatchdogOrigin, "initialized");
  return true;
}

bool WatchdogService::feed() {
  if (!initialized_) {
    logger_.error(kWatchdogOrigin, "feed before initialize");
    return false;
  }

  backend_.feed();
  updateSystemHealth(false, kUnknownRebootReason, false);
  return true;
}

void WatchdogService::markTriggered(const char* reason) {
  updateSystemHealth(true, reason == nullptr ? kUnknownRebootReason : reason,
                     true);
  logger_.error(kWatchdogOrigin, "triggered");
}

bool WatchdogService::initialized() const {
  return initialized_;
}

void WatchdogService::updateSystemHealth(bool watchdogTriggered,
                                         const char* rebootReason,
                                         bool updateRebootMetadata) {
  state::SystemHealthState systemHealth =
      state::currentSystemState().systemHealth;
  systemHealth.uptime = timeSource_.uptimeMillis();
  systemHealth.watchdogTriggered = watchdogTriggered;
  if (updateRebootMetadata) {
    systemHealth.lastReboot = timeSource_.uptimeMillis();
    copyText(systemHealth.rebootReason, state::kRebootReasonMaxLength,
             rebootReason);
  }
  state::updateSystemHealthState(systemHealth);
}

bool feedWatchdogTask(void* context) {
  if (context == nullptr) {
    return false;
  }

  WatchdogService* service = static_cast<WatchdogService*>(context);
  return service->feed();
}

}  // namespace reeflow::core::watchdog
