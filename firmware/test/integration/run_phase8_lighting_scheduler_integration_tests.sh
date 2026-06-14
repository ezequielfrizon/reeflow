#!/usr/bin/env bash
set -euo pipefail

FIRMWARE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${TMPDIR:-/tmp}/reeflow_phase8_lighting_scheduler_integration_tests"
mkdir -p "$BUILD_DIR"

g++ -std=c++17 -Wall -Wextra -Werror \
  -I"$FIRMWARE_DIR/include" \
  -I"$FIRMWARE_DIR/src" \
  -I"$FIRMWARE_DIR/test" \
  "$FIRMWARE_DIR/test/integration/test_lighting_scheduler_integration.cpp" \
  "$FIRMWARE_DIR/src/app/core_app.cpp" \
  "$FIRMWARE_DIR/src/config/config_manager.cpp" \
  "$FIRMWARE_DIR/src/core/events/event_bus.cpp" \
  "$FIRMWARE_DIR/src/core/logging/logger.cpp" \
  "$FIRMWARE_DIR/src/core/platform/core_platform.cpp" \
  "$FIRMWARE_DIR/src/core/scheduler/task_scheduler.cpp" \
  "$FIRMWARE_DIR/src/core/state/system_state.cpp" \
  "$FIRMWARE_DIR/src/core/watchdog/watchdog_service.cpp" \
  "$FIRMWARE_DIR/src/storage/storage_events.cpp" \
  "$FIRMWARE_DIR/src/storage/storage_service.cpp" \
  "$FIRMWARE_DIR/src/storage/storage_write_policy.cpp" \
  "$FIRMWARE_DIR/src/modules/ato/ato_events.cpp" \
  "$FIRMWARE_DIR/src/modules/ato/ato_policy.cpp" \
  "$FIRMWARE_DIR/src/modules/ato/ato_service.cpp" \
  "$FIRMWARE_DIR/src/modules/lighting/lighting_events.cpp" \
  "$FIRMWARE_DIR/src/modules/lighting/lighting_policy.cpp" \
  "$FIRMWARE_DIR/src/modules/lighting/lighting_service.cpp" \
  "$FIRMWARE_DIR/src/modules/modes/mode_automation_gate.cpp" \
  "$FIRMWARE_DIR/src/modules/modes/mode_effects.cpp" \
  "$FIRMWARE_DIR/src/modules/modes/mode_events.cpp" \
  "$FIRMWARE_DIR/src/modules/modes/mode_policy.cpp" \
  "$FIRMWARE_DIR/src/modules/modes/mode_service.cpp" \
  "$FIRMWARE_DIR/src/modules/relays/relay_events.cpp" \
  "$FIRMWARE_DIR/src/modules/relays/relay_service.cpp" \
  "$FIRMWARE_DIR/src/modules/temperature/temperature_events.cpp" \
  "$FIRMWARE_DIR/src/modules/temperature/temperature_service.cpp" \
  "$FIRMWARE_DIR/src/modules/temperature/temperature_state_evaluator.cpp" \
  "$FIRMWARE_DIR/src/modules/water_level/water_level_calibration.cpp" \
  "$FIRMWARE_DIR/src/modules/water_level/water_level_events.cpp" \
  "$FIRMWARE_DIR/src/modules/water_level/water_level_service.cpp" \
  "$FIRMWARE_DIR/src/modules/water_level/water_level_state_evaluator.cpp" \
  -o "$BUILD_DIR/test_lighting_scheduler_integration"

"$BUILD_DIR/test_lighting_scheduler_integration"
