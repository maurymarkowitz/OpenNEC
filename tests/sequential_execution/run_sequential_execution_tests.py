#!/usr/bin/env python3
import argparse
import math
import re
import subprocess
import tempfile
from pathlib import Path


EXPECTED_VERSION = "2.4.0"

CASES = {
    "issue25_multiple_ex_diagnostic.nec": {"frequencies": 1, "patterns": 0, "antenna_blocks": 1, "source_rows": 2},
    "issue25_multiple_ex_regression.nec": {"frequencies": 3, "patterns": 0, "antenna_blocks": 3, "source_rows": 6},
    "issue25_manual_example_2.nec": {"frequencies": 4, "patterns": 0, "antenna_blocks": 4},
    "issue25_manual_example_2_voltage_source.nec": {"frequencies": 4, "patterns": 0, "antenna_blocks": 4},
    "rp_then_xq.nec": {"frequencies": 3, "patterns": 3},
    "repeated_fr_rp.nec": {"frequencies": 2, "patterns": 2},
    "tl_fr_sweep.nec": {"frequencies": 13, "patterns": 13},
    "multi_ex_fr_sweep.nec": {"frequencies": 3, "patterns": 0, "antenna_blocks": 3, "source_rows": 6},
    "ex2_second_xq_same_freq.nec": {"frequencies": 4, "patterns": 0, "antenna_blocks": 4},
    "issue31_pt_rp.nec": {"frequencies": 1, "patterns": 1, "antenna_blocks": 1, "source_rows": 1},
    "issue31_pq_rp.nec": {"frequencies": 1, "patterns": 1, "antenna_blocks": 1, "source_rows": 1},
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


def count_antenna_input_blocks(report):
    return len(re.findall(r"ANTENNA INPUT PARAMETERS", report))


def count_antenna_input_rows(report):
    header = "ANTENNA INPUT PARAMETERS"
    marker = "CURRENTS AND LOCATION"
    count = 0
    position = 0
    while True:
        start = report.find(header, position)
        if start == -1:
            break
        end = report.find(marker, start)
        if end == -1:
            end = len(report)
        section = report[start:end]
        count += len(re.findall(r"^\s*\d+\s+\d+\s+[+-]?\d+\.\d+E[+-]\d+", section, re.MULTILINE))
        position = start + len(header)
    return count


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


def issue34_pattern_value(report):
    for line in report.splitlines():
        if re.match(r"^\s*90\.00\s+0\.00\s", line):
            fields = line.split()
            for index, field in enumerate(fields):
                if re.fullmatch(r"[+-]?\d+\.\d+E[+-]\d+", field) and index + 1 < len(fields):
                    return float(field), float(fields[index + 1])
    return None


def check_issue34(onec, decks, temporary):
    source = decks / "issue34_near_field_and_rp_range.nec"
    source_text = source.read_text()
    results = []
    for distance in (1, 2):
        deck_text = source_text.replace("90 90 1 0", f"90 90 {distance} 0")
        deck = Path(temporary) / f"issue34_{distance}m.nec"
        output = Path(temporary) / f"issue34_{distance}m.out"
        deck.write_text(deck_text)
        result = run_deck(onec, deck, output)
        if result.returncode != 0:
            return False, f"onec exited {result.returncode} at RP range {distance} m"
        report = output.read_text(errors="replace")
        for section in ("NEAR ELECTRIC FIELDS", "NEAR MAGNETIC FIELDS"):
            if section not in report:
                return False, f"missing {section} section at RP range {distance} m"
        if f"RANGE:  {distance:.6E} METERS" not in report:
            return False, f"RP range {distance} m was not reported"
        value = issue34_pattern_value(report)
        if value is None:
            return False, f"could not read E-theta at RP range {distance} m"
        results.append(value)

    amp1, phase1 = results[0]
    amp2, phase2 = results[1]
    if not math.isclose(amp1 / amp2, 2.0, rel_tol=1e-4):
        return False, f"E-theta did not scale inversely with range ({amp1} vs {amp2})"
    if math.isclose(phase1, phase2, abs_tol=1.0):
        return False, f"E-theta phase did not change with range ({phase1} vs {phase2})"
    return True, "near-field sections present and RP amplitude/phase respond to range"


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
            version_match = re.search(r"\(onec\s+([A-Za-z0-9.]+)\)", report)
            if not version_match or version_match.group(1) != EXPECTED_VERSION:
                print(
                    f"FAIL {name}: expected banner version {EXPECTED_VERSION}, "
                    f"got {version_match.group(1) if version_match else 'missing'}"
                )
                failed = True
            else:
                print(f"PASS {name}: banner version {EXPECTED_VERSION}")

            actual = (
                len(re.findall(r"FREQUENCY=", report)),
                len(re.findall(r"RADIATION PATTERNS", report)),
            )
            expected_counts = (expected["frequencies"], expected["patterns"])
            if actual != expected_counts:
                print(
                    f"FAIL {name}: expected {expected_counts[0]} frequencies and "
                    f"{expected_counts[1]} patterns, got {actual[0]} and {actual[1]}"
                )
                failed = True
            else:
                print(f"PASS {name}: {actual[0]} frequencies, {actual[1]} patterns")

            if "antenna_blocks" in expected:
                antenna_blocks = count_antenna_input_blocks(report)
                if antenna_blocks != expected["antenna_blocks"]:
                    print(
                        f"FAIL {name}: expected {expected['antenna_blocks']} antenna input blocks, "
                        f"got {antenna_blocks}"
                    )
                    failed = True
                else:
                    print(f"PASS {name}: {antenna_blocks} antenna input blocks")

            if "source_rows" in expected:
                source_rows = count_antenna_input_rows(report)
                if source_rows != expected["source_rows"]:
                    print(
                        f"FAIL {name}: expected {expected['source_rows']} antenna input rows, "
                        f"got {source_rows}"
                    )
                    failed = True
                else:
                    print(f"PASS {name}: {source_rows} antenna input rows")

        passed, detail = check_issue34(onec, decks, temporary)
        if passed:
            print(f"PASS issue34_near_field_and_rp_range.nec: {detail}")
        else:
            print(f"FAIL issue34_near_field_and_rp_range.nec: {detail}")
            failed = True

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
