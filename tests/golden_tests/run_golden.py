#!/usr/bin/env python3
"""Run onec over the golden deck matrix and compare output against its pins.

expected.json holds the pinned input impedances and report values, while
PROVENANCE.md records where each pin came from. Exits 1 on any relative error
above tolerance, a missing report section/value, a nonzero onec exit, a missing
ANTENNA INPUT PARAMETERS block, or -- the issue #14 guard -- on an ANTENNA
ENVIRONMENT block that does not echo the deck's own GN card. Stdlib only,
Python 3.8+.
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
    if kind == -1:
        return []
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


def report_pattern_value(text, theta, phi):
    for line in text.splitlines():
        if re.match(r"^\s*%0.2f\s+%0.2f\s" % (theta, phi), line):
            fields = line.split()
            for index, field in enumerate(fields):
                if re.fullmatch(r"[-+]?\d+\.\d+E[-+]\d+", field) and index + 1 < len(fields):
                    return float(field), float(fields[index + 1])
    return None


def report_pattern_gains(text, theta, phi):
    for line in text.splitlines():
        if re.match(r"^\s*%0.2f\s+%0.2f\s" % (theta, phi), line):
            fields = line.split()
            try:
                return float(fields[2]), float(fields[3])
            except (IndexError, ValueError):
                return None
    return None


def report_output_faults(entry, deck_path, out):
    """Check issue-specific report output against its checked-in sample."""
    if "report_output" not in entry:
        return []
    name = entry["deck"]
    deck_text = open(deck_path, errors="replace").read()
    expected_card = entry.get("required_card")
    if expected_card and expected_card not in deck_text.splitlines():
        return ["%s: deck is missing required card %r" % (name, expected_card)]

    sample_path = os.path.join(HERE, entry["report_output"])
    if not os.path.exists(sample_path):
        return ["%s: expected report sample is missing (%s)" % (name, sample_path)]
    sample = open(sample_path, errors="replace").read()
    faults = []
    for section in entry.get("near_field_sections", []):
        if section not in sample:
            faults.append("%s: expected report sample lacks %s" % (name, section))
        if section not in out:
            faults.append("%s: output lacks %s" % (name, section))

    for required in entry.get("required_report_text", []):
        if required not in sample:
            faults.append("%s: expected report sample lacks %r" % (name, required))
        if required not in out:
            faults.append("%s: output lacks %r" % (name, required))

    if "ground_medium" in entry:
        medium = entry["ground_medium"]
        for label, text in (("sample", sample), ("output", out)):
            epsr, sigma = EPSR.search(text), SIGMA.search(text)
            if not epsr or not sigma:
                faults.append("%s: %s lacks second-medium values" % (name, label))
            else:
                if abs(float(epsr.group(1)) - medium["epsr"]) > 1e-3:
                    faults.append("%s: %s epsr %.6g, expected %.6g"
                                  % (name, label, float(epsr.group(1)), medium["epsr"]))
                if abs(float(sigma.group(1)) - medium["sigma"]) > 1e-6:
                    faults.append("%s: %s sigma %.6g, expected %.6g"
                                  % (name, label, float(sigma.group(1)), medium["sigma"]))

    for pin in entry.get("pattern_gains", []):
        for label, text in (("sample", sample), ("output", out)):
            values = report_pattern_gains(text, pin["theta_deg"], pin["phi_deg"])
            if values is None:
                faults.append("%s: %s lacks gain row at %.2f/%.2f degrees"
                              % (name, label, pin["theta_deg"], pin["phi_deg"]))
                continue
            vertical, _horizontal = values
            if abs(vertical - pin["vertical_db"]) > 0.02:
                faults.append("%s: %s vertical gain at %.2f degrees is %.2f dB, expected %.2f dB"
                              % (name, label, pin["theta_deg"], vertical, pin["vertical_db"]))

    if "rp_range_m" in entry:
        range_match = re.search(r"RANGE:\s*([0-9.E+-]+)\s+METERS", sample)
        actual_range = re.search(r"RANGE:\s*([0-9.E+-]+)\s+METERS", out)
        wanted_range = float(entry["rp_range_m"])
        if not range_match or not actual_range:
            faults.append("%s: missing RP RANGE output" % name)
        else:
            sample_range = float(range_match.group(1))
            got_range = float(actual_range.group(1))
            if abs(sample_range - wanted_range) > 1e-6 or abs(got_range - wanted_range) > 1e-6:
                faults.append("%s: RP range expected %.6g m, sample %.6g m, got %.6g m"
                              % (name, wanted_range, sample_range, got_range))

    if "e_theta" in entry:
        pin = entry["e_theta"]
        sample_value = report_pattern_value(sample, pin["theta_deg"], pin["phi_deg"])
        actual_value = report_pattern_value(out, pin["theta_deg"], pin["phi_deg"])
        if sample_value is None or actual_value is None:
            faults.append("%s: missing E-theta sample at %.2f/%.2f degrees"
                          % (name, pin["theta_deg"], pin["phi_deg"]))
        else:
            for label, value in (("sample", sample_value), ("output", actual_value)):
                amplitude, phase = value
                if abs(amplitude - pin["amplitude"]) / pin["amplitude"] > 1e-4:
                    faults.append("%s: E-theta %s amplitude %.6g, expected %.6g"
                                  % (name, label, amplitude, pin["amplitude"]))
                if abs(phase - pin["phase_deg"]) > 0.05:
                    faults.append("%s: E-theta %s phase %.3f deg, expected %.3f deg"
                                  % (name, label, phase, pin["phase_deg"]))

    if "average_gain" in entry:
        pattern = r"AVERAGE POWER GAIN:\s*([0-9.E+-]+)"
        sample_gain = re.search(pattern, sample)
        actual_gain = re.search(pattern, out)
        if not sample_gain or not actual_gain:
            faults.append("%s: missing AVERAGE POWER GAIN output" % name)
        else:
            wanted_gain = float(entry["average_gain"])
            for label, match in (("sample", sample_gain), ("output", actual_gain)):
                got_gain = float(match.group(1))
                if abs(got_gain - wanted_gain) / wanted_gain > 1e-4:
                    faults.append("%s: average gain %s %.6g, expected %.6g"
                                  % (name, label, got_gain, wanted_gain))

    if "power_budget" in entry:
        labels = {
            "input_power_w": "INPUT POWER",
            "radiated_power_w": "RADIATED POWER",
            "structure_loss_w": "STRUCTURE LOSS",
            "network_loss_w": "NETWORK LOSS",
        }
        for key, label in labels.items():
            pattern = re.escape(label) + r"\s*=\s*([0-9.E+-]+)"
            wanted = entry["power_budget"][key]
            tolerance = entry["power_budget"].get("relative_tolerance", 1e-4)
            for source, text in (("sample", sample), ("output", out)):
                match = re.search(pattern, text)
                if not match:
                    faults.append("%s: %s lacks %s" % (name, source, label))
                elif abs(float(match.group(1)) - wanted) > tolerance * max(abs(wanted), 1e-30):
                    faults.append("%s: %s %s %.7g W, expected %.7g W"
                                  % (name, source, label, float(match.group(1)), wanted))
        efficiency = re.search(r"EFFICIENCY\s*=\s*([0-9.E+-]+)", sample)
        actual_efficiency = re.search(r"EFFICIENCY\s*=\s*([0-9.E+-]+)", out)
        wanted_efficiency = entry["power_budget"]["efficiency_percent"]
        tolerance = entry["power_budget"].get("efficiency_tolerance_percent", 0.02)
        if not efficiency or not actual_efficiency:
            faults.append("%s: missing power budget efficiency" % name)
        else:
            for source, match in (("sample", efficiency), ("output", actual_efficiency)):
                got = float(match.group(1))
                if abs(got - wanted_efficiency) > tolerance:
                    faults.append("%s: %s efficiency %.2f%%, expected %.2f%%"
                                  % (name, source, got, wanted_efficiency))
    return faults


def check_rp_xnda_variants(onec, deck_path, tmp, expected_average_gain):
    """Exercise X, N, D, and A RP I4 digits on the sequential path."""
    source = open(deck_path, errors="replace").read()
    original = "RP 0 37 73 1001 0 0 5 5"
    expectations = {
        "0000": ("MAJOR", "MINOR"),
        "1000": ("VERTC", "HORIZ"),
        "0100": ("NORMALIZED GAIN",),
        "0010": ("DIRECTIVE GAINS",),
        "0001": ("AVERAGE POWER GAIN",),
    }
    faults = []
    if original not in source.splitlines():
        return ["issue #35: source deck is missing the expected RP I4 card"]
    for flags, required in expectations.items():
        deck_text = source.replace(original, original.replace("1001", flags))
        variant = os.path.join(tmp, "issue35_xnda_%s.nec" % flags)
        output = os.path.join(tmp, "issue35_xnda_%s.out" % flags)
        open(variant, "w").write(deck_text)
        run = subprocess.run([onec, "-f", "nec2c", "-i", variant, "-o", output],
                             stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        if run.returncode != 0:
            faults.append("issue #35 I4=%s: onec exited %d" % (flags, run.returncode))
            continue
        report = open(output, errors="replace").read()
        for text in required:
            if text not in report:
                faults.append("issue #35 I4=%s: output lacks %r" % (flags, text))

    # The issue reproduction uses onec's default Fortran-style report format.
    default_output = os.path.join(tmp, "issue35_xnda_default.out")
    run = subprocess.run([onec, "-i", deck_path, "-o", default_output],
                         stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if run.returncode != 0:
        faults.append("issue #35 default format: onec exited %d" % run.returncode)
    else:
        report = open(default_output, errors="replace").read()
        match = re.search(r"AVERAGE POWER GAIN:\s*([0-9.E+-]+)", report)
        if not match:
            faults.append("issue #35 default format: missing AVERAGE POWER GAIN output")
        elif abs(float(match.group(1)) - expected_average_gain) / expected_average_gain > 1e-4:
            faults.append("issue #35 default format: average gain %.6g, expected %.6g"
                          % (float(match.group(1)), expected_average_gain))
    return faults


def check_ek_off_variant(onec, deck_path, tmp, pin, tolerance):
    """Confirm EK -1 disables the kernel and restores the plain-kernel pin."""
    source = open(deck_path, errors="replace").read()
    if "EK 0" not in source.splitlines():
        return ["issue #37: source deck is missing EK 0"]
    variant = os.path.join(tmp, "issue37_ek_minus_one.nec")
    output = os.path.join(tmp, "issue37_ek_minus_one.out")
    open(variant, "w").write(source.replace("EK 0", "EK -1"))
    run = subprocess.run([onec, "-f", "nec2c", "-i", variant, "-o", output],
                         stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if run.returncode != 0:
        return ["issue #37 EK -1: onec exited %d" % run.returncode]
    report = open(output, errors="replace").read()
    if "THE EXTENDED THIN WIRE KERNEL WILL BE USED" in report:
        return ["issue #37 EK -1: extended-kernel message should be absent"]
    rows = parse_blocks(report)
    if len(rows) != 1:
        return ["issue #37 EK -1: expected one impedance row, parsed %d" % len(rows)]
    freq, tag, seg, r, x = rows[0]
    expected = complex(pin["r"], pin["x"])
    actual = complex(r, x)
    faults = []
    if abs(actual - expected) / abs(expected) > tolerance:
        faults.append("issue #37 EK -1: impedance %.4f%+.4fj differs from plain-kernel pin %.4f%+.4fj"
                      % (r, x, expected.real, expected.imag))
    if (tag, seg) != (pin["tag"], pin["seg"]):
        faults.append("issue #37 EK -1: row is %d/%d, expected %d/%d"
                      % (tag, seg, pin["tag"], pin["seg"]))
    if freq is not None and abs(freq - pin["freq_mhz"]) > 1e-6 * pin["freq_mhz"]:
        faults.append("issue #37 EK -1: frequency %g MHz, expected %g MHz"
                      % (freq, pin["freq_mhz"]))
    return faults


def check(entry, args, tmp):
    """Printed rows plus the list of failure strings for one deck."""
    deck, points = entry["deck"], entry.get("points", [])
    path, out_path = os.path.join(args.decks, deck), os.path.join(tmp, deck + ".out")
    command = [args.onec]
    if "format" in entry:
        command += ["-f", entry["format"]]
    run = subprocess.run(command + ["-i", path, "-o", out_path],
                         stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    if run.returncode != 0:
        return ["%s: onec exited %d" % (deck, run.returncode)]
    if not os.path.exists(out_path):
        return ["%s: onec wrote no output file" % deck]
    out = open(out_path, errors="replace").read()
    bad = ["%s: %s" % (deck, f) for f in
           environment_faults(open(path, errors="replace").read(), out)]
    if points:
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
    if "report_output" in entry:
        bad += report_output_faults(entry, path, out)
        if entry.get("rp_xnda_variants"):
            bad += check_rp_xnda_variants(args.onec, path, tmp, entry["average_gain"])
        if "ek_off_variant" in entry:
            bad += check_ek_off_variant(args.onec, path, tmp,
                                        entry["ek_off_variant"], args.tolerance)
        if not points:
            print("%-27s %6s %19s %19s %9s  %s"
                  % (deck, "--", "--", "--", "--", "FAIL" if bad else "PASS"))
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
        failures += check(entry, args, tmp)
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
