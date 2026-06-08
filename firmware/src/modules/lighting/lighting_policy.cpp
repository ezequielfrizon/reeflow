#include "modules/lighting/lighting_policy.h"

#include <stddef.h>
#include <stdint.h>

namespace reeflow::modules::lighting {
namespace {

constexpr LightingChannel kChannels[] = {WHITE, BLUE, ROYAL_BLUE, UV};

const core::state::LightingChannelState& stateChannel(
    const core::state::LightingState& state, LightingChannel channel) {
  return *lightingChannelState(state, channel);
}

const config::LightingChannelConfig& configChannel(
    const config::LightingConfig& config, LightingChannel channel) {
  switch (channel) {
    case LightingChannel::kWhite:
      return config.white;
    case LightingChannel::kBlue:
      return config.blue;
    case LightingChannel::kRoyalBlue:
      return config.royalBlue;
    case LightingChannel::kUv:
      return config.uv;
    case LightingChannel::kUnknown:
      return config.white;
  }

  return config.white;
}

const LightingChannelProfile& profileChannel(const LightingProfile& profile,
                                             LightingChannel channel) {
  switch (channel) {
    case LightingChannel::kWhite:
      return profile.white;
    case LightingChannel::kBlue:
      return profile.blue;
    case LightingChannel::kRoyalBlue:
      return profile.royalBlue;
    case LightingChannel::kUv:
      return profile.uv;
    case LightingChannel::kUnknown:
      return profile.white;
  }

  return profile.white;
}

bool validLightingConfig(const config::LightingConfig& config) {
  return isCanonicalLightingMode(config.mode) &&
         validLightingMinuteOfDay(config.startMinuteOfDay) &&
         validLightingMinuteOfDay(config.endMinuteOfDay) &&
         config.startMinuteOfDay < config.endMinuteOfDay &&
         validLightingIntensityPercent(config.maxIntensityPercent) &&
         validLightingDuty(config.white.maxPWM) &&
         validLightingDuty(config.blue.maxPWM) &&
         validLightingDuty(config.royalBlue.maxPWM) &&
         validLightingDuty(config.uv.maxPWM);
}

bool validProfileWindow(const LightingProfile& profile) {
  return validLightingMinuteOfDay(profile.startMinuteOfDay) &&
         validLightingMinuteOfDay(profile.endMinuteOfDay) &&
         profile.startMinuteOfDay < profile.endMinuteOfDay;
}

bool validCommand(const LightingCommand& command) {
  if (command.source != LightingCommandSource::kLocal ||
      !isCanonicalLightingMode(command.requestedMode)) {
    return false;
  }

  switch (command.target) {
    case LightingCommandTarget::kMode:
    case LightingCommandTarget::kProfile:
    case LightingCommandTarget::kAllChannelsOff:
      return true;
    case LightingCommandTarget::kChannelDuty:
      return isCanonicalLightingChannel(command.channel) &&
             validLightingDuty(command.requestedDuty);
    case LightingCommandTarget::kChannelIntensity:
      return isCanonicalLightingChannel(command.channel) &&
             validLightingIntensityPercent(
                 command.requestedIntensityPercent);
  }

  return false;
}

uint16_t minimumDutyLimit(const config::LightingConfig& config,
                          const LightingProfile& profile,
                          LightingChannel channel) {
  uint16_t limit = kLightingMaxDuty;
  const config::LightingChannelConfig& memoryChannel =
      configChannel(config, channel);
  const LightingChannelProfile& activeChannel = profileChannel(profile, channel);

  if (memoryChannel.maxPWM < limit) {
    limit = memoryChannel.maxPWM;
  }

  if (activeChannel.maxDuty < limit) {
    limit = activeChannel.maxDuty;
  }

  return limit;
}

uint8_t globalIntensityLimitPercent(const config::LightingConfig& config,
                                    const LightingProfile& profile) {
  return config.maxIntensityPercent < profile.maxGlobalIntensityPercent
             ? config.maxIntensityPercent
             : profile.maxGlobalIntensityPercent;
}

uint8_t channelIntensityLimitPercent(const LightingProfile& profile,
                                     LightingChannel channel) {
  return profileChannel(profile, channel).maxIntensityPercent;
}

uint16_t applyPercent(uint16_t value, uint16_t percent) {
  return static_cast<uint16_t>((static_cast<uint32_t>(value) * percent) / 100U);
}

uint16_t clampDutyForChannel(uint16_t duty, const config::LightingConfig& config,
                             const LightingProfile& profile,
                             LightingChannel channel) {
  uint16_t limited = duty;
  const uint16_t channelLimit = minimumDutyLimit(config, profile, channel);
  if (limited > channelLimit) {
    limited = channelLimit;
  }

  limited = applyPercent(limited, channelIntensityLimitPercent(profile, channel));
  limited = applyPercent(limited, globalIntensityLimitPercent(config, profile));
  return limited;
}

uint16_t curvePointAsDuty(const LightingCurvePoint& point,
                          uint16_t channelLimit) {
  if (point.targetType == LightingCurveTargetType::kDuty) {
    return point.value > channelLimit ? channelLimit : point.value;
  }

  return applyPercent(channelLimit, point.value);
}

uint16_t interpolateDuty(uint16_t leftDuty, uint16_t rightDuty,
                         uint16_t leftMinute, uint16_t rightMinute,
                         uint16_t currentMinute) {
  if (rightMinute <= leftMinute) {
    return leftDuty;
  }

  const uint32_t span = rightMinute - leftMinute;
  const uint32_t elapsed = currentMinute - leftMinute;
  if (rightDuty >= leftDuty) {
    return static_cast<uint16_t>(
        leftDuty + (((rightDuty - leftDuty) * elapsed) / span));
  }

  return static_cast<uint16_t>(
      leftDuty - (((leftDuty - rightDuty) * elapsed) / span));
}

uint16_t evaluateCurveDuty(const LightingCurve& curve, uint16_t minuteOfDay,
                           uint16_t channelLimit) {
  if (validateLightingCurve(curve) != LightingCurveValidationResult::kValid) {
    return 0;
  }

  if (minuteOfDay <= curve.points[0].minuteOfDay) {
    return curvePointAsDuty(curve.points[0], channelLimit);
  }

  for (uint8_t index = 1; index < curve.pointCount; ++index) {
    const LightingCurvePoint& right = curve.points[index];
    if (minuteOfDay == right.minuteOfDay) {
      return curvePointAsDuty(right, channelLimit);
    }

    if (minuteOfDay < right.minuteOfDay) {
      const LightingCurvePoint& left = curve.points[index - 1];
      return interpolateDuty(curvePointAsDuty(left, channelLimit),
                             curvePointAsDuty(right, channelLimit),
                             left.minuteOfDay, right.minuteOfDay, minuteOfDay);
    }
  }

  return curvePointAsDuty(curve.points[curve.pointCount - 1], channelLimit);
}

bool channelEnabled(const config::LightingConfig& config,
                    const LightingProfile& profile, LightingChannel channel) {
  return configChannel(config, channel).enabled &&
         profileChannel(profile, channel).enabled;
}

LightingPolicyReason automaticPeriodReason(const LightingProfile& profile,
                                           const LightingModuleConfig& config,
                                           uint16_t minuteOfDay) {
  if (minuteOfDay < profile.startMinuteOfDay ||
      minuteOfDay > profile.endMinuteOfDay) {
    return LightingPolicyReason::kMoonlight;
  }

  const uint16_t sunriseEnd =
      profile.startMinuteOfDay + config.sunriseDurationMinutes >
              profile.endMinuteOfDay
          ? profile.endMinuteOfDay
          : profile.startMinuteOfDay + config.sunriseDurationMinutes;
  const uint16_t sunsetStart =
      profile.endMinuteOfDay > config.sunsetDurationMinutes &&
              profile.endMinuteOfDay - config.sunsetDurationMinutes >
                  profile.startMinuteOfDay
          ? profile.endMinuteOfDay - config.sunsetDurationMinutes
          : profile.startMinuteOfDay;

  if (profile.sunriseEnabled && minuteOfDay >= profile.startMinuteOfDay &&
      minuteOfDay < sunriseEnd) {
    return LightingPolicyReason::kSunrise;
  }

  if (profile.sunsetEnabled && minuteOfDay > sunsetStart &&
      minuteOfDay <= profile.endMinuteOfDay) {
    return LightingPolicyReason::kSunset;
  }

  return LightingPolicyReason::kAutomaticCurve;
}

uint16_t applyPeriodModifier(uint16_t duty, const LightingProfile& profile,
                             const LightingModuleConfig& config,
                             uint16_t minuteOfDay,
                             LightingPolicyReason reason) {
  if (reason == LightingPolicyReason::kSunrise) {
    const uint16_t elapsed = minuteOfDay - profile.startMinuteOfDay;
    const uint16_t duration =
        config.sunriseDurationMinutes == 0 ? 1 : config.sunriseDurationMinutes;
    return applyPercent(duty, (elapsed * 100U) / duration);
  }

  if (reason == LightingPolicyReason::kSunset) {
    const uint16_t sunsetStart =
        profile.endMinuteOfDay > config.sunsetDurationMinutes
            ? profile.endMinuteOfDay - config.sunsetDurationMinutes
            : profile.startMinuteOfDay;
    const uint16_t remaining = profile.endMinuteOfDay > minuteOfDay
                                   ? profile.endMinuteOfDay - minuteOfDay
                                   : 0;
    const uint16_t duration = profile.endMinuteOfDay > sunsetStart
                                  ? profile.endMinuteOfDay - sunsetStart
                                  : 1;
    return applyPercent(duty, (remaining * 100U) / duration);
  }

  if (reason == LightingPolicyReason::kMoonlight) {
    return applyPercent(duty, config.moonlightMaxIntensityPercent);
  }

  return duty;
}

uint16_t applyAcclimation(uint16_t duty, const LightingProfile& profile,
                          uint16_t elapsedDays) {
  if (!profile.acclimationEnabled || profile.acclimationDurationDays == 0) {
    return duty;
  }

  const uint16_t cappedElapsed =
      elapsedDays > profile.acclimationDurationDays
          ? profile.acclimationDurationDays
          : elapsedDays;
  return static_cast<uint16_t>(
      (static_cast<uint32_t>(duty) * cappedElapsed) /
      profile.acclimationDurationDays);
}

uint16_t fadeNextDuty(uint16_t currentDuty, uint16_t targetDuty,
                      const LightingModuleConfig& config) {
  if (currentDuty == targetDuty) {
    return currentDuty;
  }

  const uint16_t step = config.maxFadeStepPerEvaluation;
  if (targetDuty > currentDuty) {
    const uint16_t delta = targetDuty - currentDuty;
    return delta <= step ? targetDuty : currentDuty + step;
  }

  const uint16_t delta = currentDuty - targetDuty;
  return delta <= step ? targetDuty : currentDuty - step;
}

void setDecision(LightingPolicyDecision& decision, size_t index,
                 LightingChannel channel, uint16_t currentDuty,
                 uint16_t targetDuty, const LightingModuleConfig& config,
                 LightingPolicyReason reason) {
  const uint16_t nextDuty = fadeNextDuty(currentDuty, targetDuty, config);
  decision.channels[index] = {channel, currentDuty, targetDuty, nextDuty,
                              nextDuty != currentDuty, reason};
  if (reason == LightingPolicyReason::kChannelDisabled ||
      reason == LightingPolicyReason::kInvalidConfig ||
      reason == LightingPolicyReason::kInvalidProfile) {
    return;
  }

  if (nextDuty != targetDuty) {
    decision.channels[index].reason = LightingPolicyReason::kFade;
  } else if (nextDuty == currentDuty) {
    decision.channels[index].reason = LightingPolicyReason::kIdempotent;
  }
}

void convergeAllToZero(LightingPolicyDecision& decision,
                       const LightingPolicyInput& input,
                       LightingPolicyReason reason) {
  for (size_t index = 0; index < kLightingFunctionalChannelCount; ++index) {
    const LightingChannel channel = kChannels[index];
    setDecision(decision, index, channel,
                stateChannel(input.currentState, channel).currentPWM, 0,
                input.moduleConfig, reason);
  }
}

void preserveCurrent(LightingPolicyDecision& decision,
                     const LightingPolicyInput& input,
                     LightingPolicyReason reason) {
  for (size_t index = 0; index < kLightingFunctionalChannelCount; ++index) {
    const LightingChannel channel = kChannels[index];
    const uint16_t currentDuty =
        stateChannel(input.currentState, channel).currentPWM;
    decision.channels[index] = {channel, currentDuty, currentDuty, currentDuty,
                                false, reason};
  }
}

uint16_t profileTargetForChannel(const LightingPolicyInput& input,
                                 LightingChannel channel,
                                 LightingPolicyReason& reason) {
  if (!channelEnabled(input.lightingConfig, input.activeProfile, channel)) {
    reason = LightingPolicyReason::kChannelDisabled;
    return 0;
  }

  const uint16_t channelLimit =
      minimumDutyLimit(input.lightingConfig, input.activeProfile, channel);
  uint16_t targetDuty =
      evaluateCurveDuty(profileChannel(input.activeProfile, channel).curve,
                        input.currentMinuteOfDay, channelLimit);
  reason = automaticPeriodReason(input.activeProfile, input.moduleConfig,
                                 input.currentMinuteOfDay);
  targetDuty = applyPeriodModifier(targetDuty, input.activeProfile,
                                   input.moduleConfig,
                                   input.currentMinuteOfDay, reason);
  targetDuty =
      clampDutyForChannel(targetDuty, input.lightingConfig, input.activeProfile,
                          channel);
  if (input.lightingConfig.mode == ACCLIMATION ||
      (input.command != nullptr && input.command->requestedMode == ACCLIMATION)) {
    targetDuty = applyAcclimation(targetDuty, input.activeProfile,
                                  input.acclimationElapsedDays);
    if (reason == LightingPolicyReason::kAutomaticCurve) {
      reason = LightingPolicyReason::kAcclimation;
    }
  }

  return targetDuty;
}

void evaluateProfileTargets(LightingPolicyDecision& decision,
                            const LightingPolicyInput& input,
                            LightingPolicyReason forcedReason) {
  for (size_t index = 0; index < kLightingFunctionalChannelCount; ++index) {
    const LightingChannel channel = kChannels[index];
    LightingPolicyReason reason = forcedReason;
    uint16_t targetDuty = profileTargetForChannel(input, channel, reason);
    if (forcedReason == LightingPolicyReason::kManualProfile &&
        reason != LightingPolicyReason::kChannelDisabled) {
      reason = forcedReason;
    }
    setDecision(decision, index, channel,
                stateChannel(input.currentState, channel).currentPWM,
                targetDuty, input.moduleConfig, reason);
  }
}

void evaluateManualCommand(LightingPolicyDecision& decision,
                           const LightingPolicyInput& input,
                           const LightingCommand& command) {
  if (command.target == LightingCommandTarget::kProfile) {
    evaluateProfileTargets(decision, input, LightingPolicyReason::kManualProfile);
    return;
  }

  if (command.target == LightingCommandTarget::kAllChannelsOff) {
    convergeAllToZero(decision, input, LightingPolicyReason::kManualCommand);
    return;
  }

  for (size_t index = 0; index < kLightingFunctionalChannelCount; ++index) {
    const LightingChannel channel = kChannels[index];
    uint16_t currentDuty = stateChannel(input.currentState, channel).currentPWM;
    uint16_t targetDuty = currentDuty;
    LightingPolicyReason reason = LightingPolicyReason::kIdempotent;

    if (!channelEnabled(input.lightingConfig, input.activeProfile, channel)) {
      targetDuty = 0;
      reason = LightingPolicyReason::kChannelDisabled;
    } else if (command.target == LightingCommandTarget::kChannelDuty &&
               command.channel == channel) {
      targetDuty = clampDutyForChannel(command.requestedDuty,
                                       input.lightingConfig,
                                       input.activeProfile, channel);
      reason = LightingPolicyReason::kManualCommand;
    } else if (command.target == LightingCommandTarget::kChannelIntensity &&
               command.channel == channel) {
      targetDuty = applyPercent(minimumDutyLimit(input.lightingConfig,
                                                 input.activeProfile, channel),
                                command.requestedIntensityPercent);
      targetDuty = clampDutyForChannel(targetDuty, input.lightingConfig,
                                       input.activeProfile, channel);
      reason = LightingPolicyReason::kManualCommand;
    }

    setDecision(decision, index, channel, currentDuty, targetDuty,
                input.moduleConfig, reason);
  }
}

}  // namespace

LightingPolicyDecision evaluateLightingPolicy(
    const LightingPolicyInput& input) {
  LightingPolicyDecision decision = {};
  decision.mode =
      input.command != nullptr ? input.command->requestedMode : input.lightingConfig.mode;
  decision.accepted = true;
  decision.reason = LightingPolicyReason::kNone;
  decision.profileValidation = validateLightingProfile(input.activeProfile);

  if (!validLightingModuleConfig(input.moduleConfig) ||
      !validLightingConfig(input.lightingConfig) ||
      !validLightingMinuteOfDay(input.currentMinuteOfDay)) {
    decision.accepted = false;
    decision.reason = LightingPolicyReason::kInvalidConfig;
    convergeAllToZero(decision, input, LightingPolicyReason::kInvalidConfig);
    return decision;
  }

  if (decision.profileValidation != LightingProfileValidationResult::kValid ||
      !validProfileWindow(input.activeProfile)) {
    decision.accepted = false;
    decision.reason = LightingPolicyReason::kInvalidProfile;
    convergeAllToZero(decision, input, LightingPolicyReason::kInvalidProfile);
    return decision;
  }

  if (input.command != nullptr && !validCommand(*input.command)) {
    decision.accepted = false;
    decision.reason = LightingPolicyReason::kInvalidCommand;
    preserveCurrent(decision, input, LightingPolicyReason::kInvalidCommand);
    return decision;
  }

  if (input.command != nullptr && input.command->requestedMode == MANUAL &&
      input.command->target != LightingCommandTarget::kMode) {
    decision.reason = LightingPolicyReason::kManualCommand;
    evaluateManualCommand(decision, input, *input.command);
    return decision;
  }

  if (decision.mode == MANUAL) {
    decision.reason = LightingPolicyReason::kIdempotent;
    preserveCurrent(decision, input, LightingPolicyReason::kIdempotent);
    return decision;
  }

  decision.reason = decision.mode == ACCLIMATION
                        ? LightingPolicyReason::kAcclimation
                        : LightingPolicyReason::kAutomaticCurve;
  evaluateProfileTargets(decision, input, decision.reason);
  return decision;
}

}  // namespace reeflow::modules::lighting
