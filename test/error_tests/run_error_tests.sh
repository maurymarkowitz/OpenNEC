#!/usr/bin/env bash
# Runs all error test decks and verifies expected error messages are printed.
# Produces a PASS/FAIL summary.

set -uo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$REPO_ROOT"

PASSED=0
FAILED=0

# Return expected regex for a given error test deck name
expected_for() {
  case "$1" in
    invalid_load_type)
      echo "IMPROPER LOAD TYPE CHOSEN";
      ;;
    load_no_matching_tag)
      echo "LOADING DATA CARD ERROR, NO SEGMENT HAS AN ITAG =";
      ;;
    segment_below_ground)
      echo "Geometry card on line .* has an unknown mnemonic, 'GN'";
      ;;
    segment_in_ground_plane)
      echo "Geometry card on line .* has an unknown mnemonic, 'GN'";
      ;;
    segment_data_error)
      echo "GW with a zero radius";
      ;;
    ld_bad_tags)
      echo "DATA FAULT ON LOADING CARD";
      ;;
    gn_radial_sommerfeld)
      echo "RADIAL WIRE G.S. APPROXIMATION MAY NOT BE USED WITH SOMMERFELD GROUND OPTION";
      ;;
    wg_unsupported)
      echo "WG CARD NOT SUPPORTED";
      ;;
    no_ge_card)
      echo "Failed to initialize calculation defaults \(no valid geometry\)";
      ;;
    *)
      echo "";
      ;;
  esac
}

for deck in test/error_tests/*.deck; do
  name="$(basename "$deck" .deck)"
  expected_regex="$(expected_for "$name")"
  if [[ -z "$expected_regex" ]]; then
    echo "SKIP: $name (no expected message configured)"
    continue
  fi
  echo "Testing $name"
  tmpout="/tmp/error_test_${name}.out"
  ./onec "$deck" >"$tmpout" 2>&1
  status=$?
  if grep -E "$expected_regex" "$tmpout" >/dev/null; then
    echo "PASS: $name (found expected error)"
    PASSED=$((PASSED+1))
  else
    echo "FAIL: $name (missing expected error)"
    echo "Expected: $expected_regex"
    echo "--- Output tail ---"
    tail -50 "$tmpout" || true
    echo "-------------------"
    FAILED=$((FAILED+1))
  fi
done

echo "Summary: PASSED=$PASSED, FAILED=$FAILED"
exit "$FAILED"
