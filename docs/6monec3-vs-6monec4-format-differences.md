# 6monec3.out vs 6monec4.out — Format Difference Analysis

## Purpose
This document compares `tests/6monec3.out` (onec output) against `tests/6monec4.out` (nec2dxs11k.exe reference) and documents every formatting difference found in the Fortran-like output.

## Comparison method
- Files were compared line-by-line.
- Key section anchors were located and compared.
- A focused comparison was made on the first major output block from `STRUCTURE SPECIFICATION` through the first `ANTENNA INPUT PARAMETERS` section.
- Repeated output blocks later in the file exhibit the same formatting differences.

## High-level summary
- There are 18 major diff segments across the two files.
- The first major block differs in length:
  - `6monec4.out`: 170 lines from first `STRUCTURE SPECIFICATION` to first `ANTENNA INPUT PARAMETERS`
  - `6monec3.out`: 151 lines for the same range
- The differences are not only whitespace; they include:
  - blank-line placement
  - section header indentation
  - column heading text
  - column spacing and field alignment
  - numeric formatting in printed rows
- The same style mismatches recur in later repeated output blocks.

## Section-by-section differences

### 1. Comments / blank-line placement
- `6monec4.out` has an extra blank line consisting of spaces after the comment block.
- `6monec3.out` has a pure empty blank line instead.
- This causes the section header lines to shift by one line.

Example:
```text
6monec4.out line 24: "                                                                                                     "
6monec3.out line 24: ""
```

### 2. `STRUCTURE SPECIFICATION` block spacing
- In `6monec4.out`, header and text lines are separated by blank lines in a different pattern than in `6monec3.out`.
- `6monec3.out` starts `- - - STRUCTURE SPECIFICATION - - -` at an earlier line, and the following text appears with fewer blank lines.

### 3. `MULTIPLE WIRE JUNCTIONS` / `SEGMENTATION DATA` spacing
- `6monec4.out` inserts blank lines around these section headers differently than `6monec3.out`.
- The output block is effectively the same data, but `6monec3.out` has fewer blank-line separators.

### 4. `DATA CARD NO.` section alignment
- The reference file `6monec4.out` begins `***** DATA CARD NO.` at line 154.
- `6monec3.out` does not have the same line numbering, but the card content is present earlier in the file.
- No major heading text mismatch was reported here, but the overall block position differs.

### 5. `FREQUENCY` / `INTEGRATION` / `KERNEL` formatting
- In `6monec4.out`, the block looks like this:
  - blank line
  - `APPROXIMATE INTEGRATION EMPLOYED...`
  - blank line
  - `THE EXTENDED THIN WIRE KERNEL WILL BE USED`

- In `6monec3.out`, the same text appears with different indentation and with an extra blank line after it.

### 6. `STRUCTURE IMPEDANCE LOADING` table headings
`6monec4.out` uses the classic NEC-style table headings:
```text
       LOCATION          RESISTANCE   INDUCTANCE  CAPACITANCE       IMPEDANCE (OHMS)     CONDUCTIVITY    TYPE
    ITAG FROM THRU          OHMS        HENRYS       FARADS        REAL      IMAGINARY    MHOS/METER
```
`6monec3.out` uses a different heading layout:
```text
  LOCATION        RESISTANCE  INDUCTANCE  CAPACITANCE     IMPEDANCE (OHMS)   CONDUCTIVITY  CIRCUIT
  ITAG FROM THRU     OHMS       HENRYS      FARADS       REAL     IMAGINARY   MHOS/METER      TYPE
```
- The reference has `TYPE` in the last column only; `6monec3.out` adds an extra `CIRCUIT` heading field before `TYPE`.
- This is a concrete format mismatch.

### 7. `ANTENNA ENVIRONMENT` / `MATRIX TIMING`
- `6monec4.out` and `6monec3.out` differ in indentation for `FREE SPACE`.
- `MATRIX TIMING` lines have different indentation and column spacing.
- Example difference:
  - `6monec4.out`: `FILL=    0.016 SEC.,  FACTOR=    0.000 SEC.`
  - `6monec3.out`: `FILL=   2.109 SEC.  FACTOR=   0.307 SEC.`
- Note: this also indicates the numeric timing values differ between the two runs, not only formatting.

