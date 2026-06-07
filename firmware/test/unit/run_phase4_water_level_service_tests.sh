#!/usr/bin/env bash
set -euo pipefail

FIRMWARE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${TMPDIR:-/tmp}/reeflow_phase4_water_level_service_tests"
mkdir -p "$BUILD_DIR"

g++ -std=c++17 -Wall -Wextra -Werror \
  -I"$FIRMWARE_DIR/include" \
  -I"$FIRMWARE_DIR/src" \
  -I"$FIRMWARE_DIR/test" \
  "$FIRMWARE_DIR/test/unit/test_water_level_service.cpp" \
  "$FIRMWARE_DIR/src/modules/water_level/water_level_service.cpp" \
  "$FIRMWARE_DIR/src/modules/water_level/water_level_calibration.cpp" \
  "$FIRMWARE_DIR/src/modules/water_level/water_level_state_evaluator.cpp" \
  "$FIRMWARE_DIR/src/modules/water_level/water_level_events.cpp" \
  "$FIRMWARE_DIR/src/config/config_manager.cpp" \
  "$FIRMWARE_DIR/src/core/state/system_state.cpp" \
  "$FIRMWARE_DIR/src/core/events/event_bus.cpp" \
  -o "$BUILD_DIR/test_water_level_service"

"$BUILD_DIR/test_water_level_service"
