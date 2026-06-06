#include "core/scheduler/task_scheduler.h"

#include <string.h>

namespace reeflow::core::scheduler {
namespace {

bool validTaskName(const char* name) {
  if (name == nullptr || name[0] == '\0') {
    return false;
  }

  return memchr(name, '\0', kTaskNameMaxLength) != nullptr;
}

void copyTaskName(char* target, const char* source) {
  strncpy(target, source, kTaskNameMaxLength - 1);
  target[kTaskNameMaxLength - 1] = '\0';
}

}  // namespace

TaskScheduler::TaskScheduler(platform::TimeSource& timeSource,
                             events::EventBus& eventBus)
    : timeSource_(timeSource), eventBus_(eventBus) {}

TaskScheduler::TaskScheduler(platform::TimeSource& timeSource,
                             events::EventBus& eventBus,
                             logging::Logger& logger)
    : timeSource_(timeSource), eventBus_(eventBus), logger_(&logger) {}

uint8_t TaskScheduler::registerTask(const char* name, uint32_t intervalMillis,
                                    TaskCallback callback, void* context) {
  if (!validTaskName(name) || intervalMillis == 0 || callback == nullptr) {
    return kInvalidTaskId;
  }

  for (size_t index = 0; index < kMaxScheduledTasks; ++index) {
    Task& task = tasks_[index];
    if (task.registered) {
      continue;
    }

    task = {};
    task.registered = true;
    task.enabled = true;
    task.id = nextTaskId_++;
    if (nextTaskId_ == kInvalidTaskId) {
      nextTaskId_ = 1;
    }
    copyTaskName(task.name, name);
    task.intervalMillis = intervalMillis;
    task.lastRunMillis = timeSource_.uptimeMillis();
    task.callback = callback;
    task.context = context;
    return task.id;
  }

  return kInvalidTaskId;
}

bool TaskScheduler::setTaskEnabled(uint8_t taskId, bool enabled) {
  if (taskId == kInvalidTaskId) {
    return false;
  }

  for (size_t index = 0; index < kMaxScheduledTasks; ++index) {
    Task& task = tasks_[index];
    if (!task.registered || task.id != taskId) {
      continue;
    }

    task.enabled = enabled;
    if (enabled) {
      task.lastRunMillis = timeSource_.uptimeMillis();
    }
    return true;
  }

  return false;
}

SchedulerRunResult TaskScheduler::runDueTasks() {
  SchedulerRunResult result = {};
  const uint32_t nowMillis = timeSource_.uptimeMillis();

  for (size_t index = 0; index < kMaxScheduledTasks; ++index) {
    Task& task = tasks_[index];
    if (!taskIsDue(task, nowMillis)) {
      continue;
    }

    task.lastRunMillis = nowMillis;
    result.executedCount += 1;
    if (!task.callback(task.context)) {
      result.failedCount += 1;
      if (logger_ != nullptr) {
        logger_->error("scheduler", "task failed");
      }
      publishTaskFailed(task.id);
    }
  }

  return result;
}

void TaskScheduler::clear() {
  nextTaskId_ = 1;
  for (size_t index = 0; index < kMaxScheduledTasks; ++index) {
    tasks_[index] = {};
  }
}

bool TaskScheduler::taskIsDue(const Task& task, uint32_t nowMillis) const {
  return task.registered && task.enabled &&
         static_cast<uint32_t>(nowMillis - task.lastRunMillis) >=
             task.intervalMillis;
}

void TaskScheduler::publishTaskFailed(uint8_t taskId) {
  events::Event event = {};
  event.type = events::EventType::kSchedulerTaskFailed;
  event.schedulerTaskId = taskId;
  eventBus_.publish(event);
}

}  // namespace reeflow::core::scheduler
