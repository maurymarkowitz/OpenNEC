#!/usr/bin/env bash
set -euo pipefail

# Regression harness: builds selected backends and compares outputs across decks.
# Outputs and report are stored under test/regression_tests/.

ROOT_DIR=$(cd "$(dirname "$0")"/../.. && pwd)
REG_DIR="$ROOT_DIR/test/regression_tests"
mkdir -p "$REG_DIR"

# Timing setup
TIMING_CSV="$REG_DIR/timing.csv"
TIME_BIN="$(command -v /usr/bin/time || command -v time)"
if [ -z "$TIME_BIN" ]; then
  echo "Warning: time command not found; timing disabled" >&2
fi

# Detect platform
UNAME_S=$(uname -s || echo unknown)

# Select decks: default to top-level test/*.deck and test/speed_tests/*.deck
DECKS=()
while IFS= read -r -d '' f; do DECKS+=("$f"); done < <(find "$ROOT_DIR/test" -maxdepth 1 -name '*.deck' -print0 | sort -z)
if [ -d "$ROOT_DIR/test/speed_tests" ]; then
  while IFS= read -r -d '' f; do DECKS+=("$f"); done < <(find "$ROOT_DIR/test/speed_tests" -maxdepth 1 -name '*.deck' -print0 | sort -z)
fi

# Allow user-specified decks via args
if [ "$#" -gt 0 ]; then
  DECKS=("$@")
fi

