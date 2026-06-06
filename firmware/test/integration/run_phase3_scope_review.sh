#!/usr/bin/env bash
set -euo pipefail

FIRMWARE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

check_absent() {
  local pattern="$1"
  local message="$2"
  shift 2

  if grep -RInE "$pattern" "$@" >/dev/null; then
    echo "$message" >&2
    grep -RInE "$pattern" "$@" >&2
    exit 1
  fi
}

check_absent \
  "diagnostic|diagnostics/" \
  "Phase 3 functional temperature code must not depend on Phase 1 diagnostics." \
  "$FIRMWARE_DIR/include/modules/temperature" \
  "$FIRMWARE_DIR/src/modules/temperature" \
  "$FIRMWARE_DIR/src/drivers/sensors/ds18b20"

check_absent \
  "test/fakes|fakes/" \
  "Real firmware code must not include test fakes or mocks." \
  "$FIRMWARE_DIR/include" \
  "$FIRMWARE_DIR/src"

check_absent \
  "MQTT|WiFi|NVS|Preferences|AlertManager|Alert Manager|Relay|Relays|ATO|Lighting|OperationalMode" \
  "Phase 3 must not initialize future-phase capabilities." \
  "$FIRMWARE_DIR/include/modules/temperature" \
  "$FIRMWARE_DIR/src/modules/temperature" \
  "$FIRMWARE_DIR/src/drivers/sensors/ds18b20" \
  "$FIRMWARE_DIR/src/app/core_app.h" \
  "$FIRMWARE_DIR/src/app/core_app.cpp" \
  "$FIRMWARE_DIR/src/app/core_app_arduino.cpp"
