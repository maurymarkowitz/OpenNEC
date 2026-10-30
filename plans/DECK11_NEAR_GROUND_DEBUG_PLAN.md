Deck 11 near-ground debug plan
===============================

Problem statement
-----------------

Deck 11 (`11_gn2_low_height.nec`) places a 21-segment half-wave dipole 0.43 m
(0.02 lambda) above a Sommerfeld/Norton ground (GN 2, er=10, sigma=0.002) at 14
MHz.  Three independent reference engines all agree to within 9.5e-04:

| engine             | Z (ohm)            |
|--------------------|--------------------|
| nec2d (Fortran)    | 91.760 + j13.223   |
| nec2c 1.3.1 clean  | 91.758 + j13.222   |
| nec2++ / PyNEC     | 91.761 + j13.241   |

OpenNEC answers 79.228 + j4.256, which is 1.7e-01 away from the pin.

The three reference engines bracket the correct answer tightly.  The Fortran
nec2d in `~/Developer/Nec2dXS_src` reproduces the pin to five digits and is
treated as the oracle throughout this plan.

Current state of the reference builds
--------------------------------------

The nec2c copy at `~/Developer/nec2c-1.3` has been **modified** from the Debian
1.3.1 binary that produced the pin.  The changes are confined to
`calculations.c`; a clean backup exists as `calculations.c.backup`.  The
modifications added an incorrect "Issue #15" cache-sentinel reset inside
`intrp()` and layered in debug trace output, changing the computed answer to
84.970 + j4.713 (with some trace flags) or 12.375 - j40.138 (with others).

Before any instrumentation work, `calculations.c` must be **restored from its
backup**.  The `.backup` file is the Debian 1.3.1 original and must not be
modified.

### Step 0: restore nec2c

```bash
cp ~/Developer/nec2c-1.3/calculations.c.backup \
   ~/Developer/nec2c-1.3/calculations.c
make -C ~/Developer/nec2c-1.3
~/Developer/nec2c-1.3/nec2c -i tests/golden_tests/decks/11_gn2_low_height.nec \
    -o debugging/11_restore_check-nec2c.out
```

The result must be 91.758 + j13.222 before any further work begins.

Calculation pipeline and debug checkpoints
------------------------------------------

Every NEC-2 computation for a GN-2 deck passes through these stages in order.
Each stage is assigned a label used as a prefix in every debug output line.

| label  | stage                                       | key quantities                        |
|--------|---------------------------------------------|---------------------------------------|
| GEO    | Segment geometry after GW/GE expansion      | coordinates, lengths, radii           |
| SOM    | somnec table build (called once per freq)   | ck1, ck2, epscf; ar1/ar2/ar3 spot values |
| INTRP  | interpolate_sommerfeld_grid / intrp call    | (x,y) in, (f1..f4) out, grid region  |
| ZMAT   | Z-matrix fill, each element                 | (i,j), real(Z), imag(Z)               |
| RHS    | Right-hand side after EX card               | segment index, V_real, V_imag         |
| CUR    | Current vector after matrix solve           | segment index, I_real, I_imag         |
| IMP    | Final input impedance                       | Z_real, Z_imag                        |

The plan instruments only these stages.  No other code paths are changed.

Debug output format
-------------------

Every debug line follows this pattern so a single `sort | diff` can compare
two engines:

```
STAGE|key1=val1|key2=val2|...
```

Values are printed with `%.15e` for doubles and `%.15e%+.15ej` for complex.
Fields are pipe-separated with no surrounding spaces.  Example:

```
INTRP|call=42|x=4.173071086134553e-02|y=1.295568661409599e+00|igr=0|f1=-5.424291e-01-2.178332e+01j
```

All debug files go under `debugging/deck11/` in the OpenNEC project root.  The
three engines write to separate files; the comparison script reads all three.

File naming convention:

```
debugging/deck11/nec2d_GEO.txt
debugging/deck11/nec2d_SOM.txt
debugging/deck11/nec2d_INTRP.txt
debugging/deck11/nec2d_ZMAT.txt
debugging/deck11/nec2d_CUR.txt
debugging/deck11/nec2c_GEO.txt
... (same set for nec2c and onec)
debugging/deck11/compare_GEO.diff
... (diff outputs)
```

Phase 1: instrument nec2d (the oracle)
---------------------------------------

nec2d is a single Fortran file (`nec2dxs.f`, ~10 500 lines).  Add WRITE
statements conditioned on a preprocessor flag or a logical variable set by an
environment variable read at startup.

### GEO checkpoint

After the geometry is finalized (after the GE card loop), write one line per
segment:

```fortran
WRITE(dbg_unit,'("GEO|seg=",I4,"|x1=",E23.15,"|y1=",E23.15,"|z1=",E23.15,
&               "|x2=",E23.15,"|y2=",E23.15,"|z2=",E23.15,
&               "|len=",E23.15,"|rad=",E23.15)')
&  i, x1(i), y1(i), z1(i), x2(i), y2(i), z2(i), si(i), bi(i)
```

