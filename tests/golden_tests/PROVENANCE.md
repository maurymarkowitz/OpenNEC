Golden impedance matrix — provenance
====================================

Eleven minimal decks, one per feature axis, each with a pinned input impedance.
`run_golden.py` runs `onec` over them and fails CI when any |Z − Z_exp| / |Z_exp|
exceeds 1e-4.  This file records the raw number every engine gave, so a pin can
be audited rather than trusted.

Two lanes
---------

The decks run in **two CI jobs against one script**, so a red X names exactly
one deck:

| job | pins | at `acafb37` |
|---|---|---|
| `golden-impedance` | `expected.json` — decks 01–10, twelve points | **green**, widest 4.0e-05 |
| `golden-open-issues` | `expected_open.json` — decks pinned to an open issue, deck 11 today | **red by design**, 1.7e-01, issue #14 |

`run_golden.py` already took `--expected`, so the split is a second pin file and
a second job — no change to the script.  **When an issue closes, move its deck's
entry from `expected_open.json` into `expected.json`** and the floor grows by
one deck.  The floor job never runs deck 11 and the open-issues job never runs
01–10, so neither lane can mask the other.

Engines
-------

| role | engine | version | how it was driven |
|---|---|---|---|
| pin | nec2c | 1.3.1-2 (Debian) | `nec2c -i <deck>.nec`, read `<deck>.out` |
| cross-check | PyNEC (nec2++) | pynec-accel 1.7.6 | a context built card-by-card in Python, not from the deck text |
| cross-check | momwire | 0.56.0 (`--basis sinusoidal`) | `momwire.portal.run_deck(<deck text>)` |
| under test | onec | 2.2.1 @ `acafb37` | `onec -i <deck>.nec -o <out>` |

nec2c and onec both descend from the NEC-2 Fortran, so they are **not**
independent of each other; that is the point of the other two.  PyNEC drives
nec2++, a separate C++ transliteration of the same Fortran — an independent
implementation of the same algorithm.  momwire is an independent method-of-
moments code whose `sinusoidal` basis is NEC-2's basis but whose quadrature and
thin-wire kernel are its own — an independent implementation of the same
*physics*.

What "agreement" actually measures here
---------------------------------------

**No deck reaches a three-way agreement at 1e-4, and none can.**  Measured
2026-09-16 over the whole matrix:

| pair | agreement |
|---|---|
| onec ↔ nec2c | 1.1e-06 … 4.0e-05 (and 1.7e-01 on deck 11) |
| nec2c ↔ PyNEC (nec2++) | 3.2e-05 … 2.1e-04 |
| nec2c ↔ momwire sinusoidal | 7.7e-04 … 1.8e-02 |

Two independent transliterations of the same Fortran land ~2e-4 apart on these
decks (a near-constant +0.015 Ω offset in X on the single-dipole cases), and an
independent MoM code lands ~1% away.  1e-4 is therefore a **regression
tolerance against the nec2c pin**, not a claim that the pinned number is correct
to 1e-4 in any absolute sense.  Read the three columns below as three tiers:

* the pin is nec2c's own number, reproducible with a stock `nec2c`;
* nec2++ agreeing to ~2e-4 says the pin is not an arithmetic slip in nec2c;
* momwire agreeing to ~1% says the pin is the right *antenna*, reached without
  sharing a line of NEC's code.

The gap that matters is the one on deck 11: three engines inside 9.5e-04 of each
other, and onec 1.7e-01 away from all three.  No tolerance argument reaches that.

Every pin below is a three-way pin at those tiers, except deck 10, which is
noted as two-way with its reason.

Raw numbers
-----------

