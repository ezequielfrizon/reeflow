#!/usr/bin/env bash
set -euo pipefail

FIRMWARE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${TMPDIR:-/tmp}/reeflow_phase8_lighting_pwm_controller_tests"
mkdir -p "$BUILD_DIR"

g++ -std=c++17 -Wall -Wextra -Werror \
  -I"$FIRMWARE_DIR/include" \
  -I"$FIRMWARE_DIR/src" \
  -I"$FIRMWARE_DIR/test" \
  "$FIRMWARE_DIR/test/unit/test_lighting_pwm_controller.cpp" \
  "$FIRMWARE_DIR/src/drivers/lighting/ledc_lighting_pwm_controller.cpp" \
  -o "$BUILD_DIR/test_lighting_pwm_controller"

"$BUILD_DIR/test_lighting_pwm_controller"
