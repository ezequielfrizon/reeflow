#pragma once

#include <stddef.h>
#include <stdint.h>

#include "config/pwm_bringup_config.h"
#include "drivers/io/bringup_timer.h"
#include "drivers/io/pwm_ledc_interface.h"

namespace reeflow::drivers {

struct PwmDiagnosticTarget {
  size_t physicalIndex;
  config::PwmBringupChannel channel;
};

struct PwmDiagnosticResult {
  size_t channelsTested;
};

size_t pwmDiagnosticChannelCount();
bool getPwmDiagnosticTarget(size_t physicalIndex, PwmDiagnosticTarget& target);
void configurePwmBringupChannels(PwmLedcPort& pwm);
void configurePwmBringupChannels();
void applyAllPwmDutyZero(PwmLedcPort& pwm);
void applyAllPwmDutyZero();
void applyPwmDuty(PwmLedcPort& pwm, const PwmDiagnosticTarget& target,
                  uint16_t duty);
void applyPwmDuty(const PwmDiagnosticTarget& target, uint16_t duty);
PwmDiagnosticResult runPwmBringupDiagnosticSequence(PwmLedcPort& pwm,
                                                    BringupTimer& timer,
                                                    unsigned long holdMs);
PwmDiagnosticResult runPwmBringupDiagnosticSequence(unsigned long holdMs);

}  // namespace reeflow::drivers
