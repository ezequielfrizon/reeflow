#!/usr/bin/env bash
set -euo pipefail

TEST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

bash "$TEST_DIR/run_phase6_ato_contracts_tests.sh"
bash "$TEST_DIR/run_phase6_ato_policy_tests.sh"
bash "$TEST_DIR/run_phase6_ato_service_tests.sh"
