#!/usr/bin/env bash
set -euo pipefail

TEST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

"$TEST_DIR/run_phase5_relay_contract_tests.sh"
"$TEST_DIR/run_phase5_gpio_relay_controller_tests.sh"
"$TEST_DIR/run_phase5_relay_service_tests.sh"
