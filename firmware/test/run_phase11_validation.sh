#!/usr/bin/env bash
set -euo pipefail

FIRMWARE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TEST_DIR="$FIRMWARE_DIR/test"

bash "$TEST_DIR/integration/run_phase11_integration_tests.sh"
bash "$TEST_DIR/integration/run_phase11_scope_review.sh"

pio run -d "$FIRMWARE_DIR"

echo "Pendente para Hardware Validation: broker Mosquitto real, rede local real, ESP32 real, monitor Serial real, credenciais reais, queda real de Wi-Fi, latencia real, antena, caixa eletrica, aquario e bancada real."
