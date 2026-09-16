#!/usr/bin/env python3
"""Run onec over the golden deck matrix and compare Z against the pins.

One deck per feature axis; expected.json holds the pinned input impedance and
PROVENANCE.md records where each pin came from.  Exits 1 on any relative error
above the pinned tolerance, on a nonzero onec exit, on a missing ANTENNA INPUT
PARAMETERS block, and -- the issue #14 guard -- on an ANTENNA ENVIRONMENT block
that does not echo the deck's own GN card.  Stdlib only, Python 3.8+.
"""
import argparse, json, os, re, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
NUM = re.compile(r"[-+]?\d+\.\d+E[-+]\d+")
ROW = re.compile(r"^\s*(\d+)\s+(\d+)\s+[-+]?\d\.\d+E")
FREQ = re.compile(r"FREQUENCY[=:]\s*([0-9.E+-]+)\s*MHZ", re.I)
EPSR = re.compile(r"RELATIVE DIELECTRIC CONST[.:= ]+([0-9.E+-]+)", re.I)
SIGMA = re.compile(r"CONDUCTIVITY[.:= ]+([0-9.E+-]+)\s*MHOS", re.I)


def parse_blocks(text):
    """(freq_mhz, tag, seg, R, X) for the first row of each input-parameters
    block.  Each deck drives one EX.  A TL/NT deck also prints a STRUCTURE
    EXCITATION table of the same column shape, but it sits ABOVE the header,
    so scoping the search to the header excludes it."""
    rows, freq, want = [], None, False
    for line in text.splitlines():
        hit = FREQ.search(line)
        if hit:
            freq = float(hit.group(1))
        if "ANTENNA INPUT PARAMETERS" in line:
            want = True
        elif want and ROW.match(line):
            tag, seg = ROW.match(line).groups()
            v = NUM.findall(line)
            rows.append((freq, int(tag), int(seg), float(v[4]), float(v[5])))
            want = False
    return rows


def environment_faults(deck_text, out):
    """Issue #14: what the echoed ANTENNA ENVIRONMENT gets wrong about the GN."""
    gn = next((ln.split() for ln in deck_text.splitlines()
               if ln[:2].upper() == "GN"), None)
    if gn is None:
        return []
    kind = int(float(gn[1]))
    if kind == 1:
        return [] if "PERFECT GROUND" in out else ["GN 1 but no PERFECT GROUND echoed"]
    got_e, got_s = EPSR.search(out), SIGMA.search(out)
    if not got_e or not got_s:
        return ["GN %d but no epsr/sigma echoed in the environment block" % kind]
    faults = []
    for name, want, got in (("epsr", float(gn[5]), float(got_e.group(1))),
                            ("sigma", float(gn[6]), float(got_s.group(1)))):
        if abs(got - want) > 1e-3 * abs(want):
            faults.append("GN says %s=%g, output echoes %g" % (name, want, got))
    return faults


def check(deck, points, args, tmp):
    """Printed rows plus the list of failure strings for one deck."""
    path, out_path = os.path.join(args.decks, deck), os.path.join(tmp, deck + ".out")
    run = subprocess.run([args.onec, "-i", path, "-o", out_path],
                         stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if run.returncode != 0:
        return ["%s: onec exited %d" % (deck, run.returncode)]
    if not os.path.exists(out_path):
        return ["%s: onec wrote no output file" % deck]
    out = open(out_path, errors="replace").read()
    bad = ["%s: %s" % (deck, f) for f in
           environment_faults(open(path, errors="replace").read(), out)]
    rows = parse_blocks(out)
    if len(rows) != len(points):
        return bad + ["%s: expected %d input-parameters block(s), parsed %d"
                      % (deck, len(points), len(rows))]
    for want, (freq, tag, seg, r, x) in zip(points, rows):
        zw, why = complex(want["r"], want["x"]), []
        rel = abs(complex(r, x) - zw) / abs(zw)
        if rel > args.tolerance:
            why.append("rel %.3e > %.0e" % (rel, args.tolerance))
        if freq is not None and abs(freq - want["freq_mhz"]) > 1e-6 * want["freq_mhz"]:
            why.append("block at %g MHz, expected %g" % (freq, want["freq_mhz"]))
        if (tag, seg) != (want["tag"], want["seg"]):
            why.append("row is tag %d seg %d, expected %d/%d"
                       % (tag, seg, want["tag"], want["seg"]))
        print("%-27s %6.3f %9.4f%+9.4f %9.4f%+9.4f %9.2e  %s"
              % (deck, want["freq_mhz"], zw.real, zw.imag, r, x, rel,
                 "FAIL" if why else "PASS"))
        bad += ["%s at %g MHz: %s" % (deck, want["freq_mhz"], w) for w in why]
    return bad


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--onec", default="./onec", help="path to the onec binary")
    ap.add_argument("--decks", default=os.path.join(HERE, "decks"))
    ap.add_argument("--expected", default=os.path.join(HERE, "expected.json"))
    args = ap.parse_args()
    spec = json.load(open(args.expected))
    args.tolerance = spec["tolerance"]
    tmp = tempfile.mkdtemp(prefix="golden-")
    print("%-27s %6s %19s %19s %9s  %s"
          % ("DECK", "MHZ", "EXPECTED R+jX", "GOT R+jX", "REL", "RESULT"))
    failures = []
    for entry in spec["decks"]:
        failures += check(entry["deck"], entry["points"], args, tmp)
    print()
    if failures:
        print("%d failure(s):" % len(failures))
        for f in failures:
            print("  " + f)
        return 1
    print("all %d decks pass at %.0e relative" % (len(spec["decks"]), args.tolerance))
    return 0


if __name__ == "__main__":
    sys.exit(main())
