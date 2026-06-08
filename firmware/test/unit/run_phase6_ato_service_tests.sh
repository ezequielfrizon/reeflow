#!/usr/bin/env bash
set -euo pipefail

FIRMWARE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${TMPDIR:-/tmp}/reeflow_phase6_ato_service_tests"
mkdir -p "$BUILD_DIR"

g++ -std=c++17 -Wall -Wextra -Werror \
  -I"$FIRMWARE_DIR/include" \
  -I"$FIRMWARE_DIR/src" \
  -I"$FIRMWARE_DIR/test" \
  "$FIRMWARE_DIR/test/unit/test_ato_service.cpp" \
  "$FIRMWARE_DIR/src/modules/ato/ato_service.cpp" \
  "$FIRMWARE_DIR/src/modules/ato/ato_policy.cpp" \
  "$FIRMWARE_DIR/src/modules/ato/ato_events.cpp" \
  "$FIRMWARE_DIR/src/modules/relays/relay_service.cpp" \
  "$FIRMWARE_DIR/src/modules/relays/relay_events.cpp" \
  "$FIRMWARE_DIR/src/config/config_manager.cpp" \
  "$FIRMWARE_DIR/src/core/events/event_bus.cpp" \
  "$FIRMWARE_DIR/src/core/state/system_state.cpp" \
  -o "$BUILD_DIR/test_ato_service"

"$BUILD_DIR/test_ato_service"