| deck | MHz | nec2c 1.3.1 (pin) | PyNEC / nec2++ | momwire sinusoidal | max pairwise |
|---|---|---|---|---|---|
| `01_dipole_free` | 30 | 81.6220 +46.4620j | 81.62555 +46.47762j | 81.4230 +45.5720j | 9.88e-03 |
| `02_gn1_pec` | 30 | 79.3130 +36.2260j | 79.31448 +36.24141j | 79.1240 +35.3600j | 1.03e-02 |
| `03_gn2_sommerfeld` | 30 | 79.9250 +41.2270j | 79.92768 +41.24303j | 80.0060 +40.3270j | 1.02e-02 |
| `04_ld0_series_rlc` | 30 | 100.4100 +56.3270j | 100.41202 +56.34275j | 100.1700 +55.4210j | 8.28e-03 |
| `05_ld5_conductivity` | 30 | 82.3390 +47.0690j | 82.34636 +47.08417j | 82.1420 +46.1770j | 9.80e-03 |
| `06_junction_three_wire` | 30 | 46.4300 -279.3500j | 46.43254 -279.32967j | 46.3860 -280.0000j | 2.37e-03 |
| `07_tl` | 30 | 16.7160 -16.4010j | 16.71567 -16.40167j | 16.7020 -16.3540j | 2.12e-03 |
| `08_nt` | 30 | 68.5870 +18.9220j | 68.58922 +18.92919j | 68.4400 +18.4830j | 6.61e-03 |
| `09_fr_sweep` | 28 | 66.8940 -26.0700j | 66.89679 -26.05486j | 66.7410 -26.9050j | 1.20e-02 |
| `09_fr_sweep` | 29 | 73.9200 +10.3680j | 73.92273 +10.38307j | 73.7450 +9.5067j | 1.20e-02 |
| `09_fr_sweep` | 30 | 81.6220 +46.4620j | 81.62555 +46.47762j | 81.4230 +45.5720j | 9.88e-03 |
| `10_gx_symmetry` | 30 | 24.8890 +67.3310j | *(gx_card defective — see below)* | 24.4610 +66.1190j | 1.79e-02 |
| `11_gn2_low_height` | 14 | 91.7580 +13.2220j | 91.76060 +13.24058j | 91.7850 +13.1560j | 9.49e-04 |

"max pairwise" is the largest relative difference among the three engines for
that point.

Per-deck notes
--------------

* `01`–`09` — three-way.  The `nec2c` ↔ `nec2++` column runs 3.2e-05 … 2.1e-04
  and the `nec2c` ↔ momwire column 2.1e-03 … 1.2e-02.
* `03_gn2_sommerfeld` — NEC-2's `GN 2` builds its interpolation tables in
  process, so no external Sommerfeld file is needed.  The antenna is a
  wavelength up, where the ground is a weak perturbation; `onec` at `acafb37`
  gets it to 4.0e-06.  Deck 11 is the same card at a height where it bites.
* `07_tl` / `08_nt` — momwire composes both cards natively (its `DeckSolver`
  builds network ports and stamps the branch), so these are three-way too.
* `10_gx_symmetry` — **two-way**: nec2c + momwire.  PyNEC's `gx_card(1, 100)`
  returns 109.563 +50.807j where its own *explicit* two-wire twin (the same 18
  segments, tags 1 and 2, at x = ±0.5 m, built with two `wire()` calls and no
  `GX`) returns 24.897 +67.353j — a factor-of-1.2 error, i.e. nec2++ gets the
  symmetric-solve path wrong, not the geometry.  nec2c's `GX` result is
  bit-identical to nec2c's own explicit twin, and momwire's `GX` lands 1.8e-02
  from it, so the pin stands on those two.  PyNEC's explicit twin is a third
  corroboration of the *structure* at 3.5e-04, but it does not exercise `GX`
  and so is not counted as a third opinion on this axis.
