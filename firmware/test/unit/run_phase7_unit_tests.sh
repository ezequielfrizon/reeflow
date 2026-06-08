#!/usr/bin/env bash
set -euo pipefail

TEST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

bash "$TEST_DIR/run_phase7_mode_contract_tests.sh"
bash "$TEST_DIR/run_phase7_mode_policy_tests.sh"
bash "$TEST_DIR/run_phase7_mode_effects_tests.sh"
bash "$TEST_DIR/run_phase7_mode_service_tests.sh"
