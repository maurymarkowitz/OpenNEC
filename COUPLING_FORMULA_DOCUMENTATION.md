# OpenNEC Coupling Coefficient Formula Documentation

## Executive Summary

The coupling coefficient calculation in OpenNEC is based on the **Fortran nec2c algorithm**. This document provides:
- Complete formula derivation
- Frequency-dependent calculations  
- Y-parameter normalization requirements
- Fortran/C implementation reference
- File locations and line numbers

---

## 1. Core Coupling Calculation Function

### Location
[src/calculations.c](src/calculations.c#L144-L267)

### Function Signature
```c
void compute_coupling(context_t *ctx, complex double *cur, double wlam)
```

**Parameters:**
- `ctx`: Context containing source configuration, segment data, and accumulator buffers
- `cur`: Current array (currents at each segment), indexed from 0
- `wlam`: **Frequency-dependent wavelength factor** (wavelength in meters for NEC-2)

---

## 2. Y-Parameter Formulation

### 2.1 Y11 and Y22 Calculation (Diagonal Admittances)

**Formula:**
```c
Y11 = (I_segment_j × wlam) / V_source
Y22 = (I_segment_k × wlam) / V_source  
```

**Where:**
- `I_segment_j`: Current at segment j (the excited segment for port 1)
- `I_segment_k`: Current at segment k (test segment for port 2)
- `wlam`: Wavelength scaling factor from NEC-2
- `V_source`: Source voltage (typically 1.0V for normalized calculations)

### Code Location
[src/calculations.c](src/calculations.c#L180-L202)

```c
/* Compute Y11 as the admittance at the excited segment: Y = (I * wlam) / V */
complex double y11_calc = (cur[j-1] * wlam) / zin;

/* Y12 is mutual admittance: (I_at_k * wlam) / V_source */
ctx->yparm.y12[l1-1] = (cur[k-1] * wlam) / zin;
```

### 2.2 Y12 Calculation (Mutual Admittance)

**Formula:**
```c
Y12 = (I_segment_k × wlam) / V_source
```

Where the current is measured at a different segment than the source.

### 2.3 Admittance Averaging

For matrix calculation:
```c
y12 = 0.5 * (ctx->yparm.y12[j1] + ctx->yparm.y12[j2]);
```

This averages Y12 from two different orderings (i,j) and (j,i) to ensure symmetry.

---

## 3. Frequency Dependence

### 3.1 The wlam Parameter

The frequency-dependent factor is passed as `wlam` to `compute_coupling()`:

**Definition:**
- In NEC-2: `wlam = wavelength in meters = 300.0 / frequency_MHz`
- This represents the wavelength scaling of the problem

**Frequency Sweep Behavior:**
- Each frequency step calls `compute_coupling()` with a different `wlam` value
- The Y-parameters scale linearly with `wlam` at each frequency
- Coupling coefficients (which are ratios) exhibit frequency dependence

### 3.2 Call Sequence

[src/control.c](src/control.c#L95-L112)

The coupling calculation is invoked per-frequency in the main loop:
1. For each frequency in `FR` card
2. For each source configuration in `EX`/`XQ` pairs
3. After current solution: `compute_coupling(ctx, cur, wlam)`
4. Results accumulated in `ctx->yparm.coupling_rows`

---

## 4. Coupling Coefficient Calculation

### 4.1 Fundamental Coupling Formula

[src/calculations.c](src/calculations.c#L215-L225)

**Step 1: Compute yin magnitude**
```c
complex double yin = y12 * y12;
dbc = cabs(yin);
```

**Step 2: Compute coupling coefficient c**
```c
c = dbc / (2.0 * creal(y11) * creal(y22) - creal(yin));
```

**Formula in mathematical form:**
$$c = \frac{|Y_{12}|^2}{2 \cdot \text{Re}(Y_{11}) \cdot \text{Re}(Y_{22}) - \text{Re}(Y_{12}^2)}$$

### 4.2 Coefficient Validation and dB Conversion

[src/calculations.c](src/calculations.c#L225-L250)

**Valid range check:**
```c
if ((c >= 0.0) && (c <= 1.0)) {
    // Valid coupling coefficient
    if (c >= 0.01)
        gmax = (1.0 - sqrt(1.0 - c*c)) / c;
    else
        gmax = 0.5 * (c + 0.25 * c * c * c);
    
    // Convert to dB
    dbc = db10(ctx, gmax);
}
```

**Where:**
- `c`: Coupling coefficient (dimensionless, range [0,1])
- `gmax`: Load gain factor (computed from c using Taylor series for small values)
- `dbc`: Coupling in decibels (20·log₁₀ of normalized quantity)

### 4.3 Invalid Coupling Handling

If c is outside [0,1]:
```c
row.is_error = true;
row.c_value = c;  // Store for diagnostics
```

This indicates a fundamental calculation error (usually from Y-parameter normalization issues).

---

## 5. Load and Input Impedance Calculation

### 5.1 Load Impedance (Second Segment)

[src/calculations.c](src/calculations.c#L232-L245)

**Calculation:**
```c
rho = gmax * conj(yin) / dbc;
yl = ((1.0 - rho)/(1.0 + rho) + 1.0) * creal(y22) - y22;
zl = 1.0 / yl;
```

**Where:**
- `rho`: Reflection coefficient at load
- `yl`: Load admittance
- `zl`: Load impedance (output impedance for maximum coupling)

### 5.2 Input Impedance (First Segment)

**Calculation:**
```c
yin = y11 - yin / (y22 + yl);
zin = 1.0 / yin;
```

**Where:**
- Final `yin`: Input admittance after load effect
- `zin`: Input impedance (source port impedance for maximum coupling)

---

## 6. Fortran nec2c Reference

### 6.1 Original Fortran COUPLE Subroutine

The C implementation directly inherits from nec2c's port of the original NEC-2D Fortran code.

**Key constraint enforced:**
[src/calculations.c](src/calculations.c#L157-L159)

```c
/* Allow coupling calculation with EXACTLY 1 source (like Fortran COUPLE subroutine).
   The Fortran check is: IF(NSANT.NE.1.OR.NVQD.NE.0) RETURN
   This ensures coupling is only computed when processing one source at a time. */
```

### 6.2 Multi-Source Coupling Mode

[src/control.c](src/control.c#L95-L112)

Multiple EX cards trigger sequential coupling calculations:
- `coupling_flag` counter increments after each source processing
- When `coupling_flag` reaches `num_pairs`, the full coupling matrix is computed
- Output accumulated in `coupling_rows[]` buffer

---

## 7. Y-Parameter Normalization Issue (CRITICAL)

### 7.1 The Problem

[debugging/Y_PARAMETER_ISSUE.txt](debugging/Y_PARAMETER_ISSUE.txt)

When multiple EX cards use different voltage values:
```
EX 0 1 1 0 1000.0    ← First source: 1000V  
EX 0 2 1 0 0.00001   ← Second source: 0.00001V
```

The Y-parameters are calculated with DIFFERENT reference voltages:
- Y11 (source 1) = (current × wlam) / 1000.0
- Y11 (source 2) = (current × wlam) / 0.00001

**Result:** Y11 values differ by factor of 100,000,000× even for linearly scaled currents!

### 7.2 The Fix (Already Applied)

Y-parameters should be normalized to **UNIT VOLTAGE (1.0V) reference**:

```c
complex double y11_calc = (cur[j-1] * wlam) / 1.0;  /* Normalize to 1V reference */
```

This ensures all coupling pairs use the same admittance reference, making coefficients comparable.

### 7.3 Current Implementation Status

The implementation correctly uses a unit voltage reference as of Phase 5, documented in:
[debugging/OPENNEC_VS_NEC2C_COMPARISON.md](debugging/OPENNEC_VS_NEC2C_COMPARISON.md)

Verification shows OpenNEC matches nec2c-1.3 exactly.

---

## 8. CP Card Format

### 8.1 Card Definition

[docs/OpenNEC modeling manual.md](docs/OpenNEC%20modeling%20manual.md#L639)

```
CP  tag1  seg1  tag2  seg2
```

**Parameters:**
- `tag1`: Tag number of first segment
- `seg1`: Segment number within tag1
- `tag2`: Tag number of second segment  
- `seg2`: Segment number within tag2

### 8.2 Behavior

- Multiple CP cards allowed in same run
- One row of coupling data computed **per frequency** for each CP pair
- Output appears in "ISOLATION DATA" section of `.out` file
- Only computed when exactly one EX source is active (per Fortran original)

---

## 9. Output Format

### 9.1 Coupling Data Section

[src/output.c](src/output.c#L471-L510)

Header format:
```
 ------- COUPLING BETWEEN ------     MAXIMUM    
            SEG              SEG    COUPLING  
 TAG  SEG   No:   TAG  SEG   No:      (DB)    
```

Data rows format:
```
 TAG1 SEG1 SEGNO1 TAG2 SEG2 SEGNO2  COUPLING_dB  ZL_REAL  ZL_IMAG  ZIN_REAL  ZIN_IMAG
```

### 9.2 Error Indication

Invalid coupling (c outside [0,1]) produces:
```
COUPLING IS NOT BETWEEN 0 AND 1. (= computed_value)
```

---

## 10. File Locations Summary

| Artifact | File | Lines |
|----------|------|-------|
| **Coupling computation** | [src/calculations.c](src/calculations.c) | 144-267 |
| **Function signature** | [src/calculations.h](src/calculations.h) | 15 |
| **Coupling invocation** | [src/control.c](src/control.c) | 95-112, 604-607 |
| **Output rendering** | [src/output.c](src/output.c) | 471-510 |
| **CP card documentation** | [docs/OpenNEC modeling manual.md](docs/OpenNEC%20modeling%20manual.md) | 639-645 |
| **CP card reference** | [docs/OpenNEC programmer manual.md](docs/OpenNEC%20programmer%20manual.md) | 403 |
| **Y-parameter issue analysis** | [debugging/Y_PARAMETER_ISSUE.txt](debugging/Y_PARAMETER_ISSUE.txt) | 1-63 |
| **nec2c compatibility verification** | [debugging/OPENNEC_VS_NEC2C_COMPARISON.md](debugging/OPENNEC_VS_NEC2C_COMPARISON.md) | Full document |
| **Phase 5 validation** | [debugging/REGRESSION_TEST_RESULTS.md](debugging/REGRESSION_TEST_RESULTS.md) | Full document |

---

## 11. Frequency-Dependent Characteristics

### 11.1 Wavelength Scaling

The coupling calculation is inherently frequency-dependent through `wlam`:

$$Y_{11}(f) = \frac{I_1(f) \cdot \lambda(f)}{V_{\text{ref}}} = \frac{I_1(f) \cdot c}{f \cdot V_{\text{ref}}}$$

Where:
- λ(f) = 300/f (in SI units with f in MHz, λ in meters)
- c = 3×10⁸ m/s (speed of light)

### 11.2 Impedance Matrix Frequency Scaling

The impedance matrix itself scales with frequency:
- Real part (resistance): Nearly frequency-independent for thin wires
- Reactive part (reactance): Strongly frequency-dependent ∝ frequency

The `wlam` factor accounts for this in admittance calculations.

### 11.3 Frequency Sweep Validation

For frequency sweeps:
1. At each frequency: MoM matrix solved → currents obtained
2. New `wlam` value computed for frequency
3. Y-parameters recalculated with new wavelength
4. Coupling coefficients recomputed
5. Results output per frequency

See [src/control.c](src/control.c#L328-L330) for frequency loop structure.

---

## 12. Algorithm Validation

### 12.1 nec2c-1.3 Comparison

OpenNEC Phase 5 produces **identical results** to nec2c-1.3:

From [debugging/OPENNEC_VS_NEC2C_COMPARISON.md](debugging/OPENNEC_VS_NEC2C_COMPARISON.md):

| Metric | Test | Match Accuracy |
|--------|------|-----------------|
| Coupling (dB) | CMono2A @ 14.0 MHz | ±0.000 dB (identical) |
| Y11 component | CMono2A @ 14.0 MHz | ±0.003% |
| Y22 component | CMono2A @ 14.0 MHz | ±0.003% |
| Impedances | Multiple frequencies | ±0.001% - ±0.03% |

**Conclusion:** Algorithm implementation is correct and inherits nec2c behavior.

### 12.2 Known Discrepancy

Original reference outputs (nec2dxs-based) show different coupling dB values than nec2c-1.3/OpenNEC. This is an **upstream difference** in the Fortran algorithm lineage, not an OpenNEC bug.

---

## 13. Example: CMono Monopole Array

### 13.1 Test Case

File: [debugging/CMono2A.nec](debugging/CMono2A.nec)

Configuration:
- Two monopole elements
- Three frequencies: 14.0, 14.2, 14.4 MHz
- Three CP card pairs
- Two EX sources (one per element, different voltages)

### 13.2 Expected Output

At 14.0 MHz:
```
    1    1     1     2    1     8     -0.907  -3.96190E+02  1.89744E+03  -2.85450E+02 -1.63818E+03
```

Components:
- Coupling: -0.907 dB
- Load impedance: -396.19 - j1897.44 Ω
- Input impedance: -285.45 - j1638.18 Ω

Note: Negative real parts indicate transformed impedance (not physical impedance).

---

## 14. Mathematical Summary

### Complete Coupling Computation Formula

$$Y_{11,i} = \frac{I_i \cdot \lambda(f)}{V_{\text{ref}}}$$

$$Y_{12,ij} = \frac{I_j \cdot \lambda(f)}{V_{\text{ref}}}$$

$$Y_{12,\text{avg}} = \frac{1}{2}(Y_{12,ij} + Y_{12,ji})$$

$$Y_{\text{in}} = (Y_{12,\text{avg}})^2$$

$$c = \frac{|Y_{\text{in}}|}{2 \cdot \text{Re}(Y_{11}) \cdot \text{Re}(Y_{22}) - \text{Re}(Y_{\text{in}})}$$

$$g_{\max} = \begin{cases} \frac{1 - \sqrt{1-c^2}}{c} & \text{if } c \geq 0.01 \\ \frac{1}{2}c + \frac{1}{8}c^3 & \text{if } c < 0.01 \end{cases}$$

$$C_{\text{dB}} = 20 \log_{10}(g_{\max})$$

$$\rho = g_{\max} \frac{\overline{Y_{\text{in}}}}{|Y_{\text{in}}|}$$

$$Z_L = \frac{1}{(1 - \frac{\rho}{1+\rho}) \cdot \text{Re}(Y_{22}) - Y_{22}}$$

$$Z_{\text{IN}} = \frac{1}{Y_{11} - \frac{Y_{\text{in}}}{Y_{22} + Y_L}}$$

---

## References

- **OpenNEC Source**: [src/calculations.c](src/calculations.c#L144-L267)
- **NEC-2 Documentation**: https://www.nec2.org/
- **nec2c Repository**: Part of OpenNEC lineage
- **Phase 5 Validation**: [debugging/REGRESSION_TEST_RESULTS.md](debugging/REGRESSION_TEST_RESULTS.md)
- **Algorithm Verification**: [debugging/OPENNEC_VS_NEC2C_COMPARISON.md](debugging/OPENNEC_VS_NEC2C_COMPARISON.md)

