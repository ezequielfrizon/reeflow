#include "diagnostics/pwm/pwm_diagnostics.h"

#include "config/pwm_bringup_config.h"
#include "drivers/pwm/pwm_diagnostic_driver.h"

namespace reeflow::diagnostics {

void printPwmBringupConfiguration(Stream& output) {
  const config::PwmBringupParameters& parameters = config::pwmBringupParameters();

  output.println();
  output.println("PWM Bring-Up Configuration");
  output.print("Frequency Hz: ");
  output.println(parameters.frequencyHz);
  output.print("Resolution bits: ");
  output.println(parameters.resolutionBits);
  output.print("Duty zero: ");
  output.println(parameters.dutyZero);
  output.print("Duty test minimum: ");
  output.println(parameters.dutyTestMin);
  output.print("Duty test maximum: ");
  output.println(parameters.dutyTestMax);
  output.println(parameters.note);
  output.println("Canonical Name | GPIO | LEDC Channel");

  for (size_t index = 0; index < config::pwmBringupChannelCount(); ++index) {
    config::PwmBringupChannel channel = {};
    if (!config::getPwmBringupChannel(index, channel) || channel.pin == nullptr) {
      continue;
    }

    output.print(channel.pin->canonicalName);
    output.print(" | GPIO");
    output.print(channel.pin->gpio);
    output.print(" | ");
    output.println(channel.ledcChannel);
  }
}

void runPwmBringupDiagnostic(Stream& serial, unsigned long holdMs) {
  const config::PwmBringupParameters& parameters = config::pwmBringupParameters();

  serial.println();
  serial.println("PWM Bring-Up Diagnostic");
  serial.print("Frequency Hz: ");
  serial.println(parameters.frequencyHz);
  serial.print("Resolution bits: ");
  serial.println(parameters.resolutionBits);
  serial.println("Only one PWM channel is active at a time.");

  const drivers::PwmDiagnosticResult result =
      drivers::runPwmBringupDiagnosticSequence(holdMs);

  serial.println();
  serial.print("PWM channels tested: ");
  serial.println(result.channelsTested);
  serial.println("All PWM channels returned to duty zero.");
}

}  // namespace reeflow::diagnostics
