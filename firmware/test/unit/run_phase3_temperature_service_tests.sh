#!/usr/bin/env bash
set -euo pipefail

FIRMWARE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${TMPDIR:-/tmp}/reeflow_phase3_temperature_service_tests"
mkdir -p "$BUILD_DIR"

g++ -std=c++17 -Wall -Wextra -Werror \
  -I"$FIRMWARE_DIR/include" \
  -I"$FIRMWARE_DIR/src" \
  -I"$FIRMWARE_DIR/test" \
  "$FIRMWARE_DIR/test/unit/test_temperature_service.cpp" \
  "$FIRMWARE_DIR/src/modules/temperature/temperature_service.cpp" \
  "$FIRMWARE_DIR/src/modules/temperature/temperature_state_evaluator.cpp" \
  "$FIRMWARE_DIR/src/modules/temperature/temperature_events.cpp" \
  "$FIRMWARE_DIR/src/config/config_manager.cpp" \
  "$FIRMWARE_DIR/src/core/state/system_state.cpp" \
  "$FIRMWARE_DIR/src/core/events/event_bus.cpp" \
  -o "$BUILD_DIR/test_temperature_service"

"$BUILD_DIR/test_temperature_service"
