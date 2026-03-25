#!/usr/bin/env bash
set -euo pipefail
# Ignore SIGTRAP (signal 5) so that onec crashes (Trace/BPT trap) do not propagate
# up through bash's subshell chain and kill this script. The onec subshell below
# resets it back to SIG_DFL so onec itself still traps normally on internal faults.
trap '' 5

# Usage: ./examples/check_all_nec_files.sh
# This script runs onec on each examples subdirectory with -r,
# captures the stdout/stderr, and generates a summary of warnings/errors.

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
EXAMPLES_DIR="$ROOT_DIR/examples"
LOG_FILE="$EXAMPLES_DIR/check_all_nec_files_log.txt"
ISSUES_FILE="$EXAMPLES_DIR/check_all_nec_files_issues.txt"
: > "$LOG_FILE"
: > "$ISSUES_FILE"

# Prefer local build first, then fallback to system onec
ONEC_BIN="$ROOT_DIR/onec"
if [[ ! -x "$ONEC_BIN" ]]; then
  ONEC_BIN="$(command -v onec 2>/dev/null || true)"
fi
if [[ -z "$ONEC_BIN" ]] || [[ ! -x "$ONEC_BIN" ]]; then
  echo "ERROR: onec binary not found (local or in PATH)." >&2
  exit 1
fi

# Report which onec is used
echo "Using onec binary: $ONEC_BIN" | tee -a "$LOG_FILE"
echo "onec run log generated: $LOG_FILE"
echo "issue summary generated: $ISSUES_FILE"

echo "=== onec recursing through examples at $(date) ===" >> "$LOG_FILE"

total_folders=0

for folder in "$EXAMPLES_DIR"/*; do
  if [[ -d "$folder" ]]; then
    folder_name=$(basename "$folder")
    # Skip cocoaNEC folder entirely (uses XML-based .nec format)
    folder_lower=$(printf '%s' "$folder_name" | tr '[:upper:]' '[:lower:]')
    if [[ "$folder_lower" == *cocoa* ]]; then
      printf 'Skipping %s (CocoaNEC XML format)...\n' "$folder_name" | tee -a "$LOG_FILE"
      echo "SKIPPED: $folder_name (CocoaNEC folder)" >> "$ISSUES_FILE"
      continue
    fi
    total_folders=$((total_folders + 1))
    # Collect .nec/.deck files recursively, skipping CocoaNEC .nec files that are actually XML.
    # Some CocoaNEC export paths use .nec extension for XML content (not NEC format), which would
    # break OpenNEC. We detect and skip those in the script so batch runs stay clean.
    valid_files=()
    while IFS= read -r -d '' candidate; do
      cand_lower=$(printf '%s' "$candidate" | tr '[:upper:]' '[:lower:]')
      if [[ "$cand_lower" == *.nec ]]; then
        # Detect XML headers in .nec files
        if head -c 5 "$candidate" | grep -iq '^\?xml'; then
          # silently skip CocoaNEC XML-wrapped .nec files
          continue
        fi
      fi
      valid_files+=("$candidate")
    done < <(find "$folder" -type f \( -iname '*.nec' -o -iname '*.deck' \) -print0)

    nec_count=${#valid_files[@]}
    folder_issues=0

    printf '\n=== Running onec -r on %s (%d .nec/.deck files) ===\n' "$folder_name" "$nec_count" | tee -a "$LOG_FILE"

    run_output="$EXAMPLES_DIR/onec_${folder_name// /_}.out"
    {
      if [[ $nec_count -gt 0 ]]; then
        set +e
        # Use relative path for onec invocation so skip messages are partial paths.
        relative_folder="${folder#$ROOT_DIR/}"
        (trap - 5; cd "$ROOT_DIR" && "$ONEC_BIN" -r --skip-large "$relative_folder")
        status=$?
        set -e
        if [[ $status -eq 0 ]]; then
          :  # success, no status message
        elif [[ $status -eq 1 ]]; then
          : # this is fine, onec alrrady reported it
        elif [[ $status -eq 133 ]]; then
          echo "--- SKIPPING $folder_name: onec crashed with status 133 (Trace/BPT trap) ---"
          echo "SKIPPED/CRASH: $folder_name" >> "$ISSUES_FILE"
          folder_issues=$((folder_issues + 1))
        else
          echo "--- WARNING: onec returned status $status for $folder_name ---"
          echo "SKIPPED/ERROR: $folder_name (status $status)" >> "$ISSUES_FILE"
          folder_issues=$((folder_issues + 1))
        fi
      else
        echo "No valid NEC/DECK files to process in $folder"
      fi
    } 2>&1 | tee -a "$run_output" || true

    cat "$run_output" >> "$LOG_FILE"

    # collect warnings / errors for this folder
    grep -i -E "warning|error" "$run_output" > /tmp/onec_${folder_name// /_}_issues.tmp || true
    if [[ -s /tmp/onec_${folder_name// /_}_issues.tmp ]]; then
      folder_issues=$((folder_issues + 1))
      echo "=== $folder_name issues ===" >> "$ISSUES_FILE"
      sed -n '1,20p' /tmp/onec_${folder_name// /_}_issues.tmp >> "$ISSUES_FILE"
      echo "" >> "$ISSUES_FILE"
    fi

    # check any generated .out files (including from onec itself) for zero bytes
    find "$folder" -type f -name '*.out' -print0 | while IFS= read -r -d '' out_file; do
      if [[ ! -s "$out_file" ]]; then
        folder_issues=$((folder_issues + 1))
        rel_out_file="${out_file#$EXAMPLES_DIR/}"
        echo "ZERO-BYTE OUT: $rel_out_file" >> "$ISSUES_FILE"
      fi
    done

    # report per-folder status
    printf '=== Finished %s: %d file(s), %d issue(s) in folder ===\n' "$folder_name" "$nec_count" "$folder_issues" | tee -a "$LOG_FILE"

    # keep script local run output logs, but we'll remove .out files at end
  fi
done

# also inspect .out files created in root examples and subdirs that are not nested under checked path
find "$EXAMPLES_DIR" -type f -name '*.out' -print0 | while IFS= read -r -d '' out_file; do
  if [[ ! -s "$out_file" ]]; then
    rel_out_file="${out_file#$EXAMPLES_DIR/}"
    echo "ZERO-BYTE OUT: $rel_out_file" >> "$ISSUES_FILE"
  fi
done

# Remove .out files created by these test runs (including folder runs and script-created run_output)
#find "$EXAMPLES_DIR" -type f -name '*.out' -delete
#rm -f "$EXAMPLES_DIR"/onec_*_issues.tmp

echo "Done. Processed $total_folders folders." | tee -a "$LOG_FILE"