### SOM checkpoint

At the entry and exit of the embedded SOMNEC routine (around line 7000+),
write the five scalar constants and a sample of the first few cells of the
ar1/ar2/ar3 arrays (first 10 entries of each):

```fortran
WRITE(dbg_unit,'("SOM|ck2=",E23.15,"|ck1r=",E23.15,
&               "|ck1=",E23.15,"+",E23.15,"j",
&               "|epscf=",E23.15,"+",E23.15,"j")')
&  ck2, ck1r, DBLE(ck1), DIMAG(ck1), DBLE(epscf), DIMAG(epscf)
```

### INTRP checkpoint

Inside the INTRP subroutine, after the bivariate cubic interpolation returns
the four output values, write one line per call:

```fortran
WRITE(dbg_unit,'("INTRP|call=",I6,"|x=",E23.15,"|y=",E23.15,
&               "|igr=",I1,
&               "|f1=",E23.15,"+",E23.15,"j",
&               "|f2=",E23.15,"+",E23.15,"j",
&               "|f3=",E23.15,"+",E23.15,"j",
&               "|f4=",E23.15,"+",E23.15,"j")') ...
```

Use a `SAVE`d counter variable for the call number.

### CUR checkpoint

After the matrix solve, write each segment current:

```fortran
WRITE(dbg_unit,'("CUR|seg=",I4,"|Ir=",E23.15,"|Ii=",E23.15)')
&  k, DBLE(cur(k)), DIMAG(cur(k))
```

### IMP checkpoint

After the input impedance is computed:

```fortran
WRITE(dbg_unit,'("IMP|Zr=",E23.15,"|Zi=",E23.15)') zr, zi
```

Phase 2: instrument nec2c
--------------------------

nec2c's corresponding functions are:

| nec2d                  | nec2c                           | file           |
|------------------------|---------------------------------|----------------|
| geometry loop          | geometry setup                  | geometry.c     |
| SOMNEC                 | `somnec()`                      | somnec.c       |
| INTRP                  | `intrp()` (now renamed to       | calculations.c |
|                        | `interpolate_sommerfeld_grid()` in onec) |      |
| matrix solve           | LU solve in matrix.c            | matrix.c       |
| current output         | current output in calculations.c | calculations.c |

Add the same labelled `fprintf` lines.  Write to a single file opened as
`debugging/deck11/nec2c_<STAGE>.txt` using a file pointer passed by the
caller, or via a global `FILE *dbg_fp` conditioned on an environment variable
`DBG_DECK11=1`.

Because the Debian 1.3.1 binary does not support environment variables for
this, add the flag as a compile-time `#ifdef DBG_DECK11` block.  Build with
`make CFLAGS="-DDBG_DECK11"`.

The nec2c intrp function is `intrp()` in `calculations.c`.  The nec2c version
uses static sentinels `ixs`, `iys`, `igrs` to cache the last-used grid cell.
The debug print must go **after** the four output values are computed, not
inside the cache branch, so that every actual interpolation call is logged
regardless of cache state.

Phase 3: instrument OpenNEC
----------------------------

The OpenNEC equivalents are:

| nec2d / nec2c          | OpenNEC                                      | file              |
|------------------------|----------------------------------------------|-------------------|
| SOMNEC / somnec()      | `somnec()`                                   | src/somnec.c      |
| INTRP / intrp()        | `interpolate_sommerfeld_grid()`              | src/calculations.c|
| geometry               | geometry setup                               | src/geometry.c    |
| matrix solve           | matrix solve                                 | src/matrix.c      |
| current coefficients   | `compute_current_coefficients()`             | src/calculations.c|

OpenNEC already has a `DEBUG_TRACE_SOMNEC` environment variable hook in
`somnec.c`.  Extend this pattern to all five stages.  Use a single environment
variable `DBG_DECK11=1` that, when set, opens one file per stage in
`debugging/deck11/` at function entry and closes it on completion.

The OpenNEC `interpolate_sommerfeld_grid()` function is at line 641 of
`calculations.c`.  Its signature is:

```c
void interpolate_sommerfeld_grid(context_t *restrict ctx, double x, double y,
    complex double *restrict f1, complex double *restrict f2,
    complex double *restrict f3, complex double *restrict f4);
```

Add a static call counter and print one line per call in the same format as
the nec2d/nec2c versions.

Phase 4: comparison procedure
------------------------------

Run each engine with `DBG_DECK11=1` against `tests/golden_tests/decks/11_gn2_low_height.nec`.

