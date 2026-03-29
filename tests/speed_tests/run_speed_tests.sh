#!/usr/bin/env bash
set -euo pipefail

# Speed test runner: builds both backends, times runs for a few decks,
# and writes results to CSV in test/speed_tests.

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
cd "$REPO_DIR"

CSV="test/speed_tests/speed_results.csv"
TMP="test/speed_tests/.timing.tmp"
: > "$CSV"
echo "deck,segments,backend,real_seconds" >> "$CSV"

DECKS=(
  "test/speed_tests/speed_21.deck"
  "test/speed_tests/speed_101.deck"
  "test/speed_tests/speed_401.deck"
  "test/speed_tests/speed_1001.deck"
  "test/speed_tests/speed_4001.deck"
)

build_backend() {
  local backend="$1"
  make clean >/dev/null 2>&1
  make BACKEND="$backend" >/dev/null 2>&1
}

run_and_time() {
  local backend="$1"
  local deck="$2"
  # Run and capture /usr/bin/time -p (stderr)
  /usr/bin/time -p ./onec "$deck" 2> "$TMP" >/dev/null || true
  # Copy output to backend-specific file
  local base="${deck%.deck}"
  cp -f "$base.out" "${base}.${backend}.out"
  # Extract timing and segment count
  local real
  real="$(awk '/^real /{print $2}' "$TMP")"
  local segments
  segments="$(awk '/^GW[[:space:]]/{print $3; exit}' "$deck")"
  echo "$deck,$segments,$backend,$real" >> "$CSV"
}

# Accelerate backend
build_backend accelerate
for d in "${DECKS[@]}"; do
  run_and_time accelerate "$d"
done

# Custom backend
build_backend custom
for d in "${DECKS[@]}"; do
  run_and_time custom "$d"
done

# Clean up tmp
rm -f "$TMP"

# Print a quick summary
column -t -s, "$CSV" || cat "$CSV"
