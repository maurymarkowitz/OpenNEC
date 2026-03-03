#!/usr/bin/env bash
# gen_originals.sh — Run nec2c on every .nec file in a directory tree and
# mirror the results under test/originals/<relative-path>.
#
# Usage:
#   ./gen_originals.sh <input-directory>
#
# Example:
#   ./gen_originals.sh "test/DL5SAY collection"
#
# For each .nec file found (case-insensitive) in <input-directory>, the script:
#   - Creates a matching subdirectory under test/originals/<rel-to-test>/
#   - Runs:  test/nec2c -i"<input>.nec" -o"<output>.out"
#   - Reports per-file pass/fail and a final summary.
#
# If the output root for this run already exists, its contents are deleted first.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TEST_DIR="$SCRIPT_DIR/test"
NEC2C="$TEST_DIR/nec2c"
ORIGINALS_DIR="$TEST_DIR/originals"

# ── Argument handling ────────────────────────────────────────────────────────

if [[ $# -lt 1 ]]; then
    echo "Usage: $0 <input-directory>" >&2
    exit 1
fi

if [[ ! -x "$NEC2C" ]]; then
    echo "error: nec2c not found or not executable at: $NEC2C" >&2
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

OUTPUT_ROOT="$ORIGINALS_DIR/$REL_PATH"

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

    # nec2c has a short internal path buffer and fails silently on long absolute
    # input paths. Run from the file's own directory so only the basename is
    # needed. Output goes to a temp file (no spaces) and is moved afterwards.
    tmp_out="$(mktemp /tmp/nec2c_out_XXXXXX)"
    ( cd "$(dirname "$nec_file")" && "$NEC2C" -i"$(basename "$nec_file")" -o"$tmp_out" > /dev/null 2>&1 ) || true
    mv "$tmp_out" "$out_file" 2>/dev/null || true

    # A successful run produces several hundred lines; errors produce < 25 lines.
    out_lines=$(wc -l < "$out_file" 2>/dev/null || echo 0)
    if (( out_lines > 25 )); then
        printf "  OK      %s\n" "$rel_file"
        (( count_ok++ )) || true
    else
        printf "  FAILED  %s  (%d lines)\n" "$rel_file" "$out_lines" >&2
        (( count_fail++ )) || true
    fi

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
