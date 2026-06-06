#include <assert.h>
#include <stdint.h>

#include "core/events/event_bus.h"
#include "core/state/system_state.h"

namespace {

using reeflow::core::events::Event;
using reeflow::core::events::EventBus;
using reeflow::core::events::EventType;
using reeflow::core::events::StateArea;
using reeflow::core::state::TemperatureStatus;

struct EventRecord {
  Event event;
  uint8_t subscriber;
};

struct Recorder {
  EventRecord records[8];
  uint8_t count;
  bool shouldSucceed;
};

bool recordEvent(const Event& event, void* context) {
  Recorder* recorder = static_cast<Recorder*>(context);
  recorder->records[recorder->count].event = event;
  recorder->records[recorder->count].subscriber = recorder->count + 1;
  recorder->count += 1;
  return recorder->shouldSucceed;
}

bool recordSubscriberOne(const Event& event, void* context) {
  Recorder* recorder = static_cast<Recorder*>(context);
  recorder->records[recorder->count].event = event;
  recorder->records[recorder->count].subscriber = 1;
  recorder->count += 1;
  return recorder->shouldSucceed;
}

bool recordSubscriberTwo(const Event& event, void* context) {
  Recorder* recorder = static_cast<Recorder*>(context);
  recorder->records[recorder->count].event = event;
  recorder->records[recorder->count].subscriber = 2;
  recorder->count += 1;
  return recorder->shouldSucceed;
}

struct StateReadContext {
  bool observedUpdatedState;
  uint8_t count;
};

bool readStateDuringEvent(const Event& event, void* context) {
  StateReadContext* readContext = static_cast<StateReadContext*>(context);
  readContext->count += 1;
  readContext->observedUpdatedState =
      event.stateArea == StateArea::kTemperature &&
      reeflow::core::state::currentSystemState().temperature.status ==
          TemperatureStatus::kNormal &&
      reeflow::core::state::currentSystemState()
              .temperature.currentTemperature == 26.5F;
  return true;
}

Event stateChanged(StateArea area) {
  Event event = {};
  event.type = EventType::kSystemStateChanged;
  event.stateArea = area;
  return event;
}

void testPublishAndReceiveSimpleEvent() {
  EventBus bus;
  Recorder recorder = {};
  recorder.shouldSucceed = true;

  const uint8_t subscriptionId =
      bus.subscribe(EventType::kSystemStateChanged, recordEvent, &recorder);
  assert(subscriptionId != reeflow::core::events::kInvalidSubscriptionId);

  const reeflow::core::events::PublishResult result =
      bus.publish(stateChanged(StateArea::kTemperature));

  assert(result.deliveredCount == 1);
  assert(result.failedCount == 0);
  assert(recorder.count == 1);
  assert(recorder.records[0].event.type == EventType::kSystemStateChanged);
  assert(recorder.records[0].event.stateArea == StateArea::kTemperature);
  assert(recorder.records[0].event.sequence == 1);
}

void testMultipleSubscribersReceiveInSubscriptionOrder() {
  EventBus bus;
  Recorder recorder = {};
  recorder.shouldSucceed = true;

  bus.subscribe(EventType::kSystemStateChanged, recordSubscriberOne, &recorder);
  bus.subscribe(EventType::kSystemStateChanged, recordSubscriberTwo, &recorder);

  const reeflow::core::events::PublishResult result =
      bus.publish(stateChanged(StateArea::kRelays));

  assert(result.deliveredCount == 2);
  assert(result.failedCount == 0);
  assert(recorder.count == 2);
  assert(recorder.records[0].subscriber == 1);
  assert(recorder.records[1].subscriber == 2);
  assert(recorder.records[0].event.stateArea == StateArea::kRelays);
  assert(recorder.records[1].event.stateArea == StateArea::kRelays);
}

void testUnsubscribeStopsDelivery() {
  EventBus bus;
  Recorder recorder = {};
  recorder.shouldSucceed = true;

  const uint8_t firstId =
      bus.subscribe(EventType::kSystemStateChanged, recordSubscriberOne,
                    &recorder);
  bus.subscribe(EventType::kSystemStateChanged, recordSubscriberTwo, &recorder);

  assert(bus.unsubscribe(firstId));

  const reeflow::core::events::PublishResult result =
      bus.publish(stateChanged(StateArea::kNetwork));

  assert(result.deliveredCount == 1);
  assert(result.failedCount == 0);
  assert(recorder.count == 1);
  assert(recorder.records[0].subscriber == 2);
}

void testPublishWithNoSubscribers() {
  EventBus bus;

  const reeflow::core::events::PublishResult result =
      bus.publish(stateChanged(StateArea::kAto));

  assert(result.deliveredCount == 0);
  assert(result.failedCount == 0);
}

void testFailingSubscriberDoesNotBlockOthers() {
  EventBus bus;
  Recorder failingRecorder = {};
  failingRecorder.shouldSucceed = false;
  Recorder successfulRecorder = {};
  successfulRecorder.shouldSucceed = true;

  bus.subscribe(EventType::kSystemStateChanged, recordEvent, &failingRecorder);
  bus.subscribe(EventType::kSystemStateChanged, recordEvent,
                &successfulRecorder);

  const reeflow::core::events::PublishResult result =
      bus.publish(stateChanged(StateArea::kSystemHealth));

  assert(result.deliveredCount == 1);
  assert(result.failedCount == 1);
  assert(failingRecorder.count == 1);
  assert(successfulRecorder.count == 1);
}

void testSystemStateUpdatePublishesChangedArea() {
  reeflow::core::events::defaultEventBus().reset();
  reeflow::core::state::resetSystemState();

  StateReadContext readContext = {};
  reeflow::core::events::defaultEventBus().subscribe(
      EventType::kSystemStateChanged, readStateDuringEvent, &readContext);

  reeflow::core::state::TemperatureState temperature =
      reeflow::core::state::currentSystemState().temperature;
  temperature.currentTemperature = 26.5F;
  temperature.status = TemperatureStatus::kNormal;
  temperature.lastUpdate = 100;

  reeflow::core::state::updateTemperatureState(temperature);

  assert(readContext.count == 1);
  assert(readContext.observedUpdatedState);
}

void testUnchangedSystemStateDoesNotPublishDuplicateEvent() {
  reeflow::core::events::defaultEventBus().reset();
  reeflow::core::state::resetSystemState();

  Recorder recorder = {};
  recorder.shouldSucceed = true;
  reeflow::core::events::defaultEventBus().subscribe(
      EventType::kSystemStateChanged, recordEvent, &recorder);

  reeflow::core::state::TemperatureState temperature =
      reeflow::core::state::currentSystemState().temperature;
  temperature.currentTemperature = 26.5F;
  temperature.status = TemperatureStatus::kNormal;
  temperature.lastUpdate = 100;

  reeflow::core::state::updateTemperatureState(temperature);
  reeflow::core::state::updateTemperatureState(temperature);

  assert(recorder.count == 1);
  assert(recorder.records[0].event.stateArea == StateArea::kTemperature);
}

}  // namespace

int main() {
  testPublishAndReceiveSimpleEvent();
  testMultipleSubscribersReceiveInSubscriptionOrder();
  testUnsubscribeStopsDelivery();
  testPublishWithNoSubscribers();
  testFailingSubscriberDoesNotBlockOthers();
  testSystemStateUpdatePublishesChangedArea();
  testUnchangedSystemStateDoesNotPublishDuplicateEvent();
  return 0;
}
