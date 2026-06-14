#!/usr/bin/env bash
set -euo pipefail

FIRMWARE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${TMPDIR:-/tmp}/reeflow_phase10_network_unit_tests"
mkdir -p "$BUILD_DIR"

run_test() {
  local test_name="$1"
  local output="$BUILD_DIR/${test_name%.cpp}"

  g++ -std=c++17 -Wall -Wextra -Werror \
    -I"$FIRMWARE_DIR/include" \
    -I"$FIRMWARE_DIR/src" \
    -I"$FIRMWARE_DIR/test" \
    "$FIRMWARE_DIR/test/unit/$test_name" \
    "$FIRMWARE_DIR/src/config/config_manager.cpp" \
    "$FIRMWARE_DIR/src/core/events/event_bus.cpp" \
    "$FIRMWARE_DIR/src/core/state/system_state.cpp" \
    "$FIRMWARE_DIR/src/network/network_events.cpp" \
    "$FIRMWARE_DIR/src/network/network_heartbeat.cpp" \
    "$FIRMWARE_DIR/src/network/network_status_service.cpp" \
    "$FIRMWARE_DIR/src/network/ntp_service.cpp" \
    "$FIRMWARE_DIR/src/network/wifi_service.cpp" \
    -o "$output"

  "$output"
}

run_test test_network_contracts.cpp
run_test test_wifi_service.cpp
run_test test_network_reconnect_policy.cpp
run_test test_ntp_service.cpp
run_test test_network_status_service.cpp
run_test test_network_heartbeat.cpp
