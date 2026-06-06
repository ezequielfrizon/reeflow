#include "drivers/pwm/pwm_diagnostic_driver.h"

namespace reeflow::drivers {

size_t pwmDiagnosticChannelCount() {
  return config::pwmBringupChannelCount();
}

bool getPwmDiagnosticTarget(size_t physicalIndex, PwmDiagnosticTarget& target) {
  if (physicalIndex == 0) {
    target = {};
    return false;
  }

  config::PwmBringupChannel channel = {};
  if (!config::getPwmBringupChannel(physicalIndex - 1, channel) ||
      channel.pin == nullptr) {
    target = {};
    return false;
  }

  target = {physicalIndex, channel};
  return true;
}

void configurePwmBringupChannels(PwmLedcPort& pwm) {
  const config::PwmBringupParameters& parameters = config::pwmBringupParameters();

  for (size_t index = 0; index < config::pwmBringupChannelCount(); ++index) {
    config::PwmBringupChannel channel = {};
    if (!config::getPwmBringupChannel(index, channel) || channel.pin == nullptr) {
      continue;
    }

    pwm.setup(channel.ledcChannel, parameters.frequencyHz, parameters.resolutionBits);
    pwm.attachPin(channel.pin->gpio, channel.ledcChannel);
    pwm.write(channel.ledcChannel, parameters.dutyZero);
  }
}

void applyAllPwmDutyZero(PwmLedcPort& pwm) {
  const config::PwmBringupParameters& parameters = config::pwmBringupParameters();

  for (size_t index = 0; index < config::pwmBringupChannelCount(); ++index) {
    config::PwmBringupChannel channel = {};
    if (!config::getPwmBringupChannel(index, channel) || channel.pin == nullptr) {
      continue;
    }

    pwm.write(channel.ledcChannel, parameters.dutyZero);
  }
}

void applyPwmDuty(PwmLedcPort& pwm, const PwmDiagnosticTarget& target,
                  uint16_t duty) {
  if (target.channel.pin == nullptr) {
    return;
  }

  pwm.write(target.channel.ledcChannel, duty);
}

PwmDiagnosticResult runPwmBringupDiagnosticSequence(PwmLedcPort& pwm,
                                                    BringupTimer& timer,
                                                    unsigned long holdMs) {
  const config::PwmBringupParameters& parameters = config::pwmBringupParameters();
  PwmDiagnosticResult result = {};

  configurePwmBringupChannels(pwm);
  applyAllPwmDutyZero(pwm);

  for (size_t index = 1; index <= pwmDiagnosticChannelCount(); ++index) {
    PwmDiagnosticTarget target = {};
    if (!getPwmDiagnosticTarget(index, target) || target.channel.pin == nullptr) {
      continue;
    }

    applyAllPwmDutyZero(pwm);
    applyPwmDuty(pwm, target, parameters.dutyTestMin);
    timer.delayMs(holdMs);
    applyPwmDuty(pwm, target, parameters.dutyTestMax);
    timer.delayMs(holdMs);
    applyPwmDuty(pwm, target, parameters.dutyZero);
    applyAllPwmDutyZero(pwm);
    ++result.channelsTested;
  }

  applyAllPwmDutyZero(pwm);
  return result;
}

}  // namespace reeflow::drivers
