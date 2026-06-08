#include <assert.h>
#include <string.h>

#include "app/core_app.h"
#include "config/config_manager.h"
#include "core/events/event_bus.h"
#include "core/platform/core_platform.h"
#include "core/state/system_state.h"
#include "fakes/fake_lighting_pwm_controller.h"
#include "fakes/fake_log_sink.h"
#include "fakes/fake_mode_store.h"
#include "fakes/fake_relay_controller.h"
#include "fakes/fake_temperature_sensor.h"
#include "fakes/fake_time_source.h"
#include "fakes/fake_watchdog_backend.h"
#include "fakes/fake_water_level_sensor.h"
#include "modules/lighting/lighting_profile.h"
#include "modules/lighting/lighting_service.h"
#include "modules/lighting/lighting_types.h"

namespace {

using reeflow::app::CoreApp;
using reeflow::config::ConfigManager;
using reeflow::core::events::Event;
using reeflow::core::events::EventBus;
using reeflow::core::events::EventType;
using reeflow::core::platform::CorePlatform;
using reeflow::modules::lighting::AUTOMATIC;
using reeflow::modules::lighting::BLUE;
using reeflow::modules::lighting::LightingServiceResult;
using reeflow::modules::lighting::MANUAL;
using reeflow::modules::lighting::WHITE;
using reeflow::modules::lighting::makeDefaultLightingProfile;
using reeflow::test::fakes::FakeLightingPwmController;
using reeflow::test::fakes::FakeLogSink;
using reeflow::test::fakes::FakeModeStore;
using reeflow::test::fakes::FakeRelayController;
using reeflow::test::fakes::FakeTemperatureSensor;
using reeflow::test::fakes::FakeTimeSource;
using reeflow::test::fakes::FakeWatchdogBackend;
using reeflow::test::fakes::FakeWaterLevelSensor;

struct EventRecorder {
  Event events[16];
  uint8_t count;
};

bool recordEvent(const Event& event, void* context) {
  EventRecorder* recorder = static_cast<EventRecorder*>(context);
  recorder->events[recorder->count++] = event;
  return true;
}

struct Fixture {
  FakeTimeSource timeSource;
  FakeLogSink logSink;
  FakeWatchdogBackend watchdogBackend;
  CorePlatform platform;
  EventBus eventBus;
  ConfigManager configManager;
  FakeTemperatureSensor temperatureSensor;
  FakeWaterLevelSensor waterLevelSensor;
  FakeRelayController relayController;
  FakeLightingPwmController lightingController;
  FakeModeStore modeStore;
  CoreApp app;
  EventRecorder lightingStartedEvents;
  EventRecorder schedulerFailedEvents;

