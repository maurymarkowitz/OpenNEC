## Plan: Measurement Units & GS Refactor

Normalize parsed measurements and inline formulas into `fv[]`/`iv[]` early, then apply GS scaling only to geometry arrays. Preserve raw `f[]/i[]` and unit text for outputs. Insert formula evaluation in `update_card_values()` before unit conversion, validate `ftin` and AWG semantics, and confirm call order (`update_deck_values()` precedes `calculate_geometry()`).

### Steps
1. Evaluate formulas in `update_card_values()` at [src/deck.c#L607], using `card->formulas`, writing to `iv[]/fv[]` before the unit loop [src/deck.c#L612-L640]; reference `te_compile`/`te_eval` [src/tinyexpr.h#L60-L63], [src/tinyexpr.c#L512-L606].
2. Convert units into `fv[]`: use `unit_mult[]` for standard units [src/types.c#L62]; handle `ftin` and AWG cases [src/deck.c#L612-L640], keeping `unit_codes[]` for output text [src/types.c#L57].
3. Confirm order: `update_deck_values()` runs at start of `calculate_geometry()` [src/geometry.c#L62]; geometry reads `iv[]/fv[]` for counts and positions [src/geometry.c#L82-L88], taper/GC reads [src/geometry.c#L111-L119].
4. Maintain GS behavior: `GS` invokes `scale(ctx, xw1)` [src/geometry.c#L159-L160]; `scale()` multiplies wire arrays and patch arrays only [src/geometry.c#L1166-L1188]; do not alter `fv[]`.
5. Preserve raw values in output: keep printing `f[]/i[]` with `unit_codes[...]` [src/output.c#L178-L206]; ensure geometry summaries and derived calculations use `fv[]/iv[]` [src/output.c#L469-L512].
6. Add validation passes: unit parsing flags for inline formulas [src/input.c#L507-L603]; confirm AWG detection for `#n` and `awg` paths; sanity-check `ftin` component handling.

### Further Considerations
1. `ftin` input: enforce feet+inches vs. single-component fallback?
2. AWG normalization: unify `#n` and `awg` to identical conversions.
3. GS scope: keep current behavior, or optionally extend to derived non-geometry values?