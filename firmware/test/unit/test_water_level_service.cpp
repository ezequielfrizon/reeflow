#include <assert.h>
#include <stdint.h>

#include "config/config_manager.h"
#include "core/events/event_bus.h"
#include "core/state/system_state.h"
#include "fakes/fake_time_source.h"
#include "fakes/fake_water_level_sensor.h"
#include "modules/water_level/water_level_service.h"

namespace {

using reeflow::config::AtoConfig;
using reeflow::config::CalibrationsConfig;
using reeflow::config::ConfigManager;
using reeflow::core::events::Event;
using reeflow::core::events::EventBus;
using reeflow::core::events::EventType;
using reeflow::core::state::AtoStatus;
using reeflow::core::state::TemperatureStatus;
using reeflow::core::state::WaterLevelStatus;
using reeflow::modules::water_level::WaterLevelService;
using reeflow::test::fakes::FakeTimeSource;
using reeflow::test::fakes::FakeWaterLevelSensor;

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

void subscribeWaterLevelEvents(EventBus& bus, EventRecorder& recorder) {
  bus.subscribe(EventType::kWaterLevelUpdated, recordEvent, &recorder);
  bus.subscribe(EventType::kWaterLevelLow, recordEvent, &recorder);
  bus.subscribe(EventType::kWaterLevelHigh, recordEvent, &recorder);
  bus.subscribe(EventType::kWaterLevelSensorOffline, recordEvent, &recorder);
  bus.subscribe(EventType::kWaterLevelSensorRecovered, recordEvent,
                &recorder);
}

void resetState() {
  reeflow::core::events::defaultEventBus().reset();
  reeflow::core::state::resetSystemState();
}

void testValidReadingUpdatesWaterLevelStateAndPublishesUpdated() {
  resetState();
  EventBus bus;
  EventRecorder recorder = {};
  subscribeWaterLevelEvents(bus, recorder);
  ConfigManager configManager(bus);
  FakeTimeSource timeSource;
  FakeWaterLevelSensor sensor;
  timeSource.setUptimeMillis(1000);
  sensor.setValidLevel(50);
  WaterLevelService service(sensor, configManager, timeSource, bus);

  assert(service.runOnce());

  const auto& waterLevel =
      reeflow::core::state::currentSystemState().waterLevel;
  assert(waterLevel.currentLevel == 50);
  assert(waterLevel.minimumLevel == 20);
  assert(waterLevel.maximumLevel == 80);
  assert(waterLevel.status == WaterLevelStatus::kNormal);
  assert(waterLevel.lastUpdate == 1000);
  assert(recorder.count == 1);
  assert(recorder.events[0].type == EventType::kWaterLevelUpdated);
}

void testLowAndHighStatusesPublishTransitionEvents() {
  resetState();
  EventBus bus;
  EventRecorder recorder = {};
  subscribeWaterLevelEvents(bus, recorder);
  ConfigManager configManager(bus);
  FakeTimeSource timeSource;
  FakeWaterLevelSensor sensor;
  timeSource.setUptimeMillis(1000);
  WaterLevelService service(sensor, configManager, timeSource, bus);

  sensor.setValidLevel(10);
  assert(service.runOnce());
  timeSource.advanceMillis(5000);
  sensor.setValidLevel(90);
  assert(service.runOnce());

  assert(reeflow::core::state::currentSystemState().waterLevel.status ==
         WaterLevelStatus::kHigh);
  assert(recorder.count == 4);
  assert(recorder.events[0].type == EventType::kWaterLevelUpdated);
  assert(recorder.events[1].type == EventType::kWaterLevelLow);
  assert(recorder.events[2].type == EventType::kWaterLevelUpdated);
  assert(recorder.events[3].type == EventType::kWaterLevelHigh);
}

void testIsolatedFailurePreservesLastValidStateAndLastUpdate() {
  resetState();
  EventBus bus;
  EventRecorder recorder = {};
  subscribeWaterLevelEvents(bus, recorder);
  ConfigManager configManager(bus);
  FakeTimeSource timeSource;
  FakeWaterLevelSensor sensor;
  timeSource.setUptimeMillis(1000);
  sensor.setValidLevel(45);
  WaterLevelService service(sensor, configManager, timeSource, bus);
  assert(service.runOnce());

  timeSource.setUptimeMillis(2000);
  sensor.setReadError();
  assert(!service.runOnce());

  const auto& waterLevel =
      reeflow::core::state::currentSystemState().waterLevel;
  assert(waterLevel.currentLevel == 45);
  assert(waterLevel.status == WaterLevelStatus::kNormal);
  assert(waterLevel.lastUpdate == 1000);
  assert(recorder.count == 1);
}

void testFailureTimeoutBoundariesAfterLastValidReading() {
  resetState();
  EventBus bus;
  EventRecorder recorder = {};
  subscribeWaterLevelEvents(bus, recorder);
  ConfigManager configManager(bus);
  FakeTimeSource timeSource;
  FakeWaterLevelSensor sensor;
  timeSource.setUptimeMillis(1000);
  sensor.setValidLevel(55);
  WaterLevelService service(sensor, configManager, timeSource, bus);
  assert(service.runOnce());

  sensor.setSensorNotFound();
  timeSource.setUptimeMillis(30999);
  assert(!service.runOnce());
  timeSource.setUptimeMillis(31000);
  assert(!service.runOnce());
  timeSource.setUptimeMillis(31001);
  assert(service.runOnce());

  const auto& waterLevel =
      reeflow::core::state::currentSystemState().waterLevel;
  assert(waterLevel.currentLevel == 55);
  assert(waterLevel.status == WaterLevelStatus::kSensorOffline);
  assert(waterLevel.lastUpdate == 1000);
  assert(recorder.count == 2);
  assert(recorder.events[1].type == EventType::kWaterLevelSensorOffline);
}

void testNoValidReadingSinceBootCanBecomeOffline() {
  resetState();
  EventBus bus;
  EventRecorder recorder = {};
  subscribeWaterLevelEvents(bus, recorder);
  ConfigManager configManager(bus);
  FakeTimeSource timeSource;
  FakeWaterLevelSensor sensor;
  timeSource.setUptimeMillis(500);
  sensor.setReadError();
  WaterLevelService service(sensor, configManager, timeSource, bus);

  assert(!service.runOnce());
  timeSource.setUptimeMillis(30500);
  assert(!service.runOnce());
  timeSource.setUptimeMillis(30501);
  assert(service.runOnce());

  const auto& waterLevel =
      reeflow::core::state::currentSystemState().waterLevel;
  assert(waterLevel.status == WaterLevelStatus::kSensorOffline);
  assert(waterLevel.lastUpdate == 0);
  assert(waterLevel.minimumLevel == 20);
  assert(waterLevel.maximumLevel == 80);
  assert(recorder.count == 1);
  assert(recorder.events[0].type == EventType::kWaterLevelSensorOffline);
}

void testConfiguredLimitsAreWrittenToWaterLevelState() {
  resetState();
  EventBus bus;
  EventRecorder recorder = {};
  subscribeWaterLevelEvents(bus, recorder);
  ConfigManager configManager(bus);
  AtoConfig ato = configManager.ato();
  ato.minimumLevel = 40;
  ato.maximumLevel = 60;
  assert(configManager.updateAto(ato));
  FakeTimeSource timeSource;
  FakeWaterLevelSensor sensor;
  timeSource.setUptimeMillis(1000);
  sensor.setValidLevel(39);
  WaterLevelService service(sensor, configManager, timeSource, bus);

  assert(service.runOnce());

  const auto& waterLevel =
      reeflow::core::state::currentSystemState().waterLevel;
  assert(waterLevel.minimumLevel == 40);
  assert(waterLevel.maximumLevel == 60);
  assert(waterLevel.status == WaterLevelStatus::kLow);
}

void testCalibrationOffsetAffectsWaterLevelState() {
  resetState();
  EventBus bus;
  EventRecorder recorder = {};
  subscribeWaterLevelEvents(bus, recorder);
  ConfigManager configManager(bus);
  CalibrationsConfig calibrations = configManager.calibrations();
  calibrations.waterLevelOffset = 5;
  assert(configManager.updateCalibrations(calibrations));
  FakeTimeSource timeSource;
  FakeWaterLevelSensor sensor;
  timeSource.setUptimeMillis(1000);
  sensor.setValidLevel(78);
  WaterLevelService service(sensor, configManager, timeSource, bus);

  assert(service.runOnce());

  const auto& waterLevel =
      reeflow::core::state::currentSystemState().waterLevel;
  assert(waterLevel.currentLevel == 83);
  assert(waterLevel.currentLevel <= 100);
  assert(waterLevel.status == WaterLevelStatus::kHigh);
}

void testUnrelatedStateBlocksAndAtoArePreserved() {
  resetState();
  reeflow::core::state::TemperatureState temperature =
      reeflow::core::state::currentSystemState().temperature;
  temperature.currentTemperature = 26.5F;
  temperature.status = TemperatureStatus::kNormal;
  reeflow::core::state::updateTemperatureState(temperature);
  reeflow::core::state::AtoState ato =
      reeflow::core::state::currentSystemState().ato;
  ato.enabled = true;
  ato.status = AtoStatus::kNormal;
  ato.pumpRunning = true;
  ato.lastActivation = 10;
  ato.lastCompletion = 20;
  ato.timeoutCounter = 3;
  reeflow::core::state::updateAtoState(ato);

  EventBus bus;
  EventRecorder recorder = {};
  subscribeWaterLevelEvents(bus, recorder);
  ConfigManager configManager(bus);
  FakeTimeSource timeSource;
  FakeWaterLevelSensor sensor;
  timeSource.setUptimeMillis(1000);
  sensor.setValidLevel(50);
  WaterLevelService service(sensor, configManager, timeSource, bus);
  assert(service.runOnce());

  const auto& state = reeflow::core::state::currentSystemState();
  assert(state.temperature.currentTemperature == 26.5F);
  assert(state.temperature.status == TemperatureStatus::kNormal);
  assert(state.ato.enabled);
  assert(state.ato.status == AtoStatus::kNormal);
  assert(state.ato.pumpRunning);
  assert(state.ato.lastActivation == 10);
  assert(state.ato.lastCompletion == 20);
  assert(state.ato.timeoutCounter == 3);
}

void testRecoveryFromOfflinePublishesRecoveryOnce() {
  resetState();
  EventBus bus;
  EventRecorder recorder = {};
  subscribeWaterLevelEvents(bus, recorder);
  ConfigManager configManager(bus);
  FakeTimeSource timeSource;
  FakeWaterLevelSensor sensor;
  timeSource.setUptimeMillis(1000);
  sensor.setReadError();
  WaterLevelService service(sensor, configManager, timeSource, bus);
  timeSource.setUptimeMillis(31001);
  assert(service.runOnce());

  sensor.setValidLevel(50);
  timeSource.setUptimeMillis(36001);
  assert(service.runOnce());
  timeSource.setUptimeMillis(41001);
  assert(service.runOnce());

  assert(recorder.count == 4);
  assert(recorder.events[0].type == EventType::kWaterLevelSensorOffline);
  assert(recorder.events[1].type == EventType::kWaterLevelUpdated);
  assert(recorder.events[2].type == EventType::kWaterLevelSensorRecovered);
  assert(recorder.events[3].type == EventType::kWaterLevelUpdated);
}

void testRepeatedStatusDoesNotDuplicateTransitionEvent() {
  resetState();
  EventBus bus;
  EventRecorder recorder = {};
  subscribeWaterLevelEvents(bus, recorder);
  ConfigManager configManager(bus);
  FakeTimeSource timeSource;
  FakeWaterLevelSensor sensor;
  timeSource.setUptimeMillis(1000);
  sensor.setValidLevel(90);
  WaterLevelService service(sensor, configManager, timeSource, bus);
  assert(service.runOnce());
  timeSource.setUptimeMillis(6000);
  assert(service.runOnce());

  assert(recorder.count == 3);
  assert(recorder.events[0].type == EventType::kWaterLevelUpdated);
  assert(recorder.events[1].type == EventType::kWaterLevelHigh);
  assert(recorder.events[2].type == EventType::kWaterLevelUpdated);
}

void testOutOfRangeFailurePreservesUntilTimeout() {
  resetState();
  EventBus bus;
  EventRecorder recorder = {};
  subscribeWaterLevelEvents(bus, recorder);
  ConfigManager configManager(bus);
  FakeTimeSource timeSource;
  FakeWaterLevelSensor sensor;
  timeSource.setUptimeMillis(1000);
  sensor.setValidLevel(45);
  WaterLevelService service(sensor, configManager, timeSource, bus);
  assert(service.runOnce());

  sensor.setOutOfRangeLevel(120);
  timeSource.setUptimeMillis(31001);
  assert(service.runOnce());

  const auto& waterLevel =
      reeflow::core::state::currentSystemState().waterLevel;
  assert(waterLevel.currentLevel == 45);
  assert(waterLevel.status == WaterLevelStatus::kSensorOffline);
  assert(waterLevel.lastUpdate == 1000);
}

void testWaterLevelEventsUseOnlyWaterLevelArea() {
  resetState();
  EventBus bus;
  EventRecorder recorder = {};
  subscribeWaterLevelEvents(bus, recorder);
  ConfigManager configManager(bus);
  FakeTimeSource timeSource;
  FakeWaterLevelSensor sensor;
  timeSource.setUptimeMillis(1000);
  sensor.setValidLevel(10);
  WaterLevelService service(sensor, configManager, timeSource, bus);
  assert(service.runOnce());

  for (uint8_t index = 0; index < recorder.count; ++index) {
    assert(recorder.events[index].stateArea ==
           reeflow::core::events::StateArea::kWaterLevel);
  }
}

}  // namespace

int main() {
  testValidReadingUpdatesWaterLevelStateAndPublishesUpdated();
  testLowAndHighStatusesPublishTransitionEvents();
  testIsolatedFailurePreservesLastValidStateAndLastUpdate();
  testFailureTimeoutBoundariesAfterLastValidReading();
  testNoValidReadingSinceBootCanBecomeOffline();
  testConfiguredLimitsAreWrittenToWaterLevelState();
  testCalibrationOffsetAffectsWaterLevelState();
  testUnrelatedStateBlocksAndAtoArePreserved();
  testRecoveryFromOfflinePublishesRecoveryOnce();
  testRepeatedStatusDoesNotDuplicateTransitionEvent();
  testOutOfRangeFailurePreservesUntilTimeout();
  testWaterLevelEventsUseOnlyWaterLevelArea();
  return 0;
}
