#!/usr/bin/env python3
"""Exercise CLI endpoint selection and input protection with small decks."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

parser = argparse.ArgumentParser()
parser.add_argument('--onec', required=True)
args = parser.parse_args()
ONEC = str(Path(args.onec).resolve())
DECK = b'CM dipole\nCE\nGW 1 3 0 0 -0.25 0 0 0.25 0.001\nGE 0\nEX 0 1 2 0 1 0\nXQ\nEN\n'

class CLI(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(prefix='onec-cli-')
        self.dir = Path(self.tmp.name)
        self.deck = self.dir / 'antenna.nec'
        self.deck.write_bytes(DECK)

    def tearDown(self):
        self.tmp.cleanup()

    def run_onec(self, *args, data=None, ok=True):
        result = subprocess.run([ONEC, *map(str, args)], input=data, capture_output=True, cwd=self.dir)
        self.assertEqual(result.returncode == 0, ok, (args, result.stdout, result.stderr))
        return result

    def test_endpoints(self):
        out = self.dir / 'named.out'
        self.run_onec(self.deck, out)
        self.assertGreater(out.stat().st_size, 0)
        self.run_onec(self.deck)
        self.assertGreater(self.deck.with_suffix('.out').stat().st_size, 0)
        self.assertGreater(len(self.run_onec(self.deck, '-').stdout), 100)
        self.run_onec('-', out, data=DECK)
        self.assertGreater(out.stat().st_size, 0)
        for argv in [('-',), ('-', '-')]:
            self.assertGreater(len(self.run_onec(*argv, data=DECK).stdout), 100)
        self.run_onec('-i', self.deck, '-o', out)
        self.run_onec(self.deck, '-o', out)
        self.run_onec('-i', '-', '-o', out, data=DECK)

    def test_prompt_and_errors(self):
        out = self.dir / 'prompt.out'
        self.run_onec(data=(str(self.deck)+'\n'+str(out)+'\n').encode())
        self.assertGreater(out.stat().st_size, 0)
        self.run_onec(data=b'only-one-line\n', ok=False)
        self.run_onec('-o', out, data=DECK, ok=False)
        self.run_onec('-i', self.deck, out, ok=False)
        self.run_onec(self.deck, out, '-o', out, ok=False)
        self.run_onec(self.deck, out, 'extra', ok=False)
        self.assertGreater(len(self.run_onec('--help').stdout), 10)

    @unittest.skipIf(os.name == 'nt', 'POSIX pseudo-terminal')
    def test_interactive_prompt(self):
        import pty
        master, slave = pty.openpty()
        out = self.dir / 'interactive.out'
        try:
            process = subprocess.Popen([ONEC], stdin=slave, stdout=slave,
                                       stderr=subprocess.PIPE, cwd=self.dir)
            os.close(slave)
            slave = None
            os.write(master, (str(self.deck) + '\n' + str(out) + '\n').encode())
            _, error = process.communicate(timeout=20)
            self.assertEqual(process.returncode, 0, error)
            self.assertGreater(out.stat().st_size, 0)
        finally:
            if slave is not None:
                os.close(slave)
            os.close(master)

    def test_identity(self):
        self.run_onec(self.deck, self.deck, ok=False)
        self.assertEqual(self.deck.read_bytes(), DECK)
        for kind in ('hard', 'symbolic'):
            alias = self.dir / (kind + '.nec')
            try:
                if kind == 'hard': os.link(self.deck, alias)
                else: alias.symlink_to(self.deck)
            except OSError:
                continue
            self.run_onec('-i', self.deck, '-o', alias, ok=False)
            self.assertEqual(self.deck.read_bytes(), DECK)
        self.run_onec('-n', '-w', self.deck, self.deck, ok=False)
        self.assertEqual(self.deck.read_bytes(), DECK)
        out_input = self.dir / 'same.out'
        out_input.write_bytes(DECK)
        self.run_onec(out_input, ok=False)
        self.assertEqual(out_input.read_bytes(), DECK)

    def test_batch(self):
        other = self.dir / 'second.nec'
        other.write_bytes(DECK)
        self.run_onec('--batch', self.deck, other)
        self.assertTrue(self.deck.with_suffix('.out').exists())
        self.assertTrue(other.with_suffix('.out').exists())
        nested = self.dir / 'sub'
        nested.mkdir()
        shutil.copyfile(self.deck, nested / 'nested.nec')
        self.run_onec('--batch', '-r', self.dir)
        self.assertTrue((nested / 'nested.out').exists())
        self.run_onec('--batch', '-o', 'report.out', self.deck, ok=False)
        self.run_onec('--batch', '-', data=DECK, ok=False)
        self.run_onec('-r', self.dir, ok=False)

    def test_dotted_directory(self):
        parent = self.dir / 'models.v1'
        parent.mkdir()
        deck = parent / 'antenna'
        deck.write_bytes(DECK)
        self.run_onec(deck)
        self.assertTrue((parent / 'antenna.out').exists())

if __name__ == '__main__':
    unittest.main(argv=[__file__])
