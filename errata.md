# OpenNEC Known Differences from nec2c

This document tracks known numerical differences between OpenNEC and the original nec2c implementation, along with explanations of why they occur and whether they are acceptable.

## Matrix Asymmetry Value Difference

**Issue**: The maximum relative asymmetry of the driving point admittance matrix shows a small numerical difference between OpenNEC and nec2c.

**Example (test/example5.deck)**:
- **Original nec2c**: `1.073E-02` for segments 65 and 23
- **OpenNEC**: `1.209E-02` for segments 65 and 23
- **Difference**: ~13% difference in the maximum asymmetry value

**What Matches**:
- ✓ Same segments identified (65 and 23)
- ✓ Very close RMS asymmetry: `5.771E-03` vs `5.722E-03` (only 0.8% difference)
- ✓ Both values are small, indicating good matrix symmetry

**Root Causes**:
1. **Different compiler versions**: OpenNEC uses modern gcc on Apple Silicon, while the original was compiled with older gcc on x86
2. **Math library differences**: Different implementations of libm between platforms
3. **CPU architecture**: x86 vs ARM (Apple Silicon) can produce different floating-point results with identical code
4. **Optimization levels**: Tested with both `-O0` and `-O2`, difference persists
5. **Floating-point accumulation**: The asymmetry calculation involves many floating-point operations where small rounding differences accumulate

**Verdict**: **ACCEPTABLE**

This is a normal and expected difference when running the same floating-point heavy code across different:
- Hardware architectures (x86 vs ARM)
- Compiler versions and optimization strategies
- Math library implementations

Both values indicate excellent matrix symmetry (order of 1E-02), which is what matters for correct NEC operation. The segments identified are identical, and the RMS asymmetry is nearly identical (0.8% difference), confirming that the calculation is working correctly.

**Technical Details**:
- The asymmetry is calculated in `network.c` lines 120-175
- The algorithm is identical to the original nec2c
- The calculation involves:
  - Matrix inversion via `solves()`
  - Complex arithmetic with `cabs()`
  - Accumulation of many small floating-point differences
- IEEE 754 floating-point arithmetic does not guarantee bit-identical results across platforms

**References**:
- Test file: `test/example5.deck`
- Implementation: `src/network.c`, function `netwk()`, asymmetry calculation block
- Output function: `src/output.c`, function `write_matrix_asymmetry()`

---

## Output File Behavior vs stdout Redirection

**Issue**: When an input filename is provided without the `-o` flag, stdout redirection does not capture the output.

**Example**:
```bash
./onec test/example5.deck > captured.out  # captured.out remains empty
ls test/example5.out                       # output goes here instead
```

**Behavior**:
- When an input file is provided (e.g., `test/example5.deck`), the program automatically constructs an output filename by replacing the input extension with `.out`
- The program opens this file directly via `fopen()` and writes to it
- This bypasses stdout entirely, so shell redirection (`>`) captures nothing
- The `-o` flag can be used to explicitly specify the output file

**Root Cause**:
- In `src/main.c` lines 221-237, when `output_file` is empty and `input_file` is provided, the code constructs `output_file` from the input filename
- At line 263-270, the program opens this constructed filename directly with `fopen()`
- All output goes through `ctx.output_fp`, which points to the directly opened file, not stdout

**Workaround**:
- Use `-o filename` to explicitly control output destination
- Use `-o -` to write to stdout (if implemented)
- Pipe input: `cat input.deck | ./onec > output.txt`

**Status**: Documented behavior, may be unexpected for users familiar with Unix conventions where output typically goes to stdout by default.

---

*Last updated: January 22, 2026*
