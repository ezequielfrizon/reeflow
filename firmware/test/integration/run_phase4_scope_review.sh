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
  "Phase 4 functional water-level code must not depend on Phase 1 diagnostics." \
  "$FIRMWARE_DIR/include/modules/water_level" \
  "$FIRMWARE_DIR/src/modules/water_level" \
  "$FIRMWARE_DIR/src/drivers/sensors/vl6180x" \
  "$FIRMWARE_DIR/src/drivers/i2c/i2c_bus.h" \
  "$FIRMWARE_DIR/src/drivers/i2c/i2c_bus_arduino.cpp"

check_absent \
  "test/fakes|fakes/" \
  "Real firmware code must not include test fakes or mocks." \
  "$FIRMWARE_DIR/include" \
  "$FIRMWARE_DIR/src"

check_absent \
  "ATO_START|ATO_STOP|ATO_TIMEOUT|ATO_SENSOR_OFFLINE|ATO_RECOVERED" \
  "Phase 4 must not create ATO events." \
  "$FIRMWARE_DIR/include/modules/water_level" \
  "$FIRMWARE_DIR/src/modules/water_level" \
  "$FIRMWARE_DIR/src/app/core_app.h" \
  "$FIRMWARE_DIR/src/app/core_app.cpp" \
  "$FIRMWARE_DIR/src/app/core_app_arduino.cpp"

check_absent \
  "MQTT|mqtt|WiFi|NVS|Preferences|AlertManager|Alert Manager|Lighting|OperationalMode" \
  "Phase 4 must not initialize future-phase capabilities." \
  "$FIRMWARE_DIR/include/modules/water_level" \
  "$FIRMWARE_DIR/src/modules/water_level" \
  "$FIRMWARE_DIR/src/drivers/sensors/vl6180x" \
  "$FIRMWARE_DIR/src/drivers/i2c/i2c_bus.h" \
  "$FIRMWARE_DIR/src/drivers/i2c/i2c_bus_arduino.cpp" \
  "$FIRMWARE_DIR/src/app/core_app.h" \
  "$FIRMWARE_DIR/src/app/core_app.cpp" \
  "$FIRMWARE_DIR/src/app/core_app_arduino.cpp"

check_absent \
  "Relay|Relays|relay|relays|pump|bomba|atoPump" \
  "Phase 4 must not control relays, relay 3, or the ATO pump." \
  "$FIRMWARE_DIR/include/modules/water_level" \
  "$FIRMWARE_DIR/src/modules/water_level" \
  "$FIRMWARE_DIR/src/drivers/sensors/vl6180x" \
  "$FIRMWARE_DIR/src/drivers/i2c/i2c_bus.h" \
  "$FIRMWARE_DIR/src/drivers/i2c/i2c_bus_arduino.cpp" \
  "$FIRMWARE_DIR/src/app/core_app.h" \
  "$FIRMWARE_DIR/src/app/core_app.cpp" \
  "$FIRMWARE_DIR/src/app/core_app_arduino.cpp"

echo "Pendente para Hardware Validation: VL6180X real, barramento I2C real, endereco fisico, estabilidade e comportamento real do nivel."
