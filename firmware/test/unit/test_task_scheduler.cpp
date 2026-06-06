#include <assert.h>
#include <stdint.h>

#include "core/events/event_bus.h"
#include "core/scheduler/task_scheduler.h"
#include "fakes/fake_time_source.h"

namespace {

using reeflow::core::events::Event;
using reeflow::core::events::EventBus;
using reeflow::core::events::EventType;
using reeflow::core::scheduler::SchedulerRunResult;
using reeflow::core::scheduler::TaskScheduler;
using reeflow::test::fakes::FakeTimeSource;

struct TaskCounter {
  uint8_t runCount;
  bool shouldSucceed;
};

struct EventRecorder {
  uint8_t count;
  Event lastEvent;
};

bool runTask(void* context) {
  TaskCounter* counter = static_cast<TaskCounter*>(context);
  counter->runCount += 1;
  return counter->shouldSucceed;
}

bool recordEvent(const Event& event, void* context) {
  EventRecorder* recorder = static_cast<EventRecorder*>(context);
  recorder->count += 1;
  recorder->lastEvent = event;
  return true;
}

void testRegisterPeriodicTask() {
  FakeTimeSource timeSource;
  EventBus eventBus;
  TaskScheduler scheduler(timeSource, eventBus);
  TaskCounter counter = {};
  counter.shouldSucceed = true;

  const uint8_t taskId =
      scheduler.registerTask("sample", 1000, runTask, &counter);

  assert(taskId != reeflow::core::scheduler::kInvalidTaskId);
  assert(counter.runCount == 0);
}

void testTaskRunsOnlyWhenIntervalExpires() {
  FakeTimeSource timeSource;
  EventBus eventBus;
  TaskScheduler scheduler(timeSource, eventBus);
  TaskCounter counter = {};
  counter.shouldSucceed = true;

  scheduler.registerTask("sample", 1000, runTask, &counter);

  timeSource.advanceMillis(999);
  SchedulerRunResult result = scheduler.runDueTasks();
  assert(result.executedCount == 0);
  assert(counter.runCount == 0);

  timeSource.advanceMillis(1);
  result = scheduler.runDueTasks();
  assert(result.executedCount == 1);
  assert(result.failedCount == 0);
  assert(counter.runCount == 1);

  result = scheduler.runDueTasks();
  assert(result.executedCount == 0);
  assert(counter.runCount == 1);
}

void testDisableAndReenableTask() {
  FakeTimeSource timeSource;
  EventBus eventBus;
  TaskScheduler scheduler(timeSource, eventBus);
  TaskCounter counter = {};
  counter.shouldSucceed = true;

  const uint8_t taskId =
      scheduler.registerTask("sample", 1000, runTask, &counter);

  assert(scheduler.setTaskEnabled(taskId, false));
  timeSource.advanceMillis(1500);
  SchedulerRunResult result = scheduler.runDueTasks();
  assert(result.executedCount == 0);
  assert(counter.runCount == 0);

  assert(scheduler.setTaskEnabled(taskId, true));
  timeSource.advanceMillis(999);
  result = scheduler.runDueTasks();
  assert(result.executedCount == 0);
  assert(counter.runCount == 0);

  timeSource.advanceMillis(1);
  result = scheduler.runDueTasks();
  assert(result.executedCount == 1);
  assert(counter.runCount == 1);
}

void testMultipleTasksWithDifferentIntervals() {
  FakeTimeSource timeSource;
  EventBus eventBus;
  TaskScheduler scheduler(timeSource, eventBus);
  TaskCounter fast = {};
  fast.shouldSucceed = true;
  TaskCounter slow = {};
  slow.shouldSucceed = true;

  scheduler.registerTask("fast", 500, runTask, &fast);
  scheduler.registerTask("slow", 1000, runTask, &slow);

  timeSource.advanceMillis(500);
  SchedulerRunResult result = scheduler.runDueTasks();
  assert(result.executedCount == 1);
  assert(fast.runCount == 1);
  assert(slow.runCount == 0);

  timeSource.advanceMillis(500);
  result = scheduler.runDueTasks();
  assert(result.executedCount == 2);
  assert(fast.runCount == 2);
  assert(slow.runCount == 1);
}

void testFailurePublishesLocalEvent() {
  FakeTimeSource timeSource;
  EventBus eventBus;
  TaskScheduler scheduler(timeSource, eventBus);
  TaskCounter failing = {};
  failing.shouldSucceed = false;
  EventRecorder recorder = {};

  eventBus.subscribe(EventType::kSchedulerTaskFailed, recordEvent, &recorder);

  const uint8_t taskId =
      scheduler.registerTask("failing", 1000, runTask, &failing);

  timeSource.advanceMillis(1000);
  const SchedulerRunResult result = scheduler.runDueTasks();

  assert(result.executedCount == 1);
  assert(result.failedCount == 1);
  assert(failing.runCount == 1);
  assert(recorder.count == 1);
  assert(recorder.lastEvent.type == EventType::kSchedulerTaskFailed);
  assert(recorder.lastEvent.schedulerTaskId == taskId);
}

}  // namespace

int main() {
  testRegisterPeriodicTask();
  testTaskRunsOnlyWhenIntervalExpires();
  testDisableAndReenableTask();
  testMultipleTasksWithDifferentIntervals();
  testFailurePublishesLocalEvent();
  return 0;
}
