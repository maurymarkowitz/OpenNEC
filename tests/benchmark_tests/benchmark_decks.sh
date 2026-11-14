#!/bin/zsh

# Run the two benchmark decks N times and report wall-clock and NEC runtimes.
# Usage: tests/benchmark_decks.sh [path/to/onec]

set -u

SCRIPT_DIR=${0:A:h}
PROJECT_DIR=${SCRIPT_DIR:h:h}
ONEC=${1:-${OPENNEC_BIN:-$PROJECT_DIR/onec}}
RUNS=5

if [[ ! -x "$ONEC" ]]; then
  print -u2 "Executable not found or not executable: $ONEC"
  print -u2 "Build onec first, or pass its path as the first argument."
  exit 1
fi

TMP_DIR=$(mktemp -d "${TMPDIR:-/tmp}/opennec-benchmark.XXXXXX") || exit 1
trap 'rm -rf "$TMP_DIR"' EXIT INT TERM

TIMEFMT='wall=%E user=%U sys=%S cpu=%P'

benchmark_deck() {
  local name=$1
  local deck=$2
  local trial report timing elapsed report_runtime fill_time factor_time rc
  local -a results
  results=()

  print "\n$name"
  for trial in {1..$RUNS}; do
    report="$TMP_DIR/${name}-${trial}.out"
    timing="$TMP_DIR/${name}-${trial}.time"

    { time "$ONEC" -o "$report" "$deck" } 2> "$timing"
    rc=$?
    if (( rc != 0 )); then
      cat "$timing" >&2
      print -u2 "Run failed (exit $rc): $deck"
      return $rc
    fi

    elapsed=$(awk -F'[ =]' '/^wall=/{sub(/s$/, "", $2); print $2; exit}' "$timing")
    report_runtime=$(awk '/RUN TIME =/{value=$NF; gsub(/\r/, "", value)} END{print value}' "$report")
    fill_time=$(awk '/FILL=/{sum += $2} END{if (NR > 0) printf "%.3f", sum}' "$report")
    factor_time=$(awk '/FILL=/{sum += $5} END{if (NR > 0) printf "%.3f", sum}' "$report")
    if [[ -z "$elapsed" || -z "$report_runtime" || -z "$fill_time" || -z "$factor_time" ]]; then
      cat "$timing" >&2
      print -u2 "Could not read timing for trial $trial: $deck"
      return 1
    fi

    print "  trial $trial: wall ${elapsed}s; report ${report_runtime}s; fill ${fill_time}s; factor ${factor_time}s"
    results+=("$elapsed $trial $report_runtime $fill_time $factor_time")
  done

  print "  best two wall times:"
  printf '%s\n' "${results[@]}" | sort -n -k1,1 | head -2 | \
    awk '{printf "    trial %s: wall %ss; report %ss; fill %ss; factor %ss\n", $2, $1, $3, $4, $5}'
}

benchmark_deck whip_antenna_8ft_groundplane "$SCRIPT_DIR/whip_antenna_8ft_groundplane.nec" || exit $?
benchmark_deck LogYag220-520-15dbi "$SCRIPT_DIR/LogYag220-520-15dbi.nec" || exit $?
