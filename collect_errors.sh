#!/bin/zsh
# collect_errors.sh
# Runs onec on all .nec/.NEC/.deck files under test/, excluding error_tests and
# speed_tests folders, and reports any unexpected errors/warnings on stderr.
#
# Expected (ignored) messages:
#   - "EX I1: excitation type N is not supported by OpenNEC."
#   - "Card N is an LD card with type N, which is not supported."
#
# Usage:  ./collect_errors.sh [test_dir]
#   Default test_dir: test

TESTDIR=${1:-test}
ONEC=./onec
TIMEOUT_SECS=30      # per-file timeout (seconds)
ERRORS_FOUND=0
TMPOUT=$(mktemp)
TMPERR=$(mktemp)
trap "rm -f $TMPOUT $TMPERR" EXIT

FILE_NUM=0
while IFS= read -r -d '' f; do
  FILE_NUM=$((FILE_NUM + 1))
  # Print current file to stderr so we can track progress (verbose mode)
  [[ -n "$VERBOSE" ]] && echo "[$FILE_NUM] $f" >&2

  # Run with per-file timeout (macOS lacks GNU timeout; use perl alarm)
  perl -e "alarm($TIMEOUT_SECS); exec @ARGV" -- "$ONEC" "$f" >"$TMPOUT" 2>"$TMPERR"
  local_status=$?

  if [[ $local_status -eq 14 ]]; then
    # SIGALRM exit code from perl alarm
    echo "=== $f ==="
    echo "  [TIMEOUT after ${TIMEOUT_SECS}s — possible hang]"
    echo ""
    ERRORS_FOUND=1
    continue
  fi

  errs=$(
    grep -v "is an EX card with type [67]," "$TMPERR" |
    grep -v "excitation type [67] is not supported" |
    grep -v "is an EX with type [67], which" |
    grep -v "is an LD card with type 7, which is not supported"
  )
  if [[ -n "$errs" ]]; then
    echo "=== $f ==="
    echo "$errs"
    echo ""
    ERRORS_FOUND=1
  fi
done < <(
  find "$TESTDIR" \
    -not \( -path "*error_tests*" -prune \) \
    -not \( -path "*speed_tests*" -prune \) \
    \( -iname "*.nec" -o -name "*.deck" \) \
    -print0 \
  | sort -z
)

if [[ $ERRORS_FOUND -eq 0 ]]; then
  echo "No unexpected errors found."
fi
