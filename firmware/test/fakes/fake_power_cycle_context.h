#pragma once

#include <stddef.h>
#include <stdint.h>

#include "core/events/event_bus.h"
#include "core/scheduler/task_scheduler.h"
#include "core/state/system_state.h"
#include "fakes/fake_time_source.h"

namespace reeflow::test::fakes {

class FakePowerCycleContext {
 public:
  static constexpr size_t kMaxCapturedEvents = 16;

  bool start() {
    eventBus_.reset();
    capturedEventCount_ = 0;
    subscribeToLocalEvents();
    core::state::setSystemStateEventBus(eventBus_);
    core::state::resetSystemState();
    scheduler_.clear();
    started_ = true;
    return true;
  }

  bool simulatedBoot() {
    started_ = false;
    timeSource_.setUptimeMillis(0);
    return start();
  }

  uint8_t registerTask(const char* name, uint32_t intervalMillis,
                       core::scheduler::TaskCallback callback, void* context) {
    return scheduler_.registerTask(name, intervalMillis, callback, context);
  }

  core::scheduler::SchedulerRunResult runSchedulerFor(uint32_t deltaMillis) {
    timeSource_.advanceMillis(deltaMillis);
    return scheduler_.runDueTasks();
  }

  void publish(const core::events::Event& event) { eventBus_.publish(event); }

  bool started() const { return started_; }
  uint32_t nowMillis() const { return timeSource_.uptimeMillis(); }
  core::events::EventBus& eventBus() { return eventBus_; }
  const core::state::SystemState& state() const {
    return core::state::currentSystemState();
  }

  size_t capturedEventCount() const { return capturedEventCount_; }
  core::events::Event capturedEvent(size_t index) const {
    return capturedEvents_[index];
  }

 private:
  static bool captureEvent(const core::events::Event& event, void* context) {
    FakePowerCycleContext* self =
        static_cast<FakePowerCycleContext*>(context);
    if (self == nullptr || self->capturedEventCount_ >= kMaxCapturedEvents) {
      return false;
    }

    self->capturedEvents_[self->capturedEventCount_++] = event;
    return true;
  }

  void subscribeToLocalEvents() {
    eventBus_.subscribe(core::events::EventType::kSystemStateChanged,
                        captureEvent, this);
    eventBus_.subscribe(core::events::EventType::kConfigRestored,
                        captureEvent, this);
    eventBus_.subscribe(core::events::EventType::kAlertRaised, captureEvent,
                        this);
  }

  FakeTimeSource timeSource_;
  core::events::EventBus eventBus_;
  core::scheduler::TaskScheduler scheduler_{timeSource_, eventBus_};
  core::events::Event capturedEvents_[kMaxCapturedEvents] = {};
  size_t capturedEventCount_ = 0;
  bool started_ = false;
};

}  // namespace reeflow::test::fakes
