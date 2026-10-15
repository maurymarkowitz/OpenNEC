#!/usr/bin/env python3
import argparse
import math
import re
import subprocess
import tempfile
from pathlib import Path


CASES = {
    "rp_then_xq.nec": (3, 3),
    "repeated_fr_rp.nec": (2, 2),
    "tl_fr_sweep.nec": (13, 13),
}

PATTERN_ROW = re.compile(
    r"^\s*([+-]?\d+(?:\.\d+)?)\s+([+-]?\d+(?:\.\d+)?)\s+"
    r"[+-]?\d+(?:\.\d+)?\s+[+-]?\d+(?:\.\d+)?\s+"
    r"([+-]?\d+(?:\.\d+)?)\s+"
)


def run_deck(onec, deck, output):
    return subprocess.run(
        [str(onec), "-f", "original", "-o", str(output), str(deck)],
        capture_output=True,
        text=True,
    )


def final_pattern(report):
    marker = "RADIATION PATTERNS"
    if marker not in report:
        return []
    pattern = report.rsplit(marker, 1)[1]
    return [tuple(map(float, match.groups()))
            for line in pattern.splitlines()
            if (match := PATTERN_ROW.match(line))]


def patterns_match(actual, reference):
    if len(actual) != len(reference) or not actual:
        return False, f"sample counts differ ({len(actual)} versus {len(reference)})"
    for index, (sample, expected) in enumerate(zip(actual, reference)):
        if not all(math.isclose(value, wanted, abs_tol=0.02)
                   for value, wanted in zip(sample, expected)):
            return False, f"sample {index} differs ({sample} versus {expected})"
    return True, ""


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--onec", default="./onec")
    args = parser.parse_args()

    onec = Path(args.onec).resolve()
    decks = Path(__file__).parent
    failed = False

    with tempfile.TemporaryDirectory(prefix="onec-sequential-") as temporary:
        for name, expected in CASES.items():
            output = Path(temporary) / f"{Path(name).stem}.out"
            result = run_deck(onec, decks / name, output)
            if result.returncode != 0:
                print(f"FAIL {name}: onec exited {result.returncode}")
                print(result.stdout)
                print(result.stderr)
                failed = True
                continue

            report = output.read_text(errors="replace")
            actual = (
                len(re.findall(r"FREQUENCY=", report)),
                len(re.findall(r"RADIATION PATTERNS", report)),
            )
            if actual != expected:
                print(
                    f"FAIL {name}: expected {expected[0]} frequencies and "
                    f"{expected[1]} patterns, got {actual[0]} and {actual[1]}"
                )
                failed = True
            else:
                print(f"PASS {name}: {actual[0]} frequencies, {actual[1]} patterns")

        test_output = Path(temporary) / "tl_fr_sweep.out"
        reference_output = Path(temporary) / "tl_single_frequency_reference.out"
        reference_result = run_deck(
            onec, decks / "tl_single_frequency_reference.nec", reference_output)
        if reference_result.returncode != 0:
            print(
                "FAIL tl_single_frequency_reference.nec: "
                f"onec exited {reference_result.returncode}"
            )
            print(reference_result.stdout)
            print(reference_result.stderr)
            failed = True
        elif test_output.exists():
            matches, reason = patterns_match(
                final_pattern(test_output.read_text(errors="replace")),
                final_pattern(reference_output.read_text(errors="replace")),
            )
            if matches:
                print("PASS tl_fr_sweep.nec: final pattern matches clean reference")
            else:
                print(f"FAIL tl_fr_sweep.nec: final pattern mismatch: {reason}")
                failed = True

    return 1 if failed else 0


if __name__ == "__main__":
    raise SystemExit(main())
