#include <assert.h>
#include <string.h>

#include "config/config_manager.h"
#include "core/events/event_bus.h"
#include "core/state/system_state.h"
#include "fakes/fake_lighting_pwm_controller.h"
#include "modules/lighting/lighting_config.h"
#include "modules/lighting/lighting_events.h"
#include "modules/lighting/lighting_policy.h"
#include "modules/lighting/lighting_profile.h"
#include "modules/lighting/lighting_service.h"
#include "modules/lighting/lighting_types.h"

namespace {

using reeflow::config::ConfigManager;
using reeflow::config::LightingConfig;
using reeflow::core::events::Event;
using reeflow::core::events::EventBus;
using reeflow::core::events::EventType;
using reeflow::core::state::LightingState;
using reeflow::core::state::SystemState;
using reeflow::modules::lighting::ACCLIMATION;
using reeflow::modules::lighting::AUTOMATIC;
using reeflow::modules::lighting::BLUE;
using reeflow::modules::lighting::LightingCommand;
using reeflow::modules::lighting::LightingCommandReason;
using reeflow::modules::lighting::LightingCommandSource;
using reeflow::modules::lighting::LightingCommandTarget;
using reeflow::modules::lighting::LightingCurve;
using reeflow::modules::lighting::LightingCurveTargetType;
using reeflow::modules::lighting::LightingEvent;
using reeflow::modules::lighting::LightingProfile;
using reeflow::modules::lighting::LightingPwmResult;
using reeflow::modules::lighting::LightingService;
using reeflow::modules::lighting::LightingServiceResult;
using reeflow::modules::lighting::MANUAL;
using reeflow::modules::lighting::ROYAL_BLUE;
using reeflow::modules::lighting::UV;
using reeflow::modules::lighting::WHITE;
using reeflow::modules::lighting::makeDefaultLightingModuleConfig;
using reeflow::modules::lighting::makeLocalLightingDutyCommand;
using reeflow::test::fakes::FakeLightingPwmController;

struct EventRecorder {
  Event events[16];
  LightingEvent lightingEvents[16];
  uint8_t count;
};

bool recordEvent(const Event& event, void* context) {
  EventRecorder* recorder = static_cast<EventRecorder*>(context);
  const uint8_t index = recorder->count;
  recorder->events[index] = event;
  if (event.payload != nullptr) {
    recorder->lightingEvents[index] =
        *static_cast<const LightingEvent*>(event.payload);
  }
  recorder->count += 1;
  return true;
}

LightingCurve curve(uint16_t firstMinute, uint16_t firstDuty,
                    uint16_t secondMinute, uint16_t secondDuty,
                    uint16_t thirdMinute, uint16_t thirdDuty) {
  LightingCurve value = {};
  value.pointCount = 3;
  value.points[0] = {firstMinute, LightingCurveTargetType::kDuty, firstDuty};
  value.points[1] = {secondMinute, LightingCurveTargetType::kDuty, secondDuty};
  value.points[2] = {thirdMinute, LightingCurveTargetType::kDuty, thirdDuty};
  return value;
}

LightingProfile profile(const char* name = "reef") {
  LightingProfile value = {};
  strcpy(value.name, name);
  value.startMinuteOfDay = 480;
  value.endMinuteOfDay = 1200;
  value.sunriseEnabled = true;
  value.sunsetEnabled = true;
  value.acclimationEnabled = true;
  value.maxGlobalIntensityPercent = 100;
  value.acclimationDurationDays = 10;
  value.white = {true, 200, 100, curve(0, 10, 720, 200, 1439, 10)};
  value.blue = {true, 180, 100, curve(0, 8, 720, 180, 1439, 8)};
  value.royalBlue = {true, 160, 100, curve(0, 6, 720, 160, 1439, 6)};
  value.uv = {true, 120, 100, curve(0, 4, 720, 120, 1439, 4)};
  return value;
}

LightingConfig config(reeflow::core::state::LightingMode mode) {
  LightingConfig value = {};
  value.mode = mode;
  value.sunriseEnabled = true;
  value.sunsetEnabled = true;
  value.acclimationEnabled = mode == ACCLIMATION;
  value.startMinuteOfDay = 480;
  value.endMinuteOfDay = 1200;
  value.maxIntensityPercent = 100;
  value.acclimationDays = 10;
  value.white = {true, 200};
  value.blue = {true, 180};
  value.royalBlue = {true, 160};
  value.uv = {true, 120};
  return value;
}

LightingState lightingState(uint16_t white, uint16_t blue,
                            uint16_t royalBlue, uint16_t uv) {
  LightingState value = {};
  value.mode = MANUAL;
  value.white = {white, 200, true};
  value.blue = {blue, 180, true};
  value.royalBlue = {royalBlue, 160, true};
  value.uv = {uv, 120, true};
  return value;
}

reeflow::modules::lighting::LightingModuleConfig serviceModuleConfig(
    uint16_t fadeStep = 255) {
  auto value = makeDefaultLightingModuleConfig();
  value.maxFadeStepPerEvaluation = fadeStep;
  return value;
}

struct Fixture {
  EventBus eventBus;
  ConfigManager configManager;
  FakeLightingPwmController controller;
  LightingProfile activeProfile;
  LightingService service;
  EventRecorder profileChangedEvents;
  EventRecorder startedEvents;
  EventRecorder stoppedEvents;

