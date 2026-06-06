#!/usr/bin/env bash
set -euo pipefail

FIRMWARE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${TMPDIR:-/tmp}/reeflow_phase3_ds18b20_adapter_tests"
mkdir -p "$BUILD_DIR"

g++ -std=c++17 -Wall -Wextra -Werror \
  -I"$FIRMWARE_DIR/include" \
  -I"$FIRMWARE_DIR/src" \
  -I"$FIRMWARE_DIR/test" \
  "$FIRMWARE_DIR/test/unit/test_ds18b20_temperature_sensor.cpp" \
  "$FIRMWARE_DIR/src/drivers/sensors/ds18b20/ds18b20_temperature_sensor.cpp" \
  -o "$BUILD_DIR/test_ds18b20_temperature_sensor"

"$BUILD_DIR/test_ds18b20_temperature_sensor"