* `11_gn2_low_height` — **three-way, and the tightest agreement in the matrix**:
  nec2c 91.758 +13.222j, nec2++ 91.7606 +13.2406j (2.0e-04), momwire 91.785
  +13.156j (7.7e-04).  A 21-segment half-wave dipole 0.43 m up — 0.02 λ at
  14 MHz — over the same `GN 2` ground as deck 03.  `onec` answers
  79.228 +4.256j, 1.7e-01 away, **while echoing `RELATIVE DIELECTRIC CONST.=
  10.000` correctly**: the environment block is right and the solution is not,
  which is precisely why this matrix pins impedances and not just the echo.

  nec2++ was used as a corroborator here deliberately.  Its historical
  near-ground `GN 2` interpolation defect is real but was **fixed**, and
  `pynec-accel 1.7.6` carries the fix; that was re-measured for this deck rather
  than assumed.  A six-point height ladder on this geometry at 14 MHz, nec2c
  against nec2++:

  | height | 2.00 m | 0.43 m | 0.20 m | 0.10 m | 0.05 m | 0.02 m |
  |---|---|---|---|---|---|---|
  | rel | 3.7e-04 | 2.0e-04 | 1.5e-04 | 1.4e-04 | 1.0e-04 | 1.8e-04 |

  Flat all the way down to 0.02 m (0.0009 λ) — no low-height breakdown in this
  build, so there is no reason to drop it to a two-way pin.

onec at `acafb37`
-----------------

| deck | MHz | onec 2.2.1 @ acafb37 | rel to pin | |
|---|---|---|---|---|
| `01_dipole_free` | 30 | 81.6220 +46.4619j | 1.06e-06 | pass |
| `02_gn1_pec` | 30 | 79.3130 +36.2257j | 3.44e-06 | pass |
| `03_gn2_sommerfeld` | 30 | 79.9252 +41.2273j | 4.01e-06 | pass |
| `04_ld0_series_rlc` | 30 | 100.4080 +56.3268j | 1.75e-05 | pass |
| `05_ld5_conductivity` | 30 | 82.3428 +47.0685j | 4.04e-05 | pass |
| `06_junction_three_wire` | 30 | 46.4303 -279.3490j | 3.69e-06 | pass |
| `07_tl` | 30 | 16.7159 -16.4015j | 2.18e-05 | pass |
| `08_nt` | 30 | 68.5870 +18.9217j | 4.22e-06 | pass |
| `09_fr_sweep` | 28 | 66.8941 -26.0698j | 3.11e-06 | pass |
| `09_fr_sweep` | 29 | 73.9196 +10.3678j | 5.99e-06 | pass |
| `09_fr_sweep` | 30 | 81.6220 +46.4619j | 1.06e-06 | pass |
| `10_gx_symmetry` | 30 | 24.8892 +67.3309j | 3.12e-06 | pass |
| `11_gn2_low_height` | 14 | 79.2283 +4.2560j | 1.66e-01 | **FAIL** |

The twelve points of the `golden-impedance` floor all pass at 1e-4, and that
job is green.  The thirteenth, `11_gn2_low_height`, is the whole of the
`golden-open-issues` job: it **fails at 1.66e-01 and maps to issue #14** — the
`GN 2` sequential-path fix at `6a7a395` corrected the echoed environment block
and the far-field case (deck 03), but the near-ground solution is still wrong.
That job is expected red until #14 closes; the floor is the regression contract
and must stay green.

`05_ld5_conductivity` is the widest passing row at 4.0e-05 and is the one where
onec and nec2c differ by more than nec2c's printed rounding (onec 82.3428,
nec2c 82.339 ± 0.0005, nec2++ 82.34636) — onec and nec2++ agree more closely
than either agrees with nec2c on the `LD 5` axis.  Worth a look, but well
inside the gate.

Reproducing
-----------

    make RELEASE=1

    # the regression floor -- must exit 0
    python3 tests/golden_tests/run_golden.py --onec ./onec

    # the open-issue lane -- exits 1 until #14 closes
    python3 tests/golden_tests/run_golden.py --onec ./onec \
        --expected tests/golden_tests/expected_open.json

The guards were checked against deliberately broken runs: a 0.1 % shift in a
pin, a wrapper that exits 3, a wrapper that removes the input-parameters
header, and a wrapper that rewrites `RELATIVE DIELECTRIC CONST.= 10.000` to
`0.000` (the issue #14 echo symptom).  All four exit 1 and name the deck.

Pin precision
-------------

nec2c prints five significant digits, so each pin carries ~1.2e-06 of
quantisation — two orders under the gate.
