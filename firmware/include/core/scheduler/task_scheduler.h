#pragma once

#include <stddef.h>
#include <stdint.h>

#include "core/events/event_bus.h"
#include "core/logging/logger.h"
#include "core/platform/time_source.h"

namespace reeflow::core::scheduler {

constexpr size_t kMaxScheduledTasks = 12;
constexpr size_t kTaskNameMaxLength = 24;
constexpr uint8_t kInvalidTaskId = 0;

using TaskCallback = bool (*)(void* context);

struct SchedulerRunResult {
  uint8_t executedCount;
  uint8_t failedCount;
};

class TaskScheduler {
 public:
  TaskScheduler(platform::TimeSource& timeSource, events::EventBus& eventBus);
  TaskScheduler(platform::TimeSource& timeSource, events::EventBus& eventBus,
                logging::Logger& logger);

  uint8_t registerTask(const char* name, uint32_t intervalMillis,
                       TaskCallback callback, void* context);
  bool setTaskEnabled(uint8_t taskId, bool enabled);
  SchedulerRunResult runDueTasks();
  void clear();

 private:
  struct Task {
    bool registered;
    bool enabled;
    uint8_t id;
    char name[kTaskNameMaxLength];
    uint32_t intervalMillis;
    uint32_t lastRunMillis;
    TaskCallback callback;
    void* context;
  };

  bool taskIsDue(const Task& task, uint32_t nowMillis) const;
  void publishTaskFailed(uint8_t taskId);

  platform::TimeSource& timeSource_;
  events::EventBus& eventBus_;
  logging::Logger* logger_ = nullptr;
  uint8_t nextTaskId_ = 1;
  Task tasks_[kMaxScheduledTasks] = {};
};

}  // namespace reeflow::core::scheduler
