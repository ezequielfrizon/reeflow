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

check_path_absent() {
  local path="$1"
  local message="$2"

  if [ -e "$path" ]; then
    echo "$message" >&2
    find "$path" -maxdepth 3 -print >&2
    exit 1
  fi
}

check_absent \
  "test/fakes|fakes/" \
  "Real firmware code must not include test fakes or mocks." \
  "$FIRMWARE_DIR/include" \
  "$FIRMWARE_DIR/src"

check_absent \
  "MQTT|mqtt|WiFi|NVS|Preferences|AlertManager|Alert Manager|Cloud|cloud|push|notification|history|historico|histórico|remote|remoto|comando remoto" \
  "Phase 6 must not implement network, persistence, alert manager, cloud, app, history, notifications, or remote commands." \
  "$FIRMWARE_DIR/include/modules/ato" \
  "$FIRMWARE_DIR/src/modules/ato" \
  "$FIRMWARE_DIR/src/app/core_app.h" \
  "$FIRMWARE_DIR/src/app/core_app.cpp" \
  "$FIRMWARE_DIR/src/app/core_app_arduino.cpp"

check_absent \
  "OperationalMode|MODE_CHANGED|MODE_STARTED|MODE_FINISHED|FEEDING|TPA|MAINTENANCE|Lighting|lighting|PWM|sunrise|sunset|moonlight" \
  "Phase 6 must not implement operational modes or lighting." \
  "$FIRMWARE_DIR/include/modules/ato" \
  "$FIRMWARE_DIR/src/modules/ato" \
  "$FIRMWARE_DIR/src/app/core_app.h" \
  "$FIRMWARE_DIR/src/app/core_app.cpp"

check_absent \
  "digitalWrite|pinMode|gpio|Gpio|GPIO|hardware_pins|relay_contract" \
  "ATO must not command GPIO directly or depend on relay electrical contracts." \
  "$FIRMWARE_DIR/include/modules/ato" \
  "$FIRMWARE_DIR/src/modules/ato"

check_absent \
  "VL6180X|Vl6180x|I2C|i2c|readLevel\\(" \
  "ATO must consume waterLevel state, not read VL6180X or raw sensor data." \
  "$FIRMWARE_DIR/include/modules/ato" \
  "$FIRMWARE_DIR/src/modules/ato"

check_absent \
  "updateWaterLevelState|updateTemperatureState|updateLightingState|updateModesState|updateNetworkState|updateAlertsState|updateSystemHealthState" \
  "ATO module must not mutate unrelated System State blocks." \
  "$FIRMWARE_DIR/include/modules/ato" \
  "$FIRMWARE_DIR/src/modules/ato"

check_absent \
  "RelayController|GpioRelayController|RelayDiagnostic|Diagnostic" \
  "ATO module must command the pump through RelayService, not concrete relay drivers or diagnostics." \
  "$FIRMWARE_DIR/include/modules/ato" \
  "$FIRMWARE_DIR/src/modules/ato"

check_path_absent \
  "$FIRMWARE_DIR/src/drivers/ato" \
  "Phase 6 must not create a dedicated ATO pump driver."

check_path_absent \
  "$FIRMWARE_DIR/include/drivers/ato" \
  "Phase 6 must not create a dedicated ATO pump driver include path."

echo "Pendente para Hardware Validation: sensor fisico, bomba ATO real, modulo de rele real, agua real, vazao, sump, reservatorio, evaporacao, corrente, ruido eletrico, Serial real e seguranca de bancada."
