
#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

make >/dev/null

LOG_FILE=$(mktemp)
OUTPUT_FILE=$(mktemp)

# Save the current log size so we can inspect only new entries.
INITIAL_LOG_LINES=0

if [[ -f watchdog.log ]]; then
    INITIAL_LOG_LINES=$(wc -l < watchdog.log)
fi
WATCHDOG_PID=""

cleanup() {
    if [[ -n "$WATCHDOG_PID" ]]; then
        kill -INT "$WATCHDOG_PID" 2>/dev/null || true
        wait "$WATCHDOG_PID" 2>/dev/null || true
    fi

    rm -f "$LOG_FILE" "$OUTPUT_FILE"
}

trap cleanup EXIT

echo "Testing logging restart limit..."



./watchdog >"$OUTPUT_FILE" 2>&1 &
WATCHDOG_PID=$!

sleep 1

# Find Worker 1 from watchdog output.
get_worker_pid() {
    grep "Started worker 1 with PID" "$OUTPUT_FILE" |
        tail -n 1 |
        awk '{print $6}'
}

for attempt in 1 2 3 4; do
    WORKER_PID=$(get_worker_pid)

    if [[ -z "$WORKER_PID" ]]; then
        echo "FAIL: Worker 1 PID not found"
        exit 1
    fi

    if ! kill -0 "$WORKER_PID" 2>/dev/null; then
    echo "FAIL: Worker $WORKER_PID is not running"
    exit 1
    fi

    kill -TERM "$WORKER_PID"

    sleep 1
done

tail -n +"$((INITIAL_LOG_LINES + 1))" watchdog.log > "$LOG_FILE"
if grep -q "\[ERROR\] Worker 1 restart limit reached (3/3)" "$LOG_FILE"; then
    echo "PASS: Restart limit ERROR log found"
else
    echo "FAIL: Restart limit ERROR log missing"
    exit 1
fi

if [[ $(grep -c "Started worker 1 with PID" "$OUTPUT_FILE") -eq 4 ]]; then
    echo "PASS: Worker restarted exactly three times"
else
    echo "FAIL: Unexpected worker restart count"
    exit 1
fi

if grep -q "Restarting worker 1 (attempt 4/" "$LOG_FILE"; then
    echo "FAIL: Worker restarted beyond the limit"
    exit 1
fi

echo "PASS: No restart beyond the limit"
echo "Logging restart limit test PASSED!"