  Fixture()
      : platform(timeSource, logSink, watchdogBackend),
        configManager(eventBus),
        app(platform, eventBus, configManager, temperatureSensor,
            waterLevelSensor, relayController, lightingController, modeStore),
        lightingStartedEvents({}),
        schedulerFailedEvents({}) {
    eventBus.subscribe(EventType::kLightingStarted, recordEvent,
                       &lightingStartedEvents);
    eventBus.subscribe(EventType::kSchedulerTaskFailed, recordEvent,
                       &schedulerFailedEvents);
  }
};

void testSetupConfiguresLightingSafeState() {
  Fixture fixture;

  assert(fixture.app.setup());

  assert(fixture.lightingController.configured(WHITE));
  assert(fixture.lightingController.configured(BLUE));
  assert(fixture.lightingController.configured(
      reeflow::modules::lighting::ROYAL_BLUE));
  assert(fixture.lightingController.configured(
      reeflow::modules::lighting::UV));
  assert(fixture.lightingController.allOffCallCount() == 1);
  assert(fixture.lightingController.duty(WHITE) == 0);
  assert(fixture.lightingController.duty(BLUE) == 0);
  assert(reeflow::core::state::currentSystemState().lighting.white.currentPWM ==
         0);
  assert(reeflow::core::state::currentSystemState().lighting.blue.currentPWM ==
         0);
  assert(reeflow::core::state::currentSystemState()
             .lighting.royalBlue.currentPWM == 0);
  assert(reeflow::core::state::currentSystemState().lighting.uv.currentPWM ==
         0);
  assert(fixture.lightingStartedEvents.count == 0);
}

void testLightingTaskRespectsSchedulerIntervalAndFade() {
  Fixture fixture;
  assert(fixture.app.setup());
  const size_t setupCallCount = fixture.lightingController.recordedCallCount();
  assert(fixture.app.setLightingManualDuty(WHITE, 100) ==
         LightingServiceResult::kSuccess);

  fixture.timeSource.advanceMillis(999);
  auto result = fixture.app.loopOnce();
  assert(result.executedCount == 0);
  assert(fixture.lightingController.recordedCallCount() == setupCallCount);

  fixture.timeSource.advanceMillis(1);
  result = fixture.app.loopOnce();
  assert(result.executedCount == 4);
  assert(result.failedCount == 0);
  assert(fixture.lightingController.duty(WHITE) == 5);
  assert(reeflow::core::state::currentSystemState().lighting.white.currentPWM ==
         5);

  fixture.timeSource.advanceMillis(1000);
  result = fixture.app.loopOnce();
  assert(result.failedCount == 0);
  assert(fixture.lightingController.duty(WHITE) == 10);
  assert(reeflow::core::state::currentSystemState().lighting.white.currentPWM ==
         10);
}

void testLightingLocalModeApisAreAppliedByScheduler() {
  Fixture fixture;
  assert(fixture.app.setup());

  assert(fixture.app.requestLightingAutomaticMode() ==
         LightingServiceResult::kSuccess);
  fixture.timeSource.advanceMillis(1000);
  assert(fixture.app.loopOnce().failedCount == 0);
  assert(reeflow::core::state::currentSystemState().lighting.mode ==
         AUTOMATIC);

  assert(fixture.app.requestLightingManualMode() ==
         LightingServiceResult::kSuccess);
  fixture.timeSource.advanceMillis(1000);
  assert(fixture.app.loopOnce().failedCount == 0);
  assert(reeflow::core::state::currentSystemState().lighting.mode == MANUAL);

  assert(fixture.app.requestLightingAcclimationMode() ==
         LightingServiceResult::kSuccess);
  fixture.timeSource.advanceMillis(1000);
  assert(fixture.app.loopOnce().failedCount == 0);
  assert(reeflow::core::state::currentSystemState().lighting.mode ==
         reeflow::modules::lighting::ACCLIMATION);
}

void testLightingProfileAndManualTargetApis() {
  Fixture fixture;
  assert(fixture.app.setup());
  auto profile = makeDefaultLightingProfile();
  strcpy(profile.name, "lagoon");

  assert(fixture.app.setLightingProfile(profile) ==
         LightingServiceResult::kSuccess);
  assert(fixture.app.requestLightingAutomaticMode() ==
         LightingServiceResult::kSuccess);
  fixture.timeSource.advanceMillis(1000);
  assert(fixture.app.loopOnce().failedCount == 0);
  assert(strcmp(reeflow::core::state::currentSystemState()
                    .lighting.currentProfile,
                "lagoon") == 0);

  assert(fixture.app.setLightingManualDuty(BLUE, 40) ==
         LightingServiceResult::kSuccess);
  fixture.timeSource.advanceMillis(1000);
  assert(fixture.app.loopOnce().failedCount == 0);
  assert(reeflow::core::state::currentSystemState().lighting.blue.currentPWM ==
         5);
}

void testLightingAcclimationStartsWithReducedIntensity() {
  Fixture fixture;
  fixture.timeSource.setUptimeMillis(720UL * 60000UL);
  assert(fixture.app.setup());

  assert(fixture.app.requestLightingAcclimationMode() ==
         LightingServiceResult::kSuccess);
  for (uint8_t cycle = 0; cycle < 4; ++cycle) {
    fixture.timeSource.advanceMillis(1000);
    assert(fixture.app.loopOnce().failedCount == 0);
  }

  const auto& lighting = reeflow::core::state::currentSystemState().lighting;
  assert(lighting.mode == reeflow::modules::lighting::ACCLIMATION);
  assert(lighting.white.currentPWM > 0);
  assert(lighting.white.currentPWM <= 18);
}

void testLightingTaskFailureIsReportedByScheduler() {
  Fixture fixture;
  assert(fixture.app.setup());
  fixture.lightingController.failChannel(WHITE);
  assert(fixture.app.setLightingManualDuty(WHITE, 100) ==
         LightingServiceResult::kSuccess);

  fixture.timeSource.advanceMillis(1000);
  const auto result = fixture.app.loopOnce();

  assert(result.failedCount == 1);
  assert(fixture.schedulerFailedEvents.count == 1);
  assert(reeflow::core::state::currentSystemState().lighting.white.currentPWM ==
         0);
}

}  // namespace

int main() {
  testSetupConfiguresLightingSafeState();
  testLightingTaskRespectsSchedulerIntervalAndFade();
  testLightingLocalModeApisAreAppliedByScheduler();
  testLightingProfileAndManualTargetApis();
  testLightingAcclimationStartsWithReducedIntensity();
  testLightingTaskFailureIsReportedByScheduler();
  return 0;
}
