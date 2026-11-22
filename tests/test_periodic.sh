
#!/usr/bin/env bash

set -euo pipefail

cd "$(dirname "$0")/.."

TRACE=$(mktemp)
OUTPUT=$(mktemp)

cleanup() {
    rm -f "$TRACE" "$OUTPUT"
}
trap cleanup EXIT

echo "Running Test 6: Periodic Monitoring..."

if ! command -v strace >/dev/null 2>&1; then
    echo "SKIP: strace is not installed"
    exit 0
fi

# Run watchdog under strace for a limited time.
# SIGINT is sent to the traced process group on timeout.
timeout --signal=INT --kill-after=3s 5s \
    strace -f -e trace=signal -o "$TRACE" \
    ./watchdog > "$OUTPUT" 2>&1 || RESULT=$?

RESULT=${RESULT:-0}

# Timeout exit code 124 is expected.
if [[ "$RESULT" -ne 0 && "$RESULT" -ne 124 ]]; then
    echo "FAIL: Unexpected exit code: $RESULT"
    cat "$OUTPUT"
    exit 1
fi

ALARM_COUNT=$(grep -c -- '--- SIGALRM' "$TRACE" || true)

echo "SIGALRM events detected: $ALARM_COUNT"

if [[ "$ALARM_COUNT" -ge 3 ]]; then
    echo "PASS: Periodic SIGALRM timer is working"
else
    echo "FAIL: Expected at least 3 SIGALRM events"
    cat "$TRACE"
    exit 1
fi
