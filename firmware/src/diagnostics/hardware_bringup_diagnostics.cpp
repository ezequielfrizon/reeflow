#include "diagnostics/hardware_bringup_diagnostics.h"

#include "diagnostics/hardware_bringup_identity.h"
#include "diagnostics/pwm/pwm_diagnostics.h"
#include "diagnostics/relays/relay_diagnostics.h"

namespace reeflow::diagnostics {
namespace {

constexpr unsigned long kRelayDiagnosticHoldMs = 1000;
constexpr unsigned long kPwmDiagnosticHoldMs = 1000;

}  // namespace

void printHardwareBringupBanner(Stream& output) {
  output.println();
  output.println("REEFLOW Firmware");
  output.print("Mode: ");
  output.println(hardwareBringupModeName());
  output.println("Scope: Phase 1 hardware diagnostics only");
  output.println("Status: minimal diagnostic firmware ready");
}

void runHardwareBringupLoop(Stream& serial) {
  if (serial.available() <= 0) {
    yield();
    return;
  }

  const int input = serial.read();
  if (input == '\n' || input == '\r' || input == ' ') {
    yield();
    return;
  }

  if (input == 'p' || input == 'P') {
    runPwmBringupDiagnostic(serial, kPwmDiagnosticHoldMs);
    yield();
    return;
  }

  if (input < '1' || input > '4') {
    serial.println("Diagnostic command ignored. Use 1, 2, 3, 4 or p.");
    yield();
    return;
  }

  const uint8_t relayIndex = static_cast<uint8_t>(input - '0');
  runRelayBringupDiagnostic(serial, relayIndex, kRelayDiagnosticHoldMs);

  yield();
}

}  // namespace reeflow::diagnostics
