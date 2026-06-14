#!/usr/bin/env bash
set -euo pipefail

TEST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

bash "$TEST_DIR/run_phase9_storage_contract_tests.sh"
bash "$TEST_DIR/run_phase9_storage_backend_tests.sh"
bash "$TEST_DIR/run_phase9_storage_write_policy_tests.sh"
bash "$TEST_DIR/run_phase9_storage_service_tests.sh"
bash "$TEST_DIR/run_phase9_storage_domains_tests.sh"
