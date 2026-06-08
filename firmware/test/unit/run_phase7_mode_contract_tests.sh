#!/usr/bin/env bash
set -euo pipefail

FIRMWARE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${TMPDIR:-/tmp}/reeflow_phase7_mode_contract_tests"
mkdir -p "$BUILD_DIR"

g++ -std=c++17 -Wall -Wextra -Werror \
  -I"$FIRMWARE_DIR/include" \
  -I"$FIRMWARE_DIR/src" \
  -I"$FIRMWARE_DIR/test" \
  "$FIRMWARE_DIR/test/unit/test_mode_contracts.cpp" \
  "$FIRMWARE_DIR/src/modules/modes/mode_events.cpp" \
  "$FIRMWARE_DIR/src/config/config_manager.cpp" \
  "$FIRMWARE_DIR/src/core/events/event_bus.cpp" \
  -o "$BUILD_DIR/test_mode_contracts"

"$BUILD_DIR/test_mode_contracts"
