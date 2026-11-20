#!/usr/bin/env python3
"""Check numerical results with reuse enabled through the onec CLI.

Run with an OpenMP build: python3 tests/benchmark_tests/check_fill_reuse_sampling.py
The near-cap model has distinct horizontal distances between parallel wires:
reverse pairs share a class and all self pairs share one class, placing its
class count just below P/2 + 1. Varying source lengths breaks those matches.
Pass --state-probe with the compiled fill_reuse_state.c test driver to also
check reuse/rejection decisions without adding hooks to the production code.
"""

import argparse
import os
from pathlib import Path
import random
import re
import subprocess
import tempfile


def irregular_wires(count, varying_lengths, seed):
    rng = random.Random(seed)
    cards = []
    for index in range(count):
        x, y = rng.uniform(-10, 10), rng.uniform(-10, 10)
        half = 0.01 * (1 + index / count if varying_lengths else 1)
        cards.append(
            f"GW {index + 1} 1 {x:.12f} {y:.12f} {-half:.12f} "
            f"{x:.12f} {y:.12f} {half:.12f} .0001"
        )
    return cards


def numerical_report(text):
    return "\n".join(
        line.rstrip()
        for line in text.splitlines()
        if not re.search(r"FILL=|RUN TIME =", line)
    )


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--onec", default="./onec")
    parser.add_argument("--state-probe", help="path to the linked fill_reuse_state.c driver")
    args = parser.parse_args()
    binary = str(Path(args.onec).resolve())
    state_probe = str(Path(args.state_probe).resolve()) if args.state_probe else None
    cases = [
        ("small_repeated", ["GW 1 21 0 0 -2.5 0 0 2.5 .001"], 1, False),
        ("small_unique", irregular_wires(21, True, 1), 1, True),
        ("sampled_repeated", ["GW 1 201 0 0 -2.5 0 0 2.5 .001"], 101, False),
    ]
    for seed in range(3):
        cases.append((f"near_cap_{seed}", irregular_wires(1001, False, seed), 1, False))
        cases.append((f"low_reuse_{seed}", irregular_wires(1001, True, seed), 1, True))
    with tempfile.TemporaryDirectory(prefix="opennec-sampling-") as scratch:
        folder = Path(scratch)
        for name, geometry, excitation, reject in cases:
            deck = folder / f"{name}.nec"
            deck.write_text(
                "CM sampling regression\nCE\n" + "\n".join(geometry)
                + f"\nGE 0\nFR 0 3 0 0 28 1\nEX 0 1 {excitation} 0 1 0\nXQ\nEN\n"
            )
            reference = None
            for threads, reuse in [(8, False), (1, True), (8, True)]:
                output = folder / f"{name}-{threads}-{int(reuse)}.out"
                env = os.environ.copy()
                env.update(
                    OMP_NUM_THREADS=str(threads), VECLIB_MAXIMUM_THREADS="8",
                    OPENNEC_FILL_REUSE=str(int(reuse)),
                )
                result = subprocess.run(
                    [binary, "-o", str(output), str(deck)], env=env,
                    capture_output=True, text=True, check=True,
                )
                report = numerical_report(output.read_text())
                if reference is None:
                    reference = report
                    continue
                # The ordinary path independently evaluates each field pair.
                # Allow a last printed digit to round differently while requiring
                # the full report structure and all remaining text to agree.
                numbers = r"[-+]?\d+\.\d+E[-+]\d+"
                expected = [float(x) for x in re.findall(numbers, reference)]
                actual = [float(x) for x in re.findall(numbers, report)]
                assert len(expected) == len(actual), (name, "report shape")
                assert re.sub(numbers, "VALUE", reference) == re.sub(numbers, "VALUE", report), name
                differences = [(a, b) for a, b in zip(expected, actual)
                               if abs(a-b) > 2e-4 * max(abs(a), abs(b), 1e-20)]
                assert not differences, (name, "numerical difference", differences[:5])
                assert "fill reuse" not in result.stderr.lower(), (name, result.stderr)
                if state_probe:
                    subprocess.run([state_probe, str(deck), "reject" if reject else "reuse"],
                                   env=env, capture_output=True, text=True, check=True)
            if state_probe and name == "small_repeated":
                # The context's own choice wins over the environment, both ways.
                for mode, env_value in (("ctx-off", "1"), ("ctx-on", "0")):
                    env = os.environ.copy()
                    env.update(OMP_NUM_THREADS="8", VECLIB_MAXIMUM_THREADS="8",
                               OPENNEC_FILL_REUSE=env_value)
                    subprocess.run([state_probe, str(deck), mode], env=env,
                                   capture_output=True, text=True, check=True)
            choice = f"{'reject' if reject else 'reuse'}, " if state_probe else ""
            print(f"PASS {name}: {choice}1/8 threads, 3 frequencies")
    print(f"All {len(cases)} sampling cases passed.")


if __name__ == "__main__":
    main()