```bash
mkdir -p debugging/deck11

# nec2d (Fortran writes to the files directly when compiled with debug flag)
cd ~/Developer/Nec2dXS_src
gfortran -DDBG_DECK11 -o nec2dxs_dbg nec2dxs.f
./nec2dxs_dbg < /path/to/11_gn2_low_height.nec
# Files land in debugging/deck11/ because paths are hardcoded in the Fortran OPEN statements.

# nec2c (compiled with flag, run from project root so relative paths resolve)
cd ~/Developer/nec2c-1.3
make clean && make CFLAGS="-O2 -DDBG_DECK11"
DBG_DECK11=1 ./nec2c -i /path/to/11_gn2_low_height.nec \
    -o debugging/deck11/nec2c_run.out

# OpenNEC
cd /path/to/OpenNEC
make clean && make RELEASE=1
DBG_DECK11=1 ./onec -i tests/golden_tests/decks/11_gn2_low_height.nec \
    -o debugging/deck11/onec_run.out
```

Then compare stage by stage:

```bash
cd debugging/deck11

diff nec2d_GEO.txt nec2c_GEO.txt   > compare_GEO_nec2d_nec2c.diff
diff nec2d_GEO.txt onec_GEO.txt    > compare_GEO_nec2d_onec.diff

diff nec2d_SOM.txt  nec2c_SOM.txt  > compare_SOM_nec2d_nec2c.diff
diff nec2d_SOM.txt  onec_SOM.txt   > compare_SOM_nec2d_onec.diff

diff nec2d_INTRP.txt nec2c_INTRP.txt > compare_INTRP_nec2d_nec2c.diff
diff nec2d_INTRP.txt onec_INTRP.txt  > compare_INTRP_nec2d_onec.diff

diff nec2d_CUR.txt  nec2c_CUR.txt  > compare_CUR_nec2d_nec2c.diff
diff nec2d_CUR.txt  onec_CUR.txt   > compare_CUR_nec2d_onec.diff
```

The first diff file that is **non-empty** identifies the stage where the
divergence begins.  Since nec2c (restored) and nec2d are expected to agree to
five digits, the nec2d-vs-nec2c diffs should be empty or show only rounding
noise.  The nec2d-vs-onec diffs will point to the broken stage.

Phase 5: expected divergence points and hypotheses
---------------------------------------------------

Based on the known 1.7e-01 error and the near-ground geometry, the most likely
divergence point is one of:

### Hypothesis A: somnec table build

The ar1/ar2/ar3 tables are computed differently.  At 0.02 lambda height the
field point falls in the lowest-r, lowest-theta corner of the first grid
region.  Any difference in how the Romberg integration handles the near-pole
Sommerfeld integrand will propagate to every matrix element.  Check the SOM
checkpoint first.

### Hypothesis B: interpolation region selection

The INTRP/interpolate_sommerfeld_grid function chooses among three overlapping
grid regions based on x (scaled rho) and y (scaled theta).  At close range,
the x-y coordinates place the field point near a region boundary.  If one
engine selects region 0 and another selects region 1, all four Green's function
values will be wrong.  Check the `igr` field in the INTRP checkpoint.

### Hypothesis C: intrp cache sentinel bug (nec2c-specific)

The "Issue #15 fix" that was previously applied to nec2c reset `ixs` and `iys`
when `jump=TRUE`, which is incorrect for this grid layout.  That change has
been reverted in Step 0.  If the restored nec2c still differs from nec2d at
the INTRP stage this hypothesis is ruled out for OpenNEC, but if OpenNEC
carries a similar sentinel-reset logic it may be the same bug applied a
different way.  Search for any code in `interpolate_sommerfeld_grid()` that
resets the ix/iy cache variables when a jump is detected.

### Hypothesis D: Z-matrix ground term sign or normalisation

The nec2c ground term adds `erv`, `ezv`, `erh`, `eph` to the Z-matrix with
factors derived from the segment geometry.  A sign error or missing factor
would shift all off-diagonal elements.  This would show up in the INTRP outputs
being identical across engines but the ZMAT outputs diverging.  Add a ZMAT
checkpoint if GEO, SOM, and INTRP all match.

Stopping criterion
------------------

The debugging work is complete when either:

1. A specific line of OpenNEC code is identified as the divergence origin and a
   correction is proposed that makes `compare_CUR_nec2d_onec.diff` empty or
   reduces the impedance error below 1e-3, or
2. All five checkpoints match between nec2d and OpenNEC but the final impedance
   still differs, which would indicate a post-solve error (unlikely given the
   1.7e-01 scale of the error).

When the fix is confirmed, the deck's entry is moved from `expected_open.json`
into `expected.json` and issue #14 is closed.

Notes on the debugging folder
------------------------------

All output files go to `debugging/deck11/` inside the OpenNEC project root.
Do not write to `/tmp` or to the `tests/` tree.  The `debugging/` folder is
already committed and populated with normal-run output for all 11 decks; the
`deck11/` subdirectory is new and should be created by the first engine run.

The `.diff` comparison files are ephemeral; they do not need to be committed.
The `*_GEO.txt`, `*_SOM.txt`, `*_INTRP.txt`, `*_CUR.txt`, and `*_IMP.txt`
files produced by each instrumented engine are the primary deliverables of this
debugging phase and should be committed when the divergence is identified so
the analysis can be reproduced.
