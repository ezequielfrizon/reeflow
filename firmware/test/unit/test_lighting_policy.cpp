#include <assert.h>
#include <string.h>

#include "config/config_manager.h"
#include "core/state/system_state.h"
#include "modules/lighting/lighting_config.h"
#include "modules/lighting/lighting_policy.h"
#include "modules/lighting/lighting_profile.h"
#include "modules/lighting/lighting_types.h"

namespace {

using reeflow::config::LightingConfig;
using reeflow::core::state::LightingState;
using reeflow::modules::lighting::ACCLIMATION;
using reeflow::modules::lighting::AUTOMATIC;
using reeflow::modules::lighting::BLUE;
using reeflow::modules::lighting::LightingChannel;
using reeflow::modules::lighting::LightingCommand;
using reeflow::modules::lighting::LightingCommandReason;
using reeflow::modules::lighting::LightingCommandSource;
using reeflow::modules::lighting::LightingCommandTarget;
using reeflow::modules::lighting::LightingCurve;
using reeflow::modules::lighting::LightingCurveTargetType;
using reeflow::modules::lighting::LightingModuleConfig;
using reeflow::modules::lighting::LightingPolicyInput;
using reeflow::modules::lighting::LightingPolicyReason;
using reeflow::modules::lighting::LightingProfile;
using reeflow::modules::lighting::MANUAL;
using reeflow::modules::lighting::ROYAL_BLUE;
using reeflow::modules::lighting::UV;
using reeflow::modules::lighting::WHITE;
using reeflow::modules::lighting::evaluateLightingPolicy;
using reeflow::modules::lighting::kLightingFunctionalChannelCount;
using reeflow::modules::lighting::kLightingMaxDuty;
using reeflow::modules::lighting::makeDefaultLightingModuleConfig;
using reeflow::modules::lighting::makeLocalLightingDutyCommand;
using reeflow::modules::lighting::makeLocalLightingIntensityCommand;

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

LightingProfile profile() {
  LightingProfile value = {};
  strcpy(value.name, "reef");
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

LightingConfig memoryConfig(reeflow::core::state::LightingMode mode) {
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

LightingState state(uint16_t white, uint16_t blue, uint16_t royalBlue,
                    uint16_t uv) {
  LightingState value = {};
  value.mode = MANUAL;
  value.white = {white, 200, true};
  value.blue = {blue, 180, true};
  value.royalBlue = {royalBlue, 160, true};
  value.uv = {uv, 120, true};
  return value;
}

LightingModuleConfig moduleConfig(uint16_t fadeStep = 255) {
  LightingModuleConfig config = makeDefaultLightingModuleConfig();
  config.maxFadeStepPerEvaluation = fadeStep;
  return config;
}

LightingPolicyInput input(const LightingState& currentState,
                          const LightingConfig& config,
                          const LightingProfile& activeProfile,
                          uint16_t minute,
                          const LightingCommand* command = nullptr,
                          uint16_t acclimationElapsedDays = 10,
                          LightingModuleConfig module = moduleConfig()) {
  return {currentState, config, activeProfile, minute,
          acclimationElapsedDays, command, module};
}

void testManualModeSetsValidWhiteTarget() {
  const LightingState current = state(0, 0, 0, 0);
  const LightingConfig config = memoryConfig(MANUAL);
  const LightingProfile activeProfile = profile();
  const auto command = makeLocalLightingDutyCommand(MANUAL, WHITE, 100, 10);

  const auto decision =
      evaluateLightingPolicy(input(current, config, activeProfile, 720,
                                   &command));

  assert(decision.accepted);
  assert(decision.channels[0].channel == WHITE);
  assert(decision.channels[0].targetDuty == 100);
  assert(decision.channels[0].nextDuty == 100);
  assert(decision.channels[0].shouldWrite);
  assert(decision.channels[0].reason == LightingPolicyReason::kManualCommand);
}

void testManualModeTurnsBlueOff() {
  const LightingState current = state(0, 80, 0, 0);
  const LightingConfig config = memoryConfig(MANUAL);
  const LightingProfile activeProfile = profile();
  const auto command = makeLocalLightingDutyCommand(MANUAL, BLUE, 0, 10);

  const auto decision =
      evaluateLightingPolicy(input(current, config, activeProfile, 720,
                                   &command));

  assert(decision.channels[1].targetDuty == 0);
  assert(decision.channels[1].nextDuty == 0);
  assert(decision.channels[1].shouldWrite);
}

void testManualModeLimitsTargetByChannelAndGlobalLimits() {
  const LightingState current = state(0, 0, 0, 0);
  LightingConfig config = memoryConfig(MANUAL);
  config.maxIntensityPercent = 50;
  LightingProfile activeProfile = profile();
  activeProfile.white.maxDuty = 200;
  const auto command = makeLocalLightingDutyCommand(MANUAL, WHITE, 250, 10);

  const auto decision =
      evaluateLightingPolicy(input(current, config, activeProfile, 720,
                                   &command));

  assert(decision.accepted);
  assert(decision.channels[0].targetDuty == 100);
}

void testManualModeLimitsTargetByChannelIntensity() {
  const LightingState current = state(0, 0, 0, 0);
  const LightingConfig config = memoryConfig(MANUAL);
  LightingProfile activeProfile = profile();
  activeProfile.white.maxIntensityPercent = 40;
  const auto command = makeLocalLightingDutyCommand(MANUAL, WHITE, 200, 10);

  const auto decision =
      evaluateLightingPolicy(input(current, config, activeProfile, 720,
                                   &command));

  assert(decision.accepted);
  assert(decision.channels[0].targetDuty == 80);
}

void testManualIdempotentCommandDoesNotRequestWrite() {
  const LightingState current = state(75, 0, 0, 0);
  const LightingConfig config = memoryConfig(MANUAL);
  const LightingProfile activeProfile = profile();
  const auto command = makeLocalLightingDutyCommand(MANUAL, WHITE, 75, 10);

  const auto decision =
      evaluateLightingPolicy(input(current, config, activeProfile, 720,
                                   &command));

  assert(decision.channels[0].targetDuty == 75);
  assert(!decision.channels[0].shouldWrite);
  assert(decision.channels[0].reason == LightingPolicyReason::kIdempotent);
}

void testInvalidManualCommandPreservesCurrentTargets() {
  const LightingState current = state(55, 44, 33, 22);
  const LightingConfig config = memoryConfig(MANUAL);
  const LightingProfile activeProfile = profile();
  const LightingCommand command = {MANUAL,
                                  LightingCommandTarget::kChannelDuty,
                                  LightingChannel::kUnknown,
                                  20,
                                  0,
                                  {},
                                  LightingCommandSource::kLocal,
                                  10,
                                  LightingCommandReason::kUserRequest};

  const auto decision =
      evaluateLightingPolicy(input(current, config, activeProfile, 720,
                                   &command));

  assert(!decision.accepted);
  assert(decision.reason == LightingPolicyReason::kInvalidCommand);
  assert(decision.channels[0].targetDuty == 55);
  assert(decision.channels[1].targetDuty == 44);
  assert(!decision.channels[0].shouldWrite);
}

void testAutomaticCurveExactPointAndInterpolation() {
  const LightingState current = state(0, 0, 0, 0);
  const LightingConfig config = memoryConfig(AUTOMATIC);
  const LightingProfile activeProfile = profile();

  const auto exact =
      evaluateLightingPolicy(input(current, config, activeProfile, 720));
  assert(exact.channels[0].targetDuty == 200);

  const auto interpolated =
      evaluateLightingPolicy(input(current, config, activeProfile, 600));
  assert(interpolated.channels[0].targetDuty > 10);
  assert(interpolated.channels[0].targetDuty < 200);
}

void testSunriseAndSunsetModifyTargets() {
  const LightingState current = state(0, 0, 0, 0);
  const LightingConfig config = memoryConfig(AUTOMATIC);
  const LightingProfile activeProfile = profile();

  const auto sunriseStart =
      evaluateLightingPolicy(input(current, config, activeProfile, 480));
  const auto sunriseLater =
      evaluateLightingPolicy(input(current, config, activeProfile, 510));
  const auto sunsetLater =
      evaluateLightingPolicy(input(current, config, activeProfile, 1190));

  assert(sunriseStart.channels[0].targetDuty == 0);
  assert(sunriseLater.channels[0].targetDuty > sunriseStart.channels[0].targetDuty);
  assert(sunriseLater.channels[0].reason == LightingPolicyReason::kSunrise);
  assert(sunsetLater.channels[0].reason == LightingPolicyReason::kSunset);
  assert(sunsetLater.channels[0].targetDuty < 200);
}

void testMoonlightOutsideMainPeriodDoesNotCreateExtraChannel() {
  const LightingState current = state(0, 0, 0, 0);
  const LightingConfig config = memoryConfig(AUTOMATIC);
  const LightingProfile activeProfile = profile();

  const auto decision =
      evaluateLightingPolicy(input(current, config, activeProfile, 60));

  assert(kLightingFunctionalChannelCount == 4);
  assert(decision.channels[0].reason == LightingPolicyReason::kMoonlight);
  assert(decision.channels[0].targetDuty > 0);
  assert(decision.channels[0].targetDuty <= 10);
  assert(decision.channels[3].channel == UV);
}

void testAcclimationAppliesGradualFactor() {
  const LightingState current = state(0, 0, 0, 0);
  const LightingConfig config = memoryConfig(ACCLIMATION);
  LightingProfile activeProfile = profile();
  activeProfile.sunriseEnabled = false;
  activeProfile.sunsetEnabled = false;

  const auto firstDay =
      evaluateLightingPolicy(input(current, config, activeProfile, 720, nullptr,
                                   1));
  const auto lastDay =
      evaluateLightingPolicy(input(current, config, activeProfile, 720, nullptr,
                                   10));

  assert(firstDay.channels[0].targetDuty == 20);
  assert(lastDay.channels[0].targetDuty == 200);
  assert(firstDay.channels[0].reason == LightingPolicyReason::kAcclimation);
}

void testFadeLimitsIncreaseAndDecrease() {
  const LightingConfig config = memoryConfig(AUTOMATIC);
  LightingProfile activeProfile = profile();
  activeProfile.sunriseEnabled = false;
  activeProfile.sunsetEnabled = false;
  const LightingModuleConfig fade = moduleConfig(5);

  const auto increase =
      evaluateLightingPolicy(input(state(0, 0, 0, 0), config, activeProfile,
                                   720, nullptr, 10, fade));
  const auto decrease =
      evaluateLightingPolicy(input(state(50, 0, 0, 0), config, activeProfile,
                                   60, nullptr, 10, fade));

  assert(increase.channels[0].targetDuty == 200);
  assert(increase.channels[0].nextDuty == 5);
  assert(increase.channels[0].reason == LightingPolicyReason::kFade);
  assert(decrease.channels[0].targetDuty < 50);
  assert(decrease.channels[0].nextDuty == 45);
}

void testDisabledChannelConvergesToZeroByFade() {
  LightingConfig config = memoryConfig(AUTOMATIC);
  config.uv.enabled = false;
  const LightingProfile activeProfile = profile();
  const LightingModuleConfig fade = moduleConfig(5);

  const auto decision =
      evaluateLightingPolicy(input(state(0, 0, 0, 40), config, activeProfile,
                                   720, nullptr, 10, fade));

  assert(decision.channels[3].channel == UV);
  assert(decision.channels[3].targetDuty == 0);
  assert(decision.channels[3].nextDuty == 35);
  assert(decision.channels[3].reason == LightingPolicyReason::kChannelDisabled);
}

void testInvalidConfigConvergesActiveChannelsToZero() {
  LightingConfig config = memoryConfig(AUTOMATIC);
  config.startMinuteOfDay = 1200;
  config.endMinuteOfDay = 480;
  const LightingProfile activeProfile = profile();
  const LightingModuleConfig fade = moduleConfig(5);

  const auto decision =
      evaluateLightingPolicy(input(state(30, 0, 0, 0), config, activeProfile,
                                   720, nullptr, 10, fade));

  assert(!decision.accepted);
  assert(decision.reason == LightingPolicyReason::kInvalidConfig);
  assert(decision.channels[0].targetDuty == 0);
  assert(decision.channels[0].nextDuty == 25);
}

void testInvalidProfileConvergesAutomaticAndAcclimationToZero() {
  const LightingConfig automaticConfig = memoryConfig(AUTOMATIC);
  const LightingConfig acclimationConfig = memoryConfig(ACCLIMATION);
  LightingProfile invalidProfile = profile();
  invalidProfile.white.curve.points[1].minuteOfDay = 0;
  const LightingModuleConfig fade = moduleConfig(5);

  const auto automaticDecision =
      evaluateLightingPolicy(input(state(30, 0, 0, 0), automaticConfig,
                                   invalidProfile, 720, nullptr, 10, fade));
  const auto acclimationDecision =
      evaluateLightingPolicy(input(state(30, 0, 0, 0), acclimationConfig,
                                   invalidProfile, 720, nullptr, 1, fade));

  assert(!automaticDecision.accepted);
  assert(automaticDecision.reason == LightingPolicyReason::kInvalidProfile);
  assert(automaticDecision.channels[0].targetDuty == 0);
  assert(automaticDecision.channels[0].nextDuty == 25);
  assert(!acclimationDecision.accepted);
  assert(acclimationDecision.reason == LightingPolicyReason::kInvalidProfile);
  assert(acclimationDecision.channels[0].nextDuty == 25);
}

void testManualIntensityCommandUsesPercentTarget() {
  const LightingState current = state(0, 0, 0, 0);
  const LightingConfig config = memoryConfig(MANUAL);
  const LightingProfile activeProfile = profile();
  const auto command =
      makeLocalLightingIntensityCommand(MANUAL, WHITE, 50, 10);

  const auto decision =
      evaluateLightingPolicy(input(current, config, activeProfile, 720,
                                   &command));

  assert(decision.channels[0].targetDuty == 100);
}

}  // namespace

int main() {
  testManualModeSetsValidWhiteTarget();
  testManualModeTurnsBlueOff();
  testManualModeLimitsTargetByChannelAndGlobalLimits();
  testManualModeLimitsTargetByChannelIntensity();
  testManualIdempotentCommandDoesNotRequestWrite();
  testInvalidManualCommandPreservesCurrentTargets();
  testAutomaticCurveExactPointAndInterpolation();
  testSunriseAndSunsetModifyTargets();
  testMoonlightOutsideMainPeriodDoesNotCreateExtraChannel();
  testAcclimationAppliesGradualFactor();
  testFadeLimitsIncreaseAndDecrease();
  testDisabledChannelConvergesToZeroByFade();
  testInvalidConfigConvergesActiveChannelsToZero();
  testInvalidProfileConvergesAutomaticAndAcclimationToZero();
  testManualIntensityCommandUsesPercentTarget();
  return 0;
}
