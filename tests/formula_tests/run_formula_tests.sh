#!/usr/bin/env bash
set -euo pipefail

# Run baseline and all formula test decks, then compare outputs
REPO_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$REPO_ROOT"

BASE_DECK="examples/example5.deck"
BASE_OUT="examples/example5.out"

echo "Running baseline: $BASE_DECK"
./onec "$BASE_DECK" >/dev/null 2>&1 || { echo "Baseline run failed"; exit 1; }

FAILS=0
PASSED=0

for d in test/formula_tests/*.deck; do
  name="$(basename "$d" .deck)"
  out="test/formula_tests/${name}.out"
  echo "Running test deck: $d"
  ./onec "$d" >/dev/null 2>&1 || { echo "Run failed: $d"; FAILS=$((FAILS+1)); continue; }
  # Normalize outputs: drop COMMENTS section, normalize signed zeros, drop run time lines
  normalize() {
    awk '
      BEGIN{skip=0}
      /---------------- COMMENTS ----------------/{skip=1; next}
      /---------------- GEOMETRY ----------------/{skip=0; print; next}
      /TOTAL RUN TIME:/{next}
      /DATA CARD No:/{next}
      {if(skip==1) next}
      {gsub(/-0\.0000/,"0.0000"); gsub(/  +/," "); print}
    '
  }
  if normalize < "$BASE_OUT" | normalize > /tmp/base_norm.out && normalize < "$out" | normalize > /tmp/test_norm.out && diff -u /tmp/base_norm.out /tmp/test_norm.out >/dev/null; then
    echo "PASS: $name"
    PASSED=$((PASSED+1))
  else
    echo "FAIL: $name (differs from baseline)"
    FAILS=$((FAILS+1))
    # show a short diff tail for quick inspection
    diff -u /tmp/base_norm.out /tmp/test_norm.out | tail -50 || true
  fi
done

echo "Summary: PASSED=$PASSED, FAILED=$FAILS"
exit $FAILS