if [ ${#DECKS[@]} -eq 0 ]; then
  echo "No decks selected. Exiting." >&2
  exit 1
fi

# Figure out available backends
BACKENDS=()

# Accelerate (macOS)
if [ "$UNAME_S" = "Darwin" ]; then
  BACKENDS+=(accelerate)
fi

# Original (builtin) always available
BACKENDS+=(original)

# OpenBLAS: only include if pkg-config or Homebrew (arm64) is present
OPENBLAS_OK=0
if pkg-config --exists openblas 2>/dev/null; then
  OPENBLAS_OK=1
else
  if [ "$UNAME_S" = "Darwin" ]; then
    # Prefer /opt/homebrew (Apple Silicon) over /usr/local (Intel)
    if [ -d "/opt/homebrew/opt/openblas" ]; then
      OPENBLAS_OK=1
    fi
  fi
fi
if [ "$OPENBLAS_OK" = "1" ]; then
  BACKENDS+=(openblas)
fi

# Reference BLAS/LAPACK via pkg-config (Linux)
BLAS_OK=0
if pkg-config --exists blas lapack 2>/dev/null; then
  BLAS_OK=1
fi
if [ "$BLAS_OK" = "1" ]; then
  BACKENDS+=(blas)
fi

# Intel MKL (Linux/WSL) if MKL_ROOT is set and libs exist
if [ -n "${MKL_ROOT:-}" ] && [ -d "$MKL_ROOT/lib" ]; then
  BACKENDS+=(mkl)
fi

# Reference backend: Accelerate on macOS, else original
REF_BACKEND="original"
if [ "$UNAME_S" = "Darwin" ]; then
  REF_BACKEND="accelerate"
fi

# Helpers
build_backend() {
  local backend="$1"
  echo "==> Building backend: $backend"
  ( set -x; make -C "$ROOT_DIR" clean >/dev/null ) || true
  if ! ( set -x; make -C "$ROOT_DIR" BACKEND="$backend" >/dev/null ); then
    echo "Build failed for backend $backend; skipping." >&2
    return 1
  fi
  return 0
}

run_deck() {
  local backend="$1"; shift
  local deck="$1"; shift
  local deck_base
  deck_base=$(basename "$deck")
  local out_file="$REG_DIR/${deck_base}.${backend}.out"
  echo "    Running $deck (backend=$backend) -> $(basename "$out_file")"
  if [ -n "$TIME_BIN" ]; then
    local ttmp="$REG_DIR/.time_${deck_base}.${backend}.txt"
    if ! "$TIME_BIN" -p "$ROOT_DIR/onec" "$deck" >"$out_file" 2>"$ttmp"; then
      echo "    Execution failed for $deck (backend=$backend)" >&2
      echo "$deck_base,$backend,NA,NA,NA" >>"$TIMING_CSV"
      rm -f "$ttmp"
      return 1
    fi
    local real user sys
    real=$(awk '/^real /{print $2}' "$ttmp")
    user=$(awk '/^user /{print $2}' "$ttmp")
    sys=$(awk '/^sys /{print $2}' "$ttmp")
    echo "$deck_base,$backend,$real,$user,$sys" >>"$TIMING_CSV"
    rm -f "$ttmp"
  else
    if ! "$ROOT_DIR/onec" "$deck" >"$out_file" 2>&1; then
      echo "    Execution failed for $deck (backend=$backend)" >&2
      return 1
    fi
    echo "$deck_base,$backend,NA,NA,NA" >>"$TIMING_CSV"
  fi
}

normalize() {
  # Normalize output for diff: remove non-deterministic timing lines.
  # Writes to stdout.
  sed -E '/TOTAL RUN TIME:/d' "$1"
}

compare_outputs() {
  local deck="$1"; shift
  local ref_backend="$1"; shift
  local other_backend="$1"; shift
  local deck_base
  deck_base=$(basename "$deck")
  local ref_out="$REG_DIR/${deck_base}.${ref_backend}.out"
  local oth_out="$REG_DIR/${deck_base}.${other_backend}.out"
  local diff_file="$REG_DIR/${deck_base}.${ref_backend}_vs_${other_backend}.diff"
  if [ ! -f "$ref_out" ] || [ ! -f "$oth_out" ]; then
    echo "MISSING OUTPUT: $deck ($ref_backend or $other_backend)" >>"$REG_DIR/report.txt"
    return 1
  fi
  if ! diff -u <(normalize "$ref_out") <(normalize "$oth_out") >"$diff_file"; then
    echo "DIFF: $deck ($ref_backend vs $other_backend) -> $(basename "$diff_file")" >>"$REG_DIR/report.txt"
    return 1
  else
    rm -f "$diff_file"
    echo "OK: $deck ($ref_backend vs $other_backend)" >>"$REG_DIR/report.txt"
  fi
}

# Main
echo "Backends: ${BACKENDS[*]}"
echo "Reference: $REF_BACKEND"
echo "Decks:"
for d in "${DECKS[@]}"; do echo "  - $d"; done

echo "" >"$REG_DIR/report.txt"
echo "deck,backend,real,user,sys" >"$TIMING_CSV"

for backend in "${BACKENDS[@]}"; do
  build_backend "$backend" || continue
  for deck in "${DECKS[@]}"; do
    # Skip error tests
    case "$deck" in
      */error_tests/*) continue;;
    esac
    run_deck "$backend" "$deck" || true
  done
done

# Comparisons
for backend in "${BACKENDS[@]}"; do
  [ "$backend" = "$REF_BACKEND" ] && continue
  for deck in "${DECKS[@]}"; do
    case "$deck" in
      */error_tests/*) continue;;
    esac
    compare_outputs "$deck" "$REF_BACKEND" "$backend" || true
  done
done

echo "Summary:"
cat "$REG_DIR/report.txt"

echo "\nArtifacts in: $REG_DIR"

# Timing summary per backend
if [ -s "$TIMING_CSV" ]; then
  summary="$REG_DIR/timing_summary.txt"
  : >"$summary"
  for b in "${BACKENDS[@]}"; do
    awk -F, -v b="$b" 'NR>1 && $2==b && $3!="NA" {sum+=$3; n++} END{printf "%s: %d decks, avg real %.4fs\n", b, (n?n:0), (n?sum/n:0)}' "$TIMING_CSV" >>"$summary"
  done
  echo "\nTiming summary:" >>"$summary"
  echo "(per-deck real seconds; averages exclude NA)" >>"$summary"
fi
