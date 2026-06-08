#!/usr/bin/env bash
set -euo pipefail

TEST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

bash "$TEST_DIR/run_phase8_lighting_contract_tests.sh"
bash "$TEST_DIR/run_phase8_lighting_pwm_controller_tests.sh"
bash "$TEST_DIR/run_phase8_lighting_policy_tests.sh"
bash "$TEST_DIR/run_phase8_lighting_service_tests.sh"
