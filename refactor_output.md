# Output Functions Refactoring TODO

## Goal
Refactor all fprintf/printf calls in calculation-side files to use dedicated output functions in output.c that read from ctx state.

## NEC Output Sequence (from test/example5.original)

### Already Implemented ✓
1. **Header** - `write_header(FILE *file, nec_context_t *ctx)`
2. **Structure** - `write_structure(FILE *file, nec_context_t *ctx)`
3. **Segments** - `write_segments(FILE *file, nec_context_t *ctx)`
4. **Patches** - `write_patches(FILE *file, nec_context_t *ctx)`
5. **Input Cards Echo** - `write_input_cards(FILE *file, deck_t *deck)`
6. **Frequency Data** - `write_frequency_data(FILE *file, nec_context_t *ctx)`
7. **Loading Data** - `write_loading_data(FILE *file, nec_context_t *ctx)`
8. **Environment Data** - `write_environment_data(FILE *file, nec_context_t *ctx)`
9. **Matrix Timing** - `write_matrix_timing(FILE *file, nec_context_t *ctx)` ✓
   - **NOTE:** Timing capture requires main calculation loop implementation. When implementing the calculation loop, add:
   ```c
   // Before cmset() call:
   double tim1, tim2;
   secnds(ctx, &tim1);
   cmset(ctx, ctx->netcx.neq, cm, rkh, iexk);
   secnds(ctx, &tim2);
   ctx->mat_fill_time = (tim2 - tim1) / 1000.0;  // Convert ms to seconds
   
   // Before factrs() call:
   factrs(ctx, ctx->netcx.npeq, ctx->netcx.neq, cm, ctx->save.ip);
   secnds(ctx, &tim1);
   ctx->mat_factor_time = (tim1 - tim2) / 1000.0;  // Convert ms to seconds
   ```
10. **Network Data** - `write_network_data(FILE *file, nec_context_t *ctx)`

### To Be Implemented

#### Matrix/Network Results
11. **Matrix Asymmetry** - `write_matrix_asymmetry(FILE *file, nec_context_t *ctx)`
    - Transmission line network table
    - Source: `ctx->netcx` network parameters
    - Replaces: Network setup prints in network.c

11. **Matrix Asymmetry** - `write_matrix_asymmetry(FILE *file, nec_context_t *ctx)` ✓
    - Maximum/RMS relative asymmetry
    - Source: `ctx->netcx.asmx`, `ctx->netcx.asa`, segment numbers
    - Replaces: `network()` fprintf at line 168
    - ✓ DONE: Updated network.c to store asymmetry values in ctx

12. **Network Excitation** - `write_network_excitation(FILE *file, nec_context_t *ctx)` ✓
    - Structure excitation data at network connection points
    - Source: `ctx->netcx.exc_*` arrays (voltage, current, impedance, admittance, power)
    - Replaces: `network()` fprintf at lines 442-485
    - ✓ DONE: Updated network.c to store excitation data in ctx arrays

13. **Antenna Input Parameters** - `write_antenna_input_parameters(FILE *file, nec_context_t *ctx)` ✓
    - Input impedance, admittance, power at sources
    - Source: `ctx->netcx.inp_*` arrays (voltage, current, impedance, admittance, power)
    - Replaces: `network()` fprintf at lines 511-585
    - ✓ DONE: Updated network.c to store input data in ctx arrays

14. **Coupling Data** - `write_coupling_data(FILE *file, nec_context_t *ctx)` [SKIP - not in test output]
    - Isolation data between segments
    - Source: `ctx->yparm` coupling parameters
    - Replaces: `couple()` fprintf at lines 152-209

#### Current Distribution
15. **Currents** - `write_currents(FILE *file, nec_context_t *ctx)` ✓
    - Current magnitude and phase at each segment
    - Source: `ctx->crnt.cur[]` current array, `ctx->geometry` for coordinates

16. **Power Budget** - `write_power_budget(FILE *file, nec_context_t *ctx)` ✓
    - Input/radiated/loss power and efficiency
    - Source: `ctx->netcx.pin`, `ctx->netcx.pnls`, `ctx->fpat.ploss`

#### Radiation Patterns
17. **Ground Parameters** - `write_ground_parameters(FILE *file, nec_context_t *ctx)` [SKIP - not in free space test]
    - Far field ground parameters (radial wires, cliff)
    - Source: `ctx->gnd`, `ctx->fpat` structures
    - Replaces: `rdpat()` fprintf at lines 433-488

18. **Radiation Pattern Header** - `write_radiation_pattern_header(FILE *file, nec_context_t *ctx)` ✓
    - Column headers for pattern data
    - Source: `ctx->fpat` pattern settings
    - Replaces: `rdpat()` fprintf at lines 503-530

19. **Radiation Pattern Data** - `write_radiation_pattern_data(FILE *file, nec_context_t *ctx)` [PARTIAL]
    - Field values at each theta/phi point
    - Source: Computed radiation pattern in ctx
    - Replaces: `rdpat()` fprintf at lines 724+
    - **TODO:** Add pattern data storage arrays and populate during calculation

20. **Average Power Gain** - `write_average_power_gain(FILE *file, nec_context_t *ctx)` [PARTIAL]
    - Average gain over solid angle
    - Source: Pattern integration results
    - Replaces: `rdpat()` fprintf at line 806
    - **TODO:** Add average gain calculation and storage

21. **Normalized Gain** - `write_normalized_gain(FILE *file, nec_context_t *ctx)` [PARTIAL]
    - Normalized gain table
    - Source: Computed gain data
    - Replaces: `rdpat()` fprintf at line 818+
    - **TODO:** Add normalized gain data storage and formatting

22. **Footer** - `write_footer(FILE *file, nec_context_t *ctx)` [PARTIAL]
    - Total run time, EN card echo
    - **TODO:** Add runtime tracking

## Implementation Notes

### Keep Inline (Errors/Warnings)
- Segment connection errors (calculations.c lines 927, 1132, 1374)
- Symmetry errors (matrix.c line 1188)
- Loading card errors (calculations.c line 376)
- Step size warnings (calculations.c line 765)

These should remain inline since they indicate fatal/diagnostic conditions.

### Data Storage Requirements
Some output functions will require new fields in ctx to store:
- Matrix timing data
- Asymmetry calculation results
- Radiation pattern computed data

Note: Input cards are available in `deck_t`, not `ctx`.

## File Locations
- **Calculation prints to remove:**
  - src/calculations.c: lines 152, 199, 209, 233, 259, 284, 376, 765, 927, 1132, 1374
  - src/matrix.c: lines 1128, 1188
  - src/network.c: lines 168, 442, 446, 465, 486, 511, 515, 559, 585
  - src/radiation.c: lines 433, 440, 467, 488, 503, 513, 521, 724+

- **Output functions to add:**
  - src/output.c: Implement functions 5-22 above
  - src/proto.h: Add function prototypes