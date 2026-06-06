#!/usr/bin/env bash
set -euo pipefail

FIRMWARE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${TMPDIR:-/tmp}/reeflow_phase1_bringup_tests"
mkdir -p "$BUILD_DIR"

g++ -std=c++17 -Wall -Wextra -Werror \
  -I"$FIRMWARE_DIR/include" \
  -I"$FIRMWARE_DIR/src" \
  "$FIRMWARE_DIR/test/unit/test_phase1_bringup_logic.cpp" \
  "$FIRMWARE_DIR/src/config/pwm_bringup_config.cpp" \
  "$FIRMWARE_DIR/src/diagnostics/hardware_bringup_identity.cpp" \
  "$FIRMWARE_DIR/src/diagnostics/pinout/pinout_diagnostics.cpp" \
  "$FIRMWARE_DIR/src/drivers/initial_safe_state.cpp" \
  "$FIRMWARE_DIR/src/drivers/pwm/pwm_diagnostic_driver.cpp" \
  "$FIRMWARE_DIR/src/drivers/relays/relay_diagnostic_driver.cpp" \
  -o "$BUILD_DIR/test_phase1_bringup_logic"

"$BUILD_DIR/test_phase1_bringup_logic"