### 8. `ANTENNA INPUT PARAMETERS` header and column labels
This is one of the largest formatting mismatches.

`6monec4.out` header:
```text
  TAG   SEG.    VOLTAGE (VOLTS)         CURRENT (AMPS)         IMPEDANCE (OHMS)        ADMITTANCE (MHOS)      POWER
  NO.   NO.    REAL        IMAG.       REAL        IMAG.       REAL        IMAG.       REAL        IMAG.     (WATTS)
```
`6monec3.out` header:
```text
  TAG   SEG       VOLTAGE (VOLTS)         CURRENT (AMPS)         IMPEDANCE (OHMS)        ADMITTANCE (MHOS)     POWER
  No:   No:     REAL      IMAGINARY     REAL      IMAGINARY     REAL      IMAGINARY    REAL       IMAGINARY   (WATTS)
```
- Differences:
  - `SEG.` vs `SEG`
  - `NO.` vs `No:`
  - `REAL        IMAG.` vs `REAL      IMAGINARY`
  - `ADMITTANCE (MHOS)` field spacing differs
  - The data row alignment does not follow the same fixed-width field layout.

### 9. `ANTENNA INPUT PARAMETERS` numeric row formatting
`6monec4.out` prints the first data row as fixed Fortran-style columns:
```text
     1    15 1.00000E+00 0.00000E+00 2.51266E-02-3.89832E-03 3.88630E+01 6.02950E+00 2.51266E-02-3.89832E-03 1.25633E-02
```
`6monec3.out` prints the same row with different spacing and explicit separators:
```text
    1    15  1.0000E+00  0.0000E+00  2.5127E-02 -3.8983E-03  3.8863E+01  6.0295E+00  2.5127E-02 -3.8983E-03  1.2563E-02
```
- In `6monec4.out`, the real and imaginary parts are placed in adjacent fixed-width fields, sometimes with no separating space if the sign occupies the first column.
- In `6monec3.out`, numbers are printed with additional spaces and different exponent precision.

### 10. `CURRENTS AND LOCATION` section headings
`6monec4.out` uses:
```text
  SEG.  TAG    COORD. OF SEG. CENTER     SEG.            - - - CURRENT (AMPS) - - -
  NO.   NO.     X        Y        Z      LENGTH     REAL        IMAG.       MAG.        PHASE
```
`6monec3.out` uses:
```text
   SEG  TAG    COORDINATES OF SEGM CENTER     SEGM    ------------- CURRENT (AMPS) -------------
   No:  No:       X         Y         Z      LENGTH     REAL      IMAGINARY    MAGN        PHASE
```
- The first label differs: `COORD. OF SEG. CENTER` vs `COORDINATES OF SEGM CENTER`.
- `SEG.` vs `SEGM` and `REAL        IMAG.` vs `REAL      IMAGINARY`.
- The onec file uses `MAGN` instead of `MAG.`.
- The numeric column spacing differs as well.

### 11. Numeric data formatting differences
- `6monec4.out` often prints values with five digits after the decimal and uses older NEC fixed-width conventions.
- `6monec3.out` prints many values with four digits after the decimal and uses explicit field separators.
- Example single value difference:
  - `6monec4.out`: `0.00000`
  - `6monec3.out`: `-0.00000`
- This demonstrates the need to match sign-handling and floating-point formatting exactly.

## Repeating pattern
- The same style of differences appears in later repeated output blocks.
- The `ANTENNA INPUT PARAMETERS` and `CURRENTS AND LOCATION` mismatches are duplicated in the later repeated section.
- Therefore, the required format changes are likely systemic, not isolated to a single block.

## Practical conclusion
To make onec output match `6monec4.out` exactly, the following areas must be fixed:
1. blank-line insertion and blank-line whitespace content
2. section header indentation and blank-line placement
3. `STRUCTURE IMPEDANCE LOADING` heading layout and the `CIRCUIT`/`TYPE` column arrangement
4. `ANTENNA INPUT PARAMETERS` column headings and fixed-field formatting
5. `CURRENTS AND LOCATION` heading labels and current table alignment
6. numeric print formatting, including exponent width, decimal precision, and sign handling of zero

## Notes
- The current files differ in run-specific numeric values as well as in formatting.
- The document focuses on formatting differences, but any exact match will also require handling the numeric field layout precisely.
