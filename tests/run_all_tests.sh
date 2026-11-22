
#!/usr/bin/env bash

set -euo pipefail

cd "$(dirname "$0")/.."

echo "=================================="
echo "    WATCHDOG AUTOMATED TESTS"
echo "=================================="

echo "Building project..."
make

PASSED=0
FAILED=0
SKIPPED=0

TESTS=(
    test_startup.sh
    test_restart.sh
    test_restart_limit.sh
    test_sigusr1.sh
    test_shutdown.sh
    test_periodic.sh
)

for TEST in "${TESTS[@]}"; do
    echo ""
    echo "----------------------------------"
    echo "Executing: $TEST"

    if OUTPUT=$(bash "tests/$TEST" 2>&1); then
        echo "$OUTPUT"

        if grep -q "SKIP:" <<< "$OUTPUT"; then
            ((SKIPPED+=1))
        elif grep -q "PASS:" <<< "$OUTPUT"; then
            ((PASSED+=1))
        else
            echo "FAIL: No PASS result reported"
            ((FAILED+=1))
        fi
    else
        echo "$OUTPUT"
        ((FAILED+=1))
    fi
done

echo ""
echo "=================================="
echo "           TEST SUMMARY"
echo "=================================="
echo "Passed:  $PASSED"
echo "Failed:  $FAILED"
echo "Skipped: $SKIPPED"
echo "Total:   ${#TESTS[@]}"

if [[ "$FAILED" -gt 0 || "$SKIPPED" -gt 0 ]]; then
    exit 1
fi

echo "ALL TESTS PASSED!"
