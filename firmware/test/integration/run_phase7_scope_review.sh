#!/usr/bin/env bash
set -euo pipefail

FIRMWARE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
REPO_DIR="$(cd "$FIRMWARE_DIR/.." && pwd)"

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
  "Preferences|NVS|MQTT|WiFi|wifi|AlertManager|Alert Manager|Cloud|cloud|push|notification|history|historico|histórico|remote|remoto|comando remoto|Serial" \
  "Phase 7 must not implement NVS, Wi-Fi, MQTT, Alert Manager, cloud, app, history, notifications, remote commands, or Serial commands." \
  "$FIRMWARE_DIR/include/modules/modes" \
  "$FIRMWARE_DIR/src/modules/modes" \
  "$FIRMWARE_DIR/src/app/core_app.h" \
  "$FIRMWARE_DIR/src/app/core_app.cpp" \
  "$FIRMWARE_DIR/src/app/core_app_arduino.cpp"

check_absent \
  "Lighting|lighting|PWM|sunrise|sunset|moonlight|acclimation|Aclim" \
  "Phase 7 must not implement lighting behavior." \
  "$FIRMWARE_DIR/include/modules/modes" \
  "$FIRMWARE_DIR/src/modules/modes" \
  "$FIRMWARE_DIR/src/app/core_app.h" \
  "$FIRMWARE_DIR/src/app/core_app.cpp"

check_absent \
  "digitalWrite|pinMode|GpioPort|GPIO|hardware_pins|GpioRelayController|RelayController|Diagnostic|diagnostic" \
  "Modes must not command GPIO directly, instantiate relay drivers, or depend on diagnostics." \
  "$FIRMWARE_DIR/include/modules/modes" \
  "$FIRMWARE_DIR/src/modules/modes"

check_absent \
  "updateTemperatureState|updateWaterLevelState|updateLightingState|updateAtoState|updateNetworkState|updateAlertsState|updateSystemHealthState" \
  "Mode module must not mutate unrelated System State blocks." \
  "$FIRMWARE_DIR/include/modules/modes" \
  "$FIRMWARE_DIR/src/modules/modes"

check_path_absent \
  "$FIRMWARE_DIR/src/drivers/modes" \
  "Phase 7 must not create mode-specific hardware drivers."

check_path_absent \
  "$FIRMWARE_DIR/include/drivers/modes" \
  "Phase 7 must not create mode-specific hardware driver includes."

if ! git -C "$REPO_DIR" diff --quiet -- architecture specs tasks/firmware-master-plan.md; then
  echo "Phase 7 must not alter architecture, specs, or firmware-master-plan." >&2
  git -C "$REPO_DIR" diff --name-only -- architecture specs tasks/firmware-master-plan.md >&2
  exit 1
fi

echo "Pendente para Hardware Validation: reles reais, bomba real, sensores reais, aquario, agua, TPA real, alimentacao real, manutencao real, corrente, ruido eletrico, resposta fisica, Serial real e seguranca de bancada."
