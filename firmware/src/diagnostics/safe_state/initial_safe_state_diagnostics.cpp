#include "diagnostics/safe_state/initial_safe_state_diagnostics.h"

namespace reeflow::diagnostics {

void printInitialSafeHardwareState(Stream& output, size_t relaysConfigured,
                                   size_t pwmChannelsConfigured,
                                   size_t sensorBusPinsPrepared) {
  output.println();
  output.println("Initial Safe Hardware State");
  output.println("Status: applied");
  output.print("Relays configured OFF: ");
  output.println(relaysConfigured);
  output.print("PWM channels configured at duty zero: ");
  output.println(pwmChannelsConfigured);
  output.print("Sensor bus pins prepared without actuation: ");
  output.println(sensorBusPinsPrepared);
}

}  // namespace reeflow::diagnostics
