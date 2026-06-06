#pragma once

#include <stdint.h>

#include "core/logging/logger.h"
#include "core/platform/time_source.h"
#include "core/platform/watchdog_backend.h"

namespace reeflow::core::watchdog {

constexpr uint8_t kWatchdogReasonMaxLength = 32;

class WatchdogService {
 public:
  WatchdogService(platform::WatchdogBackend& backend,
                  platform::TimeSource& timeSource, logging::Logger& logger);

  bool initialize(uint32_t timeoutMillis);
  bool feed();
  void markTriggered(const char* reason);
  bool initialized() const;

 private:
  void updateSystemHealth(bool watchdogTriggered, const char* rebootReason,
                          bool updateRebootMetadata);

  platform::WatchdogBackend& backend_;
  platform::TimeSource& timeSource_;
  logging::Logger& logger_;
  bool initialized_ = false;
  uint32_t timeoutMillis_ = 0;
};

bool feedWatchdogTask(void* context);

}  // namespace reeflow::core::watchdog
