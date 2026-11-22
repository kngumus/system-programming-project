
#!/usr/bin/env bash

set -euo pipefail

cd "$(dirname "$0")/.."

OUTPUT=$(mktemp)
WATCHDOG_PID=""

cleanup() {
    if [[ -n "$WATCHDOG_PID" ]]; then
        kill -TERM "$WATCHDOG_PID" 2>/dev/null || true
        wait "$WATCHDOG_PID" 2>/dev/null || true
    fi
    rm -f "$OUTPUT"
}
trap cleanup EXIT

echo "Running Test 3: Maximum Restart Limit..."

./watchdog > "$OUTPUT" 2>&1 &
WATCHDOG_PID=$!

sleep 2

# Worker 1 should start once initially, then restart 3 times.
for ATTEMPT in 1 2 3 4; do

    # Find the most recently started worker 1 PID.
    WORKER_PID=$(sed -n \
        's/.*Started worker 1 with PID \([0-9]*\).*/\1/p' \
        "$OUTPUT" | tail -n 1)

    if [[ -z "$WORKER_PID" ]]; then
        echo "FAIL: Could not find worker 1 PID"
        cat "$OUTPUT"
        exit 1
    fi

    echo "Terminating worker 1 (attempt $ATTEMPT)..."

    kill -KILL "$WORKER_PID"

    sleep 1
done

# Allow time for any unexpected additional restart.
sleep 2

START_COUNT=$(grep -c "Started worker 1 with PID" "$OUTPUT" || true)

echo "Total worker 1 starts: $START_COUNT"

if [[ "$START_COUNT" -eq 4 ]]; then
    echo "PASS: Restart limit enforced (3 restarts)"
else
    echo "FAIL: Expected 4 total starts, got $START_COUNT"
    cat "$OUTPUT"
    exit 1
fi