  Fixture()
      : configManager(eventBus),
        activeProfile(profile()),
        service(controller, configManager, eventBus, activeProfile,
                serviceModuleConfig()),
        profileChangedEvents({}),
        startedEvents({}),
        stoppedEvents({}) {
    reeflow::core::state::setSystemStateEventBus(eventBus);
    reeflow::core::state::resetSystemState();
    configManager.loadDefaults();
    assert(configManager.updateLighting(config(MANUAL)));
    eventBus.subscribe(EventType::kLightingProfileChanged, recordEvent,
                       &profileChangedEvents);
    eventBus.subscribe(EventType::kLightingStarted, recordEvent,
                       &startedEvents);
    eventBus.subscribe(EventType::kLightingStopped, recordEvent,
                       &stoppedEvents);
  }

  void updateConfig(reeflow::core::state::LightingMode mode) {
    assert(configManager.updateLighting(config(mode)));
  }
};

void setLighting(const LightingState& lighting) {
  reeflow::core::state::updateLightingState(lighting);
}

void assertUnrelatedBlocksMatch(const SystemState& before) {
  const SystemState& after = reeflow::core::state::currentSystemState();
  assert(after.temperature.currentTemperature ==
         before.temperature.currentTemperature);
  assert(after.temperature.status == before.temperature.status);
  assert(after.waterLevel.currentLevel == before.waterLevel.currentLevel);
  assert(after.waterLevel.status == before.waterLevel.status);
  assert(after.relays.recalque.enabled == before.relays.recalque.enabled);
  assert(after.modes.currentMode == before.modes.currentMode);
  assert(after.ato.status == before.ato.status);
  assert(after.network.wifiConnected == before.network.wifiConnected);
  assert(after.alerts.activeAlertCount == before.alerts.activeAlertCount);
  assert(after.systemHealth.uptime == before.systemHealth.uptime);
}

void testManualEvaluationWritesPwmAndUpdatesLightingState() {
  Fixture fixture;
  const auto command = makeLocalLightingDutyCommand(MANUAL, WHITE, 100, 10);

  const auto result = fixture.service.evaluateOnce(720, &command);
  const LightingState& lighting =
      reeflow::core::state::currentSystemState().lighting;

  assert(result.result == LightingServiceResult::kSuccess);
  assert(result.stateUpdated);
  assert(fixture.controller.recordedCallCount() == 1);
  assert(fixture.controller.recordedCall(0).channel == WHITE);
  assert(fixture.controller.recordedCall(0).duty == 100);
  assert(lighting.white.currentPWM == 100);
  assert(lighting.white.maxPWM == 200);
  assert(lighting.white.enabled);
  assert(lighting.blue.currentPWM == 0);
  assert(fixture.startedEvents.count == 1);
}

void testOnlyTargetChannelCurrentPwmChangesForManualCommand() {
  Fixture fixture;
  setLighting(lightingState(10, 20, 30, 40));
  const auto command = makeLocalLightingDutyCommand(MANUAL, BLUE, 60, 10);

  const auto result = fixture.service.evaluateOnce(720, &command);
  const LightingState& lighting =
      reeflow::core::state::currentSystemState().lighting;

  assert(result.result == LightingServiceResult::kSuccess);
  assert(lighting.white.currentPWM == 10);
  assert(lighting.blue.currentPWM == 60);
  assert(lighting.royalBlue.currentPWM == 30);
  assert(lighting.uv.currentPWM == 40);
}

void testAutomaticEvaluationUsesProfileAndUpdatesProfileState() {
  Fixture fixture;
  fixture.updateConfig(AUTOMATIC);

  const auto result = fixture.service.evaluateOnce(720);
  const LightingState& lighting =
      reeflow::core::state::currentSystemState().lighting;

  assert(result.result == LightingServiceResult::kSuccess);
  assert(lighting.mode == AUTOMATIC);
  assert(strcmp(lighting.currentProfile, "reef") == 0);
  assert(lighting.sunriseEnabled);
  assert(lighting.sunsetEnabled);
  assert(lighting.white.currentPWM == 200);
  assert(fixture.profileChangedEvents.count == 1);
  assert(fixture.profileChangedEvents.lightingEvents[0].mode == AUTOMATIC);
  assert(strcmp(fixture.profileChangedEvents.lightingEvents[0].profile,
                "reef") == 0);
  assert(fixture.profileChangedEvents.lightingEvents[0].affectedChannels ==
         reeflow::modules::lighting::kLightingChannelMaskAll);
  assert(fixture.startedEvents.count == 1);
}

void testAcclimationEvaluationUsesInjectedProgress() {
  Fixture fixture;
  fixture.updateConfig(ACCLIMATION);

  const auto result = fixture.service.evaluateOnce(720, nullptr, 1);
  const LightingState& lighting =
      reeflow::core::state::currentSystemState().lighting;

  assert(result.result == LightingServiceResult::kSuccess);
  assert(lighting.mode == ACCLIMATION);
  assert(lighting.acclimationEnabled);
  assert(lighting.white.currentPWM == 20);
}

void testControllerFailurePreservesSystemStateAndReportsFailure() {
  Fixture fixture;
  setLighting(lightingState(0, 0, 0, 0));
  fixture.controller.failChannel(WHITE);
  const auto command = makeLocalLightingDutyCommand(MANUAL, WHITE, 100, 10);

  const auto result = fixture.service.evaluateOnce(720, &command);
  const LightingState& lighting =
      reeflow::core::state::currentSystemState().lighting;

  assert(result.result == LightingServiceResult::kControllerFailure);
  assert(result.pwmResult == LightingPwmResult::kControllerFailure);
  assert(!result.stateUpdated);
  assert(lighting.white.currentPWM == 0);
  assert(fixture.startedEvents.count == 0);
}

void testInvalidConfigOnlyConvergesCurrentPwmAndPreservesMetadata() {
  Fixture fixture;
  LightingState current = lightingState(10, 0, 0, 0);
  current.mode = AUTOMATIC;
  current.sunriseEnabled = true;
  current.sunsetEnabled = true;
  current.acclimationEnabled = false;
  strcpy(current.currentProfile, "stable");
  current.white.maxPWM = 123;
  current.white.enabled = true;
  setLighting(current);

  LightingConfig invalid = config(AUTOMATIC);
  invalid.startMinuteOfDay = 1200;
  invalid.endMinuteOfDay = 480;
  assert(fixture.configManager.updateLighting(invalid));

  const auto result = fixture.service.evaluateOnce(720, nullptr, 0);
  const LightingState& after =
      reeflow::core::state::currentSystemState().lighting;

  assert(result.result == LightingServiceResult::kInvalidConfig);
  assert(result.stateUpdated);
  assert(after.white.currentPWM == 0);
  assert(after.mode == AUTOMATIC);
  assert(after.sunriseEnabled);
  assert(after.sunsetEnabled);
  assert(!after.acclimationEnabled);
  assert(strcmp(after.currentProfile, "stable") == 0);
  assert(after.white.maxPWM == 123);
  assert(after.white.enabled);
}

void testProfileChangedIsEmittedForProfileModeAndCurveChangesOnlyOnce() {
  Fixture fixture;
  fixture.updateConfig(AUTOMATIC);

  assert(fixture.service.evaluateOnce(720).result ==
         LightingServiceResult::kSuccess);
  assert(fixture.profileChangedEvents.count == 1);

  assert(fixture.service.evaluateOnce(720).result ==
         LightingServiceResult::kSuccess);
  assert(fixture.profileChangedEvents.count == 1);

  LightingProfile nextProfile = profile("lagoon");
  fixture.service.setActiveProfile(nextProfile);
  assert(fixture.service.evaluateOnce(720).result ==
         LightingServiceResult::kSuccess);
  assert(fixture.profileChangedEvents.count == 2);

  nextProfile.white.curve.points[1].value = 180;
  fixture.service.setActiveProfile(nextProfile);
  assert(fixture.service.evaluateOnce(720).result ==
         LightingServiceResult::kSuccess);
  assert(fixture.profileChangedEvents.count == 3);
}

void testIdempotentManualCommandDoesNotEmitDuplicateEvents() {
  Fixture fixture;
  setLighting(lightingState(50, 0, 0, 0));
  const auto command = makeLocalLightingDutyCommand(MANUAL, WHITE, 50, 10);

  const auto result = fixture.service.evaluateOnce(720, &command);

  assert(result.result == LightingServiceResult::kSuccess);
  assert(fixture.controller.recordedCallCount() == 0);
  assert(fixture.profileChangedEvents.count == 0);
  assert(fixture.startedEvents.count == 0);
  assert(fixture.stoppedEvents.count == 0);
}

void testLightingStartedIsNotDuplicatedDuringFade() {
  Fixture fixture;
  LightingService fadingService(fixture.controller, fixture.configManager,
                                fixture.eventBus, fixture.activeProfile,
                                serviceModuleConfig(5));
  setLighting(lightingState(5, 0, 0, 0));
  const auto command = makeLocalLightingDutyCommand(MANUAL, WHITE, 100, 10);

  const auto result = fadingService.evaluateOnce(720, &command);

  assert(result.result == LightingServiceResult::kSuccess);
  assert(reeflow::core::state::currentSystemState().lighting.white.currentPWM ==
         10);
  assert(fixture.startedEvents.count == 0);
}

void testLightingStoppedOnlyWhenAllChannelsReachZero() {
  Fixture fixture;
  setLighting(lightingState(10, 20, 0, 0));
  const auto blueOff = makeLocalLightingDutyCommand(MANUAL, BLUE, 0, 10);

  assert(fixture.service.evaluateOnce(720, &blueOff).result ==
         LightingServiceResult::kSuccess);
  assert(fixture.stoppedEvents.count == 0);

  const LightingCommand allOff = {MANUAL,
                                  LightingCommandTarget::kAllChannelsOff,
                                  reeflow::modules::lighting::LightingChannel::kUnknown,
                                  0,
                                  0,
                                  {},
                                  LightingCommandSource::kLocal,
                                  20,
                                  LightingCommandReason::kSafetyOff};
  assert(fixture.service.evaluateOnce(720, &allOff).result ==
         LightingServiceResult::kSuccess);
  assert(fixture.stoppedEvents.count == 1);
  assert(fixture.stoppedEvents.lightingEvents[0].affectedChannels ==
         reeflow::modules::lighting::kLightingChannelMaskWhite);
  assert(fixture.stoppedEvents.lightingEvents[0].occurredAtMillis == 20);
  assert(reeflow::core::state::currentSystemState().lighting.white.currentPWM ==
         0);
}

void testInvalidCommandDoesNotUpdateState() {
  Fixture fixture;
  setLighting(lightingState(10, 0, 0, 0));
  const LightingState before =
      reeflow::core::state::currentSystemState().lighting;
  const LightingCommand command = {MANUAL,
                                  LightingCommandTarget::kChannelDuty,
                                  reeflow::modules::lighting::LightingChannel::kUnknown,
                                  20,
                                  0,
                                  {},
                                  LightingCommandSource::kLocal,
                                  10,
                                  LightingCommandReason::kUserRequest};

  const auto result = fixture.service.evaluateOnce(720, &command);
  const LightingState& after =
      reeflow::core::state::currentSystemState().lighting;

  assert(result.result == LightingServiceResult::kInvalidCommand);
  assert(!result.stateUpdated);
  assert(after.white.currentPWM == before.white.currentPWM);
  assert(fixture.controller.recordedCallCount() == 0);
}

void testUnrelatedStateBlocksArePreserved() {
  Fixture fixture;
  const SystemState before = reeflow::core::state::currentSystemState();
  const auto command = makeLocalLightingDutyCommand(MANUAL, WHITE, 100, 10);

  assert(fixture.service.evaluateOnce(720, &command).result ==
         LightingServiceResult::kSuccess);

  assertUnrelatedBlocksMatch(before);
}

void testLightingEventsStayLocalToLightingEventTypes() {
  Fixture fixture;
  const auto command = makeLocalLightingDutyCommand(MANUAL, WHITE, 100, 10);

  assert(fixture.service.evaluateOnce(720, &command).result ==
         LightingServiceResult::kSuccess);

  assert(fixture.startedEvents.count == 1);
  assert(fixture.startedEvents.events[0].type == EventType::kLightingStarted);
  assert(fixture.startedEvents.events[0].stateArea ==
         reeflow::core::events::StateArea::kLighting);
  assert(fixture.startedEvents.lightingEvents[0].mode == MANUAL);
  assert(fixture.startedEvents.lightingEvents[0].affectedChannels ==
         reeflow::modules::lighting::kLightingChannelMaskWhite);
  assert(fixture.startedEvents.lightingEvents[0].occurredAtMillis == 10);
  assert(fixture.profileChangedEvents.count == 0);
}

}  // namespace

int main() {
  testManualEvaluationWritesPwmAndUpdatesLightingState();
  testOnlyTargetChannelCurrentPwmChangesForManualCommand();
  testAutomaticEvaluationUsesProfileAndUpdatesProfileState();
  testAcclimationEvaluationUsesInjectedProgress();
  testControllerFailurePreservesSystemStateAndReportsFailure();
  testInvalidConfigOnlyConvergesCurrentPwmAndPreservesMetadata();
  testProfileChangedIsEmittedForProfileModeAndCurveChangesOnlyOnce();
  testIdempotentManualCommandDoesNotEmitDuplicateEvents();
  testLightingStartedIsNotDuplicatedDuringFade();
  testLightingStoppedOnlyWhenAllChannelsReachZero();
  testInvalidCommandDoesNotUpdateState();
  testUnrelatedStateBlocksArePreserved();
  testLightingEventsStayLocalToLightingEventTypes();
  return 0;
}
