# TinyExpr Case Sensitivity Test Results

**Date**: February 6, 2026  
**Test Command**: `./onec --test-tinyexpr`

## Summary

**TinyExpr only supports lowercase variable names.** Any uppercase or mixed-case identifiers fail to compile.

## Test Results

| Test | Description | Result | Status |
|------|-------------|--------|--------|
| 1 | Lowercase `mm` | `10*mm = 0.01` | ✓ PASS |
| 2 | Uppercase `MM` | Compile error at pos 4 | ✗ FAIL |
| 3 | Mixed case `uF` | Compile error at pos 4 | ✗ FAIL |
| 4 | Case distinct `mm` vs `MM` | Compile error at pos 11 | ✗ FAIL |
| 5 | Uppercase `PI` override | Compile error at pos 1 | ✗ FAIL |
| 6 | Complex lowercase | `5*ft + 8*in + 100*mm = 1.8272` | ✓ PASS |

## Implications for Unit Refactoring

### 1. Variable Naming Strategy

**Must normalize all unit identifiers to lowercase before formula evaluation:**

- User types: `10MM`, `5FT`, `100uF`
- Store original: `f_orig[i] = "10MM"`, `f_orig[i] = "5FT"`, `f_orig[i] = "100uF"`
- Convert formula to lowercase: `10*mm`, `5*ft`, `100*uf`
- Define symbols: `mm=0.001`, `ft=0.3048`, `uf=1e-6`

### 2. Unit Constants to Define

All unit constants must be lowercase:

**Length units:**
- `m = 1.0`
- `cm = 0.01`
- `mm = 0.001`
- `ft = 0.3048`
- `in = 0.0254`

**Capacitance units (note: all lowercase):**
- `pf = 1e-12`
- `nf = 1e-9`
- `uf = 1e-6` (NOT `uF`)

**Inductance units:**
- `nh = 1e-9`
- `uh = 1e-6`

**Wire gauge (lowercase, no `#` symbol):**
- `awg0` through `awg40` with radius values from lookup table

### 3. Implementation Requirements

**Parser changes (`input.c`):**
1. Save original token: `card->f_orig[i] = strdup(token)`
2. Convert token to lowercase before formula compilation
3. Remove old unit detection code

**Symbol table (`deck.c`):**
1. Add all unit constants as lowercase symbols
2. Cannot support mixed-case variants like `uF` (must use `uf`)

**Output functions (`output.c`):**
1. Use `f_orig[i]` to write original capitalization
2. Preserves user's exact input: `10MM` written back as `10MM`

### 4. User-Facing Behavior

**Input flexibility:**
- Users can type any capitalization: `10MM`, `10mm`, `10Mm`
- Parser normalizes to lowercase for evaluation: `10*mm`
- Original preserved: output shows exact input `10MM`

**Documentation:**
- Recommend lowercase in examples for clarity
- Note that internal evaluation uses lowercase
- Explain that capitalization is purely cosmetic (preserved but not semantic)

### 5. Backward Compatibility

**Old notation** (`10mm` as value+unit):
- Still works: parser treats as formula `10*mm`
- Symbol `mm=0.001` provides conversion
- No breaking changes

**4nec2 compatibility:**
- 4nec2 decks using `10*mm` work directly
- Decks using `10*uF` must be converted to `10*uf` (lowercase)
- May need preprocessing step or warning for mixed-case

### 6. Edge Cases

**Built-in constants:**
- TinyExpr has lowercase built-ins: `pi`, `e`
- Our lowercase units won't conflict
- `PI` macro in C code is separate from tinyexpr's `pi`

**Special characters:**
- Cannot use `#14` syntax (tinyexpr identifier rules)
- Must use `awg14` notation
- Parser can convert `#14` → `awg14` in preprocessing

**Formula preprocessing:**
- Must lowercase entire formula: `5*FT+8*IN` → `5*ft+8*in`
- Preserve spaces and operators
- Only lowercase alphabetic characters

## Recommended Implementation

1. **Add lowercase unit symbols** to `add_default_symbols()` in [deck.c](src/deck.c)
2. **Create preprocessing function** to lowercase formulas while preserving originals
3. **Store original tokens** in new `char *f_orig[8]` array in `card_t`
4. **Update output** to use `f_orig[]` for user-facing display
5. **Document** that units are case-insensitive (cosmetic only)

## Code Example

```c
// In parser (input.c):
char *original_token = strdup(token);  // Save "10MM"
char *lowercase_token = lowercase(token);  // Convert to "10mm"

card->f_orig[i] = original_token;  // Store for output
// Compile lowercase version for evaluation
te_expr *expr = te_compile(lowercase_token, deck->symbols, num_syms, &err);

// In output (output.c):
if (card->f_orig[i]) {
    fprintf(fp, "%s", card->f_orig[i]);  // Write "10MM" exactly
} else {
    fprintf(fp, "%g", card->fv[i]);  // Write numeric value
}
```

## Conclusion

The tinyexpr library's lowercase-only restriction is **not a limitation** for our use case. By:

1. Normalizing to lowercase for evaluation
2. Preserving original capitalization for output
3. Defining all units as lowercase symbols

We achieve full 4nec2 compatibility while maintaining user input fidelity.

The refactoring plan in [unit_refactor_plan.md](unit_refactor_plan.md) should be updated to reflect this lowercase-only approach.
