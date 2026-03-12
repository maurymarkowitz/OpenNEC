#!/bin/sh
# run_tests.sh - execute all validation decks and display results

set -e

# assume this script lives under test/validation_tests
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
ONEC="$ROOT/onec"

printf "Running validation decks in %s\n" "$(pwd)"
for d in *.deck; do
  printf "\n=== %s ===\n" "$d"
  "$ONEC" -t -n "$d" 2>&1 || true
done
