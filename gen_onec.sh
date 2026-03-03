#!/usr/bin/env bash
# gen_onec.sh — Run onec on every .nec file in a directory tree and
# mirror the results under test/onec/<relative-path>.
#
# Usage:
#   ./gen_onec.sh <input-directory>
#
# Example:
#   ./gen_onec.sh "test/DL5SAY collection"
#
# For each .nec file found (case-insensitive) in <input-directory>, the script:
#   - Creates a matching subdirectory under test/onec/<rel-to-test>/
#   - Runs:  ./onec -i "<input>.nec" -o "<output>.out"
#   - Reports per-file pass/fail and a final summary.
#
# If the output root for this run already exists, its contents are deleted first.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TEST_DIR="$SCRIPT_DIR/test"
ONEC="$SCRIPT_DIR/onec"
ONEC_DIR="$TEST_DIR/onec"

# ── Argument handling ────────────────────────────────────────────────────────

if [[ $# -lt 1 ]]; then
    echo "Usage: $0 <input-directory>" >&2
    exit 1
fi

if [[ ! -x "$ONEC" ]]; then
    echo "error: onec not found or not executable at: $ONEC" >&2
    exit 1
fi

INPUT_DIR="$(cd "$1" && pwd)"   # resolve to absolute path

# ── Compute output root ──────────────────────────────────────────────────────
# Strip TEST_DIR prefix to get the relative portion, e.g.
#   /…/test/DL5SAY collection  →  DL5SAY collection
#   /…/test/4nec2 examples      →  4nec2 examples

if [[ "$INPUT_DIR" == "$TEST_DIR/"* ]]; then
    REL_PATH="${INPUT_DIR#"$TEST_DIR/"}"
else
    # Input is outside test/ — use just the basename
    REL_PATH="$(basename "$INPUT_DIR")"
fi

OUTPUT_ROOT="$ONEC_DIR/$REL_PATH"

echo "Input directory : $INPUT_DIR"
echo "Output root     : $OUTPUT_ROOT"
echo ""

# ── Clear output root if it exists ──────────────────────────────────────────

if [[ -d "$OUTPUT_ROOT" ]]; then
    echo "Clearing existing output directory…"
    rm -rf "$OUTPUT_ROOT"
fi

mkdir -p "$OUTPUT_ROOT"

# ── Process files ────────────────────────────────────────────────────────────

count_ok=0
count_fail=0

while IFS= read -r -d $'\0' nec_file; do
    # Path of this file relative to INPUT_DIR
    rel_file="${nec_file#"$INPUT_DIR/"}"
    rel_dir="$(dirname "$rel_file")"
    base="$(basename "$nec_file")"

    # Output filename: same stem, .out extension
    stem="${base%.*}"
    out_name="${stem}.out"

    # Build output directory (may be nested)
    if [[ "$rel_dir" == "." ]]; then
        out_dir="$OUTPUT_ROOT"
    else
        out_dir="$OUTPUT_ROOT/$rel_dir"
    fi

    mkdir -p "$out_dir"
    out_file="$out_dir/$out_name"

    # onec handles long paths and spaces natively; capture stderr, use exit code.
    tmp_err="$(mktemp /tmp/onec_err_XXXXXX)"
    if "$ONEC" -i "$nec_file" -o "$out_file" 2>"$tmp_err"; then
        printf "  OK      %s\n" "$rel_file"
        (( count_ok++ )) || true
    else
        err="$(head -1 "$tmp_err")"
        printf "  FAILED  %s  (%s)\n" "$rel_file" "$err" >&2
        rm -f "$out_file"
        (( count_fail++ )) || true
    fi
    rm -f "$tmp_err"

done < <(find "$INPUT_DIR" -type f \( -iname "*.nec" \) -print0 | sort -z)

# ── Summary ──────────────────────────────────────────────────────────────────

echo ""
echo "────────────────────────────────────────"
echo "  $count_ok succeeded,  $count_fail failed"
echo "  Output: $OUTPUT_ROOT"
echo "────────────────────────────────────────"

if (( count_fail > 0 )); then
    exit 1
fi
