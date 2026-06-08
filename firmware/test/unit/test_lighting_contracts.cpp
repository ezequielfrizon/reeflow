#include <assert.h>
#include <string.h>

#include "core/events/event_bus.h"
#include "core/state/system_state.h"
#include "fakes/fake_lighting_pwm_controller.h"
#include "modules/lighting/lighting_config.h"
#include "modules/lighting/lighting_events.h"
#include "modules/lighting/lighting_profile.h"
#include "modules/lighting/lighting_pwm_controller.h"
#include "modules/lighting/lighting_types.h"

namespace {

using reeflow::core::events::EventType;
using reeflow::core::state::LightingMode;
using reeflow::modules::lighting::ACCLIMATION;
using reeflow::modules::lighting::AUTOMATIC;
using reeflow::modules::lighting::BLUE;
using reeflow::modules::lighting::LightingChannel;
using reeflow::modules::lighting::LightingCommandSource;
using reeflow::modules::lighting::LightingCommandTarget;
using reeflow::modules::lighting::LightingCurve;
using reeflow::modules::lighting::LightingCurvePoint;
using reeflow::modules::lighting::LightingCurveTargetType;
using reeflow::modules::lighting::LightingCurveValidationResult;
using reeflow::modules::lighting::LightingEventType;
using reeflow::modules::lighting::LightingProfile;
using reeflow::modules::lighting::LightingProfileValidationResult;
using reeflow::modules::lighting::LightingPwmResult;
using reeflow::modules::lighting::MANUAL;
using reeflow::modules::lighting::ROYAL_BLUE;
using reeflow::modules::lighting::UV;
using reeflow::modules::lighting::WHITE;
using reeflow::modules::lighting::isCanonicalLightingChannel;
using reeflow::modules::lighting::isCanonicalLightingMode;
using reeflow::modules::lighting::kLightingFunctionalChannelCount;
using reeflow::modules::lighting::kLightingMaxCurvePoints;
using reeflow::modules::lighting::kLightingMaxDuty;
using reeflow::modules::lighting::kLightingMaxIntensityPercent;
using reeflow::modules::lighting::kLightingMaxMinuteOfDay;
using reeflow::modules::lighting::kLightingMaxProfilesInMemory;
using reeflow::modules::lighting::lightingChannelMask;
using reeflow::modules::lighting::lightingChannelName;
using reeflow::modules::lighting::lightingChannelState;
using reeflow::modules::lighting::lightingChannelSystemStateFieldName;
using reeflow::modules::lighting::lightingCoreEventType;
using reeflow::modules::lighting::lightingCurveEmptyMeansDutyZero;
using reeflow::modules::lighting::lightingEventName;
using reeflow::modules::lighting::lightingModeName;
using reeflow::modules::lighting::makeDefaultLightingModuleConfig;
using reeflow::modules::lighting::makeLocalLightingDutyCommand;
using reeflow::modules::lighting::validateLightingCurve;
using reeflow::modules::lighting::validateLightingProfile;
using reeflow::modules::lighting::validLightingModuleConfig;
using reeflow::test::fakes::FakeLightingPwmController;

LightingCurve onePointCurve(uint16_t minuteOfDay, LightingCurveTargetType type,
                            uint16_t value) {
  LightingCurve curve = {};
  curve.pointCount = 1;
  curve.points[0] = {minuteOfDay, type, value};
  return curve;
}

LightingProfile validProfile() {
  LightingProfile profile = {};
  strcpy(profile.name, "default");
  profile.startMinuteOfDay = 480;
  profile.endMinuteOfDay = 1200;
  profile.sunriseEnabled = true;
  profile.sunsetEnabled = true;
  profile.acclimationEnabled = false;
  profile.maxGlobalIntensityPercent = 80;
  profile.acclimationDurationDays = 14;
  profile.white = {true, 200, 80,
                   onePointCurve(480, LightingCurveTargetType::kDuty, 120)};
  profile.blue = {true, 220, 90,
                  onePointCurve(480, LightingCurveTargetType::kDuty, 140)};
  profile.royalBlue = {
      true, 220, 90,
      onePointCurve(480, LightingCurveTargetType::kIntensityPercent, 60)};
  profile.uv = {true, 120, 40,
                onePointCurve(480, LightingCurveTargetType::kDuty, 40)};
  return profile;
}

void testCanonicalLightingChannels() {
  assert(kLightingFunctionalChannelCount == 4);
  assert(WHITE == LightingChannel::kWhite);
  assert(BLUE == LightingChannel::kBlue);
  assert(ROYAL_BLUE == LightingChannel::kRoyalBlue);
  assert(UV == LightingChannel::kUv);
  assert(isCanonicalLightingChannel(WHITE));
  assert(isCanonicalLightingChannel(BLUE));
  assert(isCanonicalLightingChannel(ROYAL_BLUE));
  assert(isCanonicalLightingChannel(UV));
  assert(!isCanonicalLightingChannel(LightingChannel::kUnknown));
  assert(strcmp(lightingChannelName(UV), "UV") == 0);
  assert(strcmp(lightingChannelName(LightingChannel::kUnknown),
                "UNKNOWN_LIGHTING_CHANNEL") == 0);
}

void testChannelsMapToSystemStateFields() {
  assert(strcmp(lightingChannelSystemStateFieldName(WHITE),
                "system.lighting.white") == 0);
  assert(strcmp(lightingChannelSystemStateFieldName(BLUE),
                "system.lighting.blue") == 0);
  assert(strcmp(lightingChannelSystemStateFieldName(ROYAL_BLUE),
                "system.lighting.royalBlue") == 0);
  assert(strcmp(lightingChannelSystemStateFieldName(UV),
                "system.lighting.uv") == 0);

  reeflow::core::state::LightingState state = {};
  state.white.currentPWM = 10;
  state.blue.currentPWM = 20;
  state.royalBlue.currentPWM = 30;
  state.uv.currentPWM = 40;
  assert(lightingChannelState(state, WHITE)->currentPWM == 10);
  assert(lightingChannelState(state, BLUE)->currentPWM == 20);
  assert(lightingChannelState(state, ROYAL_BLUE)->currentPWM == 30);
  assert(lightingChannelState(state, UV)->currentPWM == 40);
  assert(lightingChannelState(state, LightingChannel::kUnknown) == nullptr);
}

void testNoReservePwmOrMoonlightChannelIsExposed() {
  assert(kLightingFunctionalChannelCount == 4);
  assert(strcmp(lightingChannelName(WHITE), "WHITE") == 0);
  assert(strcmp(lightingChannelName(BLUE), "BLUE") == 0);
  assert(strcmp(lightingChannelName(ROYAL_BLUE), "ROYAL_BLUE") == 0);
  assert(strcmp(lightingChannelName(UV), "UV") == 0);
  assert(lightingChannelMask(WHITE) != 0);
  assert(lightingChannelMask(BLUE) != 0);
  assert(lightingChannelMask(ROYAL_BLUE) != 0);
  assert(lightingChannelMask(UV) != 0);
}

void testCanonicalLightingModes() {
  assert(MANUAL == LightingMode::kManual);
  assert(AUTOMATIC == LightingMode::kAutomatic);
  assert(ACCLIMATION == LightingMode::kAcclimation);
  assert(isCanonicalLightingMode(MANUAL));
  assert(isCanonicalLightingMode(AUTOMATIC));
  assert(isCanonicalLightingMode(ACCLIMATION));
  assert(strcmp(lightingModeName(MANUAL), "MANUAL") == 0);
  assert(strcmp(lightingModeName(AUTOMATIC), "AUTOMATIC") == 0);
  assert(strcmp(lightingModeName(ACCLIMATION), "ACCLIMATION") == 0);
}

void testLocalCommandContractDoesNotExposeRemoteSources() {
  const auto command =
      makeLocalLightingDutyCommand(MANUAL, WHITE, 128, 5000);

  assert(command.requestedMode == MANUAL);
  assert(command.target == LightingCommandTarget::kChannelDuty);
  assert(command.channel == WHITE);
  assert(command.requestedDuty == 128);
  assert(command.source == LightingCommandSource::kLocal);
  assert(command.requestedAtMillis == 5000);
}

void testLightingModuleConfigDeclaresFunctionalPwmAndFade() {
  const auto config = makeDefaultLightingModuleConfig();

  assert(validLightingModuleConfig(config));
  assert(config.evaluationIntervalMillis > 0);
  assert(config.pwm.frequencyHz > 0);
  assert(config.pwm.resolutionBits == 8);
  assert(config.pwm.minDuty == 0);
  assert(config.pwm.maxDuty == kLightingMaxDuty);
  assert(config.maxFadeStepPerEvaluation > 0);
}

void testLightingEventsExposeCanonicalNamesAndLocalBusTypes() {
  assert(strcmp(lightingEventName(LightingEventType::kLightingProfileChanged),
                "LIGHTING_PROFILE_CHANGED") == 0);
  assert(strcmp(lightingEventName(LightingEventType::kLightingStarted),
                "LIGHTING_STARTED") == 0);
  assert(strcmp(lightingEventName(LightingEventType::kLightingStopped),
                "LIGHTING_STOPPED") == 0);
  assert(lightingCoreEventType(LightingEventType::kLightingProfileChanged) ==
         EventType::kLightingProfileChanged);
  assert(lightingCoreEventType(LightingEventType::kLightingStarted) ==
         EventType::kLightingStarted);
  assert(lightingCoreEventType(LightingEventType::kLightingStopped) ==
         EventType::kLightingStopped);
}

void testCurveValidationAcceptsIncreasingPoints() {
  LightingCurve curve = {};
  curve.pointCount = 3;
  curve.points[0] = {0, LightingCurveTargetType::kDuty, 0};
  curve.points[1] = {720, LightingCurveTargetType::kDuty, 120};
  curve.points[2] = {1439, LightingCurveTargetType::kIntensityPercent, 10};

  assert(validateLightingCurve(curve) ==
         LightingCurveValidationResult::kValid);
}

void testCurveValidationRejectsInvalidMinuteAndOrder() {
  LightingCurve invalidMinute =
      onePointCurve(kLightingMaxMinuteOfDay + 1,
                    LightingCurveTargetType::kDuty, 10);
  assert(validateLightingCurve(invalidMinute) ==
         LightingCurveValidationResult::kMinuteOutOfRange);

  LightingCurve outOfOrder = {};
  outOfOrder.pointCount = 2;
  outOfOrder.points[0] = {600, LightingCurveTargetType::kDuty, 10};
  outOfOrder.points[1] = {600, LightingCurveTargetType::kDuty, 20};
  assert(validateLightingCurve(outOfOrder) ==
         LightingCurveValidationResult::kPointsNotIncreasing);
}

void testCurveValidationRejectsTargetAndPointLimits() {
  LightingCurve tooMany = {};
  tooMany.pointCount = kLightingMaxCurvePoints + 1;
  assert(validateLightingCurve(tooMany) ==
         LightingCurveValidationResult::kTooManyPoints);

  LightingCurve invalidDuty =
      onePointCurve(10, LightingCurveTargetType::kDuty, kLightingMaxDuty + 1);
  assert(validateLightingCurve(invalidDuty) ==
         LightingCurveValidationResult::kTargetOutOfRange);

  LightingCurve invalidIntensity =
      onePointCurve(10, LightingCurveTargetType::kIntensityPercent,
                    kLightingMaxIntensityPercent + 1);
  assert(validateLightingCurve(invalidIntensity) ==
         LightingCurveValidationResult::kTargetOutOfRange);
}

void testEmptyCurveRuleIsExplicitAndSafe() {
  LightingCurve empty = {};

  assert(validateLightingCurve(empty) ==
         LightingCurveValidationResult::kEmptyCurve);
  assert(lightingCurveEmptyMeansDutyZero());
}

void testProfileContractLimitsAreDeclaredAndValidated() {
  assert(kLightingMaxProfilesInMemory > 0);
  LightingProfile profile = validProfile();

  assert(validateLightingProfile(profile) ==
         LightingProfileValidationResult::kValid);

  profile.maxGlobalIntensityPercent = kLightingMaxIntensityPercent + 1;
  assert(validateLightingProfile(profile) ==
         LightingProfileValidationResult::kGlobalIntensityOutOfRange);

  profile = validProfile();
  profile.uv.maxDuty = kLightingMaxDuty + 1;
  assert(validateLightingProfile(profile) ==
         LightingProfileValidationResult::kChannelLimitOutOfRange);

  profile = validProfile();
  profile.royalBlue.curve.points[0].minuteOfDay =
      kLightingMaxMinuteOfDay + 1;
  assert(validateLightingProfile(profile) ==
         LightingProfileValidationResult::kCurveInvalid);
}

void testFakeLightingPwmControllerRecordsConfigureAndWrites() {
  FakeLightingPwmController controller;
  const auto config = makeDefaultLightingModuleConfig().pwm;

  assert(controller.configureChannel(WHITE, config) ==
         LightingPwmResult::kSuccess);
  assert(controller.configured(WHITE));

  assert(controller.writeDuty(WHITE, 127) == LightingPwmResult::kSuccess);
  assert(controller.duty(WHITE) == 127);
  assert(controller.recordedCallCount() == 2);
  assert(controller.recordedCall(0).type ==
         FakeLightingPwmController::RecordedCallType::kConfigureChannel);
  assert(controller.recordedCall(1).type ==
         FakeLightingPwmController::RecordedCallType::kWriteDuty);
  assert(controller.recordedCall(1).channel == WHITE);
  assert(controller.recordedCall(1).duty == 127);
}

void testFakeLightingPwmControllerAllOffZerosChannels() {
  FakeLightingPwmController controller;
  assert(controller.writeDuty(WHITE, 10) == LightingPwmResult::kSuccess);
  assert(controller.writeDuty(BLUE, 20) == LightingPwmResult::kSuccess);
  assert(controller.writeDuty(ROYAL_BLUE, 30) == LightingPwmResult::kSuccess);
  assert(controller.writeDuty(UV, 40) == LightingPwmResult::kSuccess);

  assert(controller.allOff() == LightingPwmResult::kSuccess);
  assert(controller.allOffCallCount() == 1);
  assert(controller.duty(WHITE) == 0);
  assert(controller.duty(BLUE) == 0);
  assert(controller.duty(ROYAL_BLUE) == 0);
  assert(controller.duty(UV) == 0);
}

void testFakeLightingPwmControllerSimulatesFailureAndUnknownChannel() {
  FakeLightingPwmController controller;
  controller.failChannel(UV);

  assert(controller.writeDuty(UV, 40) ==
         LightingPwmResult::kControllerFailure);
  assert(controller.duty(UV) == 0);

  assert(controller.writeDuty(LightingChannel::kUnknown, 40) ==
         LightingPwmResult::kUnknownChannel);
  assert(controller.writeDuty(WHITE, kLightingMaxDuty + 1) ==
         LightingPwmResult::kInvalidDuty);
}

}  // namespace

int main() {
  testCanonicalLightingChannels();
  testChannelsMapToSystemStateFields();
  testNoReservePwmOrMoonlightChannelIsExposed();
  testCanonicalLightingModes();
  testLocalCommandContractDoesNotExposeRemoteSources();
  testLightingModuleConfigDeclaresFunctionalPwmAndFade();
  testLightingEventsExposeCanonicalNamesAndLocalBusTypes();
  testCurveValidationAcceptsIncreasingPoints();
  testCurveValidationRejectsInvalidMinuteAndOrder();
  testCurveValidationRejectsTargetAndPointLimits();
  testEmptyCurveRuleIsExplicitAndSafe();
  testProfileContractLimitsAreDeclaredAndValidated();
  testFakeLightingPwmControllerRecordsConfigureAndWrites();
  testFakeLightingPwmControllerAllOffZerosChannels();
  testFakeLightingPwmControllerSimulatesFailureAndUnknownChannel();
  return 0;
}
