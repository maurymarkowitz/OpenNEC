#!/usr/bin/env python3
"""Regression checks for symbol scope and formula errors."""

import subprocess
import tempfile
import unittest
from pathlib import Path


ONEC = Path(__file__).resolve().parents[2] / "onec"


class OrderedSymbolTests(unittest.TestCase):
    def run_deck(self, cards):
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "input.nec"
            output = Path(directory) / "output.nec2"
            source.write_text("CM Symbol test\nCE\n" + cards + "\nEN\n")
            result = subprocess.run(
                [str(ONEC), "-n", "-w", str(output), str(source)],
                capture_output=True, text=True,
            )
            return result, output.read_text() if output.exists() else ""

    def test_redefinition_applies_to_later_cards_and_assignments(self):
        result, output = self.run_deck("""\
SY a=1
GW 1 3 0 0 0 0 a 0 0.001
SY A=2
GW 2 3 1 0 0 1 a 0 0.001
GE 0
FR 0 1 0 0 A 0
SY a=a+1
FR 0 1 0 0 a 0""")
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("GW 1 3 0 0 0 0 1 0 0.001", output)
        self.assertIn("GW 2 3 1 0 0 1 2 0 0.001", output)
        self.assertIn("FR 0 1 0 0 2 0", output)
        self.assertIn("FR 0 1 0 0 3 0", output)

    def test_forward_reference_is_rejected(self):
        result, output = self.run_deck("""\
GW 1 3 0 0 0 0 length 0 0.001
SY length=2
GE 0""")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Error evaluating formula", result.stderr)
        self.assertEqual(output, "")

    def test_cycle_is_rejected(self):
        result, output = self.run_deck("SY a=b, b=a\nGE 0")
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Error evaluating formula", result.stderr)
        self.assertEqual(output, "")


if __name__ == "__main__":
    unittest.main()
