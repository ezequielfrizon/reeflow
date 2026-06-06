#include <assert.h>
#include <stdint.h>

#include "config/config_manager.h"
#include "core/events/event_bus.h"
#include "core/state/system_state.h"
#include "fakes/fake_temperature_sensor.h"
#include "fakes/fake_time_source.h"
#include "modules/temperature/temperature_service.h"

namespace {

using reeflow::config::ConfigManager;
using reeflow::config::TemperatureConfig;
using reeflow::core::events::Event;
using reeflow::core::events::EventBus;
using reeflow::core::events::EventType;
using reeflow::core::state::AtoStatus;
using reeflow::core::state::TemperatureStatus;
using reeflow::core::state::WaterLevelStatus;
using reeflow::modules::temperature::TemperatureService;
using reeflow::test::fakes::FakeTemperatureSensor;
using reeflow::test::fakes::FakeTimeSource;

struct EventRecorder {
  Event events[16];
  uint8_t count;
};

bool recordEvent(const Event& event, void* context) {
  EventRecorder* recorder = static_cast<EventRecorder*>(context);
  recorder->events[recorder->count] = event;
  recorder->count += 1;
  return true;
}

void subscribeTemperatureEvents(EventBus& bus, EventRecorder& recorder) {
  bus.subscribe(EventType::kTemperatureUpdated, recordEvent, &recorder);
  bus.subscribe(EventType::kTemperatureHigh, recordEvent, &recorder);
  bus.subscribe(EventType::kTemperatureLow, recordEvent, &recorder);
  bus.subscribe(EventType::kTemperatureSensorOffline, recordEvent, &recorder);
  bus.subscribe(EventType::kTemperatureSensorRecovered, recordEvent,
                &recorder);
}

void resetState() {
  reeflow::core::events::defaultEventBus().reset();
  reeflow::core::state::resetSystemState();
}

void testValidReadingUpdatesTemperatureStateAndPublishesUpdated() {
  resetState();
  EventBus bus;
  EventRecorder recorder = {};
  subscribeTemperatureEvents(bus, recorder);
  ConfigManager configManager(bus);
  FakeTimeSource timeSource;
  FakeTemperatureSensor sensor;
  timeSource.setUptimeMillis(1000);
  sensor.setValidTemperature(26.5F);
  TemperatureService service(sensor, configManager, timeSource, bus);

  assert(service.runOnce());

  const auto& temperature =
      reeflow::core::state::currentSystemState().temperature;
  assert(temperature.currentTemperature == 26.5F);
  assert(temperature.minTemperature == 25.0F);
  assert(temperature.maxTemperature == 28.0F);
  assert(temperature.status == TemperatureStatus::kNormal);
  assert(temperature.lastUpdate == 1000);
  assert(recorder.count == 1);
  assert(recorder.events[0].type == EventType::kTemperatureUpdated);
}

void testHighAndLowStatusesPublishTransitionEvents() {
  resetState();
  EventBus bus;
  EventRecorder recorder = {};
  subscribeTemperatureEvents(bus, recorder);
  ConfigManager configManager(bus);
  FakeTimeSource timeSource;
  FakeTemperatureSensor sensor;
  timeSource.setUptimeMillis(1000);
  TemperatureService service(sensor, configManager, timeSource, bus);

  sensor.setHighTemperature();
  assert(service.runOnce());
  timeSource.advanceMillis(5000);
  sensor.setLowTemperature();
  assert(service.runOnce());

  assert(reeflow::core::state::currentSystemState().temperature.status ==
         TemperatureStatus::kLow);
  assert(recorder.count == 4);
  assert(recorder.events[0].type == EventType::kTemperatureUpdated);
  assert(recorder.events[1].type == EventType::kTemperatureHigh);
  assert(recorder.events[2].type == EventType::kTemperatureUpdated);
  assert(recorder.events[3].type == EventType::kTemperatureLow);
}

void testIsolatedFailurePreservesLastValidStateAndLastUpdate() {
  resetState();
  EventBus bus;
  EventRecorder recorder = {};
  subscribeTemperatureEvents(bus, recorder);
  ConfigManager configManager(bus);
  FakeTimeSource timeSource;
  FakeTemperatureSensor sensor;
  timeSource.setUptimeMillis(1000);
  sensor.setValidTemperature(26.0F);
  TemperatureService service(sensor, configManager, timeSource, bus);
  assert(service.runOnce());

  timeSource.setUptimeMillis(2000);
  sensor.setReadError();
  assert(!service.runOnce());

  const auto& temperature =
      reeflow::core::state::currentSystemState().temperature;
  assert(temperature.currentTemperature == 26.0F);
  assert(temperature.status == TemperatureStatus::kNormal);
  assert(temperature.lastUpdate == 1000);
  assert(recorder.count == 1);
}

void testFailureTimeoutBoundariesAfterLastValidReading() {
  resetState();
  EventBus bus;
  EventRecorder recorder = {};
  subscribeTemperatureEvents(bus, recorder);
  ConfigManager configManager(bus);
  FakeTimeSource timeSource;
  FakeTemperatureSensor sensor;
  timeSource.setUptimeMillis(1000);
  sensor.setValidTemperature(26.0F);
  TemperatureService service(sensor, configManager, timeSource, bus);
  assert(service.runOnce());

  sensor.setSensorNotFound();
  timeSource.setUptimeMillis(30999);
  assert(!service.runOnce());
  timeSource.setUptimeMillis(31000);
  assert(!service.runOnce());
  timeSource.setUptimeMillis(31001);
  assert(service.runOnce());

  const auto& temperature =
      reeflow::core::state::currentSystemState().temperature;
  assert(temperature.currentTemperature == 26.0F);
  assert(temperature.status == TemperatureStatus::kSensorOffline);
  assert(temperature.lastUpdate == 1000);
  assert(recorder.count == 2);
  assert(recorder.events[1].type == EventType::kTemperatureSensorOffline);
}

void testNoValidReadingSinceBootCanBecomeOffline() {
  resetState();
  EventBus bus;
  EventRecorder recorder = {};
  subscribeTemperatureEvents(bus, recorder);
  ConfigManager configManager(bus);
  FakeTimeSource timeSource;
  FakeTemperatureSensor sensor;
  timeSource.setUptimeMillis(500);
  sensor.setReadError();
  TemperatureService service(sensor, configManager, timeSource, bus);

  assert(!service.runOnce());
  timeSource.setUptimeMillis(30500);
  assert(!service.runOnce());
  timeSource.setUptimeMillis(30501);
  assert(service.runOnce());

  const auto& temperature =
      reeflow::core::state::currentSystemState().temperature;
  assert(temperature.status == TemperatureStatus::kSensorOffline);
  assert(temperature.lastUpdate == 0);
  assert(recorder.count == 1);
  assert(recorder.events[0].type == EventType::kTemperatureSensorOffline);
}

void testConfiguredLimitsAreWrittenToTemperatureState() {
  resetState();
  EventBus bus;
  EventRecorder recorder = {};
  subscribeTemperatureEvents(bus, recorder);
  ConfigManager configManager(bus);
  TemperatureConfig config = configManager.temperature();
  config.minTemperature = 26.0F;
  config.maxTemperature = 27.0F;
  config.targetTemperature = 26.5F;
  assert(configManager.updateTemperature(config));
  FakeTimeSource timeSource;
  FakeTemperatureSensor sensor;
  timeSource.setUptimeMillis(1000);
  sensor.setValidTemperature(27.5F);
  TemperatureService service(sensor, configManager, timeSource, bus);

  assert(service.runOnce());

  const auto& temperature =
      reeflow::core::state::currentSystemState().temperature;
  assert(temperature.minTemperature == 26.0F);
  assert(temperature.maxTemperature == 27.0F);
  assert(temperature.status == TemperatureStatus::kHigh);
}

void testUnrelatedStateBlocksArePreserved() {
  resetState();
  reeflow::core::state::WaterLevelState waterLevel =
      reeflow::core::state::currentSystemState().waterLevel;
  waterLevel.currentLevel = 42;
  waterLevel.status = WaterLevelStatus::kNormal;
  reeflow::core::state::updateWaterLevelState(waterLevel);
  reeflow::core::state::AtoState ato =
      reeflow::core::state::currentSystemState().ato;
  ato.enabled = true;
  ato.status = AtoStatus::kNormal;
  reeflow::core::state::updateAtoState(ato);

  EventBus bus;
  EventRecorder recorder = {};
  subscribeTemperatureEvents(bus, recorder);
  ConfigManager configManager(bus);
  FakeTimeSource timeSource;
  FakeTemperatureSensor sensor;
  timeSource.setUptimeMillis(1000);
  sensor.setValidTemperature(26.5F);
  TemperatureService service(sensor, configManager, timeSource, bus);
  assert(service.runOnce());

  const auto& state = reeflow::core::state::currentSystemState();
  assert(state.waterLevel.currentLevel == 42);
  assert(state.waterLevel.status == WaterLevelStatus::kNormal);
  assert(state.ato.enabled);
  assert(state.ato.status == AtoStatus::kNormal);
}

void testRecoveryFromOfflinePublishesRecoveryOnce() {
  resetState();
  EventBus bus;
  EventRecorder recorder = {};
  subscribeTemperatureEvents(bus, recorder);
  ConfigManager configManager(bus);
  FakeTimeSource timeSource;
  FakeTemperatureSensor sensor;
  timeSource.setUptimeMillis(1000);
  sensor.setReadError();
  TemperatureService service(sensor, configManager, timeSource, bus);
  timeSource.setUptimeMillis(31001);
  assert(service.runOnce());

  sensor.setValidTemperature(26.5F);
  timeSource.setUptimeMillis(36001);
  assert(service.runOnce());
  timeSource.setUptimeMillis(41001);
  assert(service.runOnce());

  assert(recorder.count == 4);
  assert(recorder.events[0].type == EventType::kTemperatureSensorOffline);
  assert(recorder.events[1].type == EventType::kTemperatureUpdated);
  assert(recorder.events[2].type == EventType::kTemperatureSensorRecovered);
  assert(recorder.events[3].type == EventType::kTemperatureUpdated);
}

void testRepeatedStatusDoesNotDuplicateTransitionEvent() {
  resetState();
  EventBus bus;
  EventRecorder recorder = {};
  subscribeTemperatureEvents(bus, recorder);
  ConfigManager configManager(bus);
  FakeTimeSource timeSource;
  FakeTemperatureSensor sensor;
  timeSource.setUptimeMillis(1000);
  sensor.setHighTemperature();
  TemperatureService service(sensor, configManager, timeSource, bus);
  assert(service.runOnce());
  timeSource.setUptimeMillis(6000);
  assert(service.runOnce());

  assert(recorder.count == 3);
  assert(recorder.events[0].type == EventType::kTemperatureUpdated);
  assert(recorder.events[1].type == EventType::kTemperatureHigh);
  assert(recorder.events[2].type == EventType::kTemperatureUpdated);
}

void testConversionPendingIsNonBlockingSuccessfulIteration() {
  resetState();
  EventBus bus;
  EventRecorder recorder = {};
  subscribeTemperatureEvents(bus, recorder);
  ConfigManager configManager(bus);
  FakeTimeSource timeSource;
  FakeTemperatureSensor sensor;
  timeSource.setUptimeMillis(1000);
  sensor.setConversionPending();
  TemperatureService service(sensor, configManager, timeSource, bus);

  assert(service.runOnce());

  const auto& temperature =
      reeflow::core::state::currentSystemState().temperature;
  assert(temperature.status == TemperatureStatus::kSensorOffline);
  assert(temperature.lastUpdate == 0);
  assert(recorder.count == 0);
}

}  // namespace

int main() {
  testValidReadingUpdatesTemperatureStateAndPublishesUpdated();
  testHighAndLowStatusesPublishTransitionEvents();
  testIsolatedFailurePreservesLastValidStateAndLastUpdate();
  testFailureTimeoutBoundariesAfterLastValidReading();
  testNoValidReadingSinceBootCanBecomeOffline();
  testConfiguredLimitsAreWrittenToTemperatureState();
  testUnrelatedStateBlocksArePreserved();
  testRecoveryFromOfflinePublishesRecoveryOnce();
  testRepeatedStatusDoesNotDuplicateTransitionEvent();
  testConversionPendingIsNonBlockingSuccessfulIteration();
  return 0;
}
