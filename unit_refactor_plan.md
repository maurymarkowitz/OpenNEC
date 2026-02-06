# Plan: Refactor Measurements to Use Formula Constants

OpenNEC currently stores units separately from values (e.g., `10` + `mm` unit code). This plan converts to 4nec2's approach where units are formula constants (e.g., `10*mm` where `mm=0.001`), enabling simpler parsing and broader compatibility.

## Steps

### 1. Expand default symbols in deck.c to include all unit conversion constants

**Location**: [deck.c](src/deck.c#L691-L723) `add_default_symbols()`

**Add these constants** (with proper mixed-case notation):
- Length: `m=1`, `cm=0.01`, `mm=0.001`, `ft=0.3048`, `in=0.0254`
- Capacitance: `pF=1e-12`, `nF=1e-9`, `uF=1e-6`
- Inductance: `nH=1e-9`, `uH=1e-6`
- Wire gauge: Create AWG constants `awg0` through `awg40` with radius values from [types.c](src/types.c#L558-L621) `convert_awg_to_meters()` table
  - Use `awg14` notation instead of `#14` (tinyexpr may not support `#` in identifiers)

**Note**: Symbol keys use proper case (e.g., `uF`, `nH`). When building the tinyexpr variable array, symbol names are converted to lowercase since tinyexpr only supports lowercase identifiers. User formulas are also converted to lowercase before evaluation (see Step 3).

**Considerations**:
- Extract unit constants into separate function `add_unit_constants()` called by `add_default_symbols()` for code clarity

### 2. Update data structures to remove old unit system

**Location**: [types.h](src/types.h#L158-L208) `card_t` structure

**Remove**:
- `int units[8]` array (unit indices no longer needed)
- `unit_codes[]` and `unit_mult[]` arrays from [types.c](src/types.c#L69-L79)
- `NUM_ONEC_UNIT_CODES` constant and related definitions

**Keep**:
- `deck_t.unit_val` and `deck_t.unit_typ` fields (these are for external API - see step 8)

### 3. Simplify value parsing in input.c

**Location**: [input.c](src/input.c#L623-L799) `parse_geometry_or_control_card()`

**Changes**:
- Remove unit detection logic that matches against `unit_codes[]`
- Remove AWG special handling (leading `#` or trailing `awg`)
- Parse tokens as: pure number → `f[i]`, OR token with letters → formula
- Remove ftin (feet+inches) parsing - users write `5*ft + 8*in` instead of `5.08ftin`
- Update to treat `10mm` as formula `10*mm` rather than `10` with unit code `2`
- **Preserve exact capitalization in card->formulas**: When parser creates `key_value_t` entries in `card->formulas` list (for inline formulas like `=10MM`), store the original capitalization in the value field
- **Lowercase normalization for tinyexpr**: When evaluating a formula with units, create a lowercase copy of the formula string for tinyexpr compilation (since all unit constants are lowercase). After evaluation, discard the lowercase copy. This happens HERE in the parser, not in tinyexpr or elsewhere.

### 4. Remove unit conversion from deck.c

**Location**: [deck.c](src/deck.c#L987-L1122) `update_card_values()`

**Remove**:
- Entire unit application section that multiplies by `unit_mult[]`
- AWG gauge-to-radius conversion code
- ftin feet+inches conversion logic

**Result**: Formulas like `10*mm` or `10*MM` (lowercased to `10*mm` during evaluation) will evaluate correctly through symbol lookup

### 5. Update AWG preprocessing

**Location**: [deck.c](src/deck.c#L919-L983) `preprocess_awg()`

**Options**:
- Remove function entirely since AWG constants in symbol table handle this
- Or modify to convert `#14` → `awg14` for backward compatibility
- Verify tinyexpr accepts symbol names like `awg14`

### 6. Adjust output functions to preserve original notation

**Location**: [output.c](src/output.c)

**Changes**:
- **Look up formula entry in card->formulas by key**: If formula exists for field (e.g., key 'F3' in formulas list), write the value field exactly as typed: `10MM`, `5ft`, `#14`
- If no formula entry exists (programmatically created card), use numeric value from `fv[j]`
- If `flt_form_inline[j]` is true, write the formula string instead
- Remove old unit suffix appending logic (replaced by formulas list)
- Remove `#` prefix logic (handled by formulas list)
- Preserve user's exact input including capitalization and spacing

### 7. Update test cases and validation

**Create test decks**:
- Formulas using new constants: `GW 1 5 0 0 0 10*mm 0 1*ft awg14`
- Various capitalizations: `10MM`, `10mm`, `10Mm` all evaluate correctly (via lowercase normalization)
- Verify geometry calculations produce identical results to old system
- Test that `10*mm` evaluates to same value as old `10mm` with unit code
- Validate AWG constants give same radius values as `convert_awg_to_meters()` lookup
- **Test round-trip**: Read deck with `10MM`, write it back out, verify `10MM` preserved in `card->formulas`
- Test backward compatibility: old decks with `10mm` notation parse as formula `10*mm`

### 8. Implement GS unit tracking for external API

**Purpose**: External programs can query "what unit system does this deck use?"

**Currently**: `unit_val` and `unit_typ` fields defined but never set

**Implementation**:
- In `parse_deck()` or `calculate_geometry()`, detect if there's exactly one GS card in geometry section
- After formula evaluation, check if `card->fv[1]` matches a known unit conversion factor
- Match against symbol values: if `fv[1]` ≈ `in` constant (0.0254), set `deck->unit_typ` to inches index
- Set `deck->unit_val = card->fv[1]` (the raw scale factor)
- If multiple GS cards or unrecognized value, leave as defaults (`unit_val=1.0`, `unit_typ=0`)
- Create mapping from symbol constants to unit type indices for recognition
- Handle both numeric and formula cases (e.g., `GS 0 0 1/mm` or `GS 0 0 39.3701`)

## Further Considerations

### 1. Symbol name constraints
- **COMPLETE**: Tinyexpr case sensitivity testing confirmed lowercase-only support
- Symbol keys in our code use proper case: `uF`, `nH`, `mm`
- Symbol names converted to lowercase when passed to tinyexpr: `uf`, `nh`, `mm`
- `#` character not supported in identifiers - use `awg14` notation

### 2. Case sensitivity in tinyexpr
TinyExpr only supports lowercase identifiers. Implementation approach:
- **Define symbols with proper case** in symbol table (e.g., key=`"uF"`, value=`1e-6`)
- **Convert symbol names to lowercase** when building tinyexpr variable array
- **Normalize formulas to lowercase** before tinyexpr evaluation (e.g., `10*uF` → `10*uf`)
- **Preserve original casing** in `card->formulas` for output (e.g., store `"10uF"`)
- Users can type any capitalization, internal evaluation uses lowercase

### 3. Formulas list memory management
- The `card->formulas` list infrastructure is already implemented
- Existing cleanup code in card lifecycle handles allocation/deallocation
- Strings are properly freed when cards are destroyed

### 4. Chained symbol definitions
Symbols can reference other symbols, including unit constants. For example:
- `SY mu=in` defines `mu` as alias for the `in` unit constant (0.0254)
- `SY len=10*mu` then references `mu` which references `in`
- This already works correctly via recursive `eval_symbol()` calls in [deck.c](src/deck.c#L760-L920)
- After refactor, will continue working seamlessly
- Unit constants are just symbols like any other, fully composable

### 5. Backward compatibility
Old decks with `10mm` notation will:
- Parse as formula `10*mm` automatically if `mm` symbol exists
- Be stored in `card->formulas` list as `"10mm"` 
- Write back out as `10mm` (exact preservation)
- No breaking change for users

### 6. Additional 4nec2 constants
Research 4nec2 documentation for other built-in constants:
- Wavelength `lambda` or `λ` - requires frequency context from FR card, complex to implement
- Impedance constants `ohm`, `kohm`
- Angle units `deg`, `rad` (degrees/radians)
- Check 4nec2 help files or source code for complete list

### 7. Performance
- Adding ~50-100 constants to symbol table plus storing original strings is negligible memory overhead
- Formula evaluation time unchanged since unit multiplication still occurs (via constant symbols vs explicit code)
- No performance impact expected

### 8. GS card with formulas
After refactor, users can write:
- `GS 0 0 1/mm` (scale to millimeters)
- `GS 0 0 39.3701` (scale to inches)
- `GS 0 0 ft` (scale by feet constant)
- Original format preserved in `card->formulas` list
- The `unit_typ` detection in step 8 needs to handle both numeric and formula cases

### 9. Documentation updates
Update [doc/OpenNEC file format.md](doc/OpenNEC file format.md):
- Document available unit constants (mm, ft, in, awg14, etc.)
- Explain lowercase normalization for unit matching
- Note that original capitalization is preserved in output
- Add examples: `GW 1 5 0 0 0 10*mm 0 1*ft awg14` or `GW 1 5 0 0 0 10MM 0 1FT AWG14`

Update [README.MD](README.MD):
- Remove known limitation about measurements being different from 4nec2

### 10. Error messages
- Update parser error messages that reference unit parsing
- Replace "invalid unit" with "invalid formula" or "unrecognized symbol"

### 11. GUI displays
The `card->formulas` list enables GUIs to show exactly what user typed:
- Display: `10MM` (from formulas list lookup)
- Tooltip: `= 0.01 meters` (from `fv[7]`)
- Preserves user intent and readability

### 12. Programmatic card creation
When code creates cards (not from text), `card->formulas` list will be empty. Output logic needs to:
- Check if formula entry exists for field in `card->formulas` list
- If yes: write original string from value field
- If no: write numeric value from `fv[j]` with appropriate precision
- Or: Allow programmatic code to add entries to `card->formulas` with preferred format

### 13. Format conversion flag
The `-f(ormat)` flag (from TODO) can use `card->formulas` list to support different output styles:
- `--format=original`: Use formulas list entries exactly as typed
- `--format=nec2`: Convert to numeric values only
- `--format=4nec2`: Write as formulas with normalized unit constants
- Supports gradual migration and format conversion

## Implementation Order

1. **Complete**: Tinyexpr case sensitivity testing (confirmed lowercase-only support)
2. Add unit constants to `add_default_symbols()` (proper case: `uF`, `nH`, etc.)
3. Update existing code that builds tinyexpr variable arrays to lowercase symbol names
4. Update parser to add lowercase normalization for formula evaluation
4. Ensure formulas stored with original capitalization in `card->formulas` list
5. Remove unit conversion code from `update_card_values()`
6. Update output functions to use `card->formulas` list
7. Remove `units[]`, `unit_codes[]`, `unit_mult[]` infrastructure
8. Implement GS unit tracking
9. Update tests and documentation
10. Remove or update `preprocess_awg()`

## Testing Strategy

### Unit Tests
- Create test decks with various unit notations
- Verify numeric results match old system
- Test round-trip (read → write → read)
- Test all capitalization variants (normalized internally)

### Integration Tests
- Run against existing test deck collection
- Verify no regressions in calculations
- Test with 4nec2 example models

### Performance Tests
- Measure parsing time before/after
- Measure formula evaluation time
- Verify no degradation

## Risks and Mitigation

### Risk: Tinyexpr doesn't support uppercase
**Mitigation**: Normalize all formulas to lowercase before evaluation, preserve original in `card->formulas` list (confirmed via testing)

### Risk: Breaking changes for external API users
**Mitigation**: Preserve `unit_val` and `unit_typ` fields, implement GS tracking

### Risk: Backward compatibility issues
**Mitigation**: Automatic conversion of old notation to formulas is seamless

### Risk: User confusion about new syntax
**Mitigation**: Update documentation, support both old and new notations during transition

## Success Criteria

- [ ] All unit constants available as formula symbols (proper case in symbol table)
- [ ] Symbol names converted to lowercase when passed to tinyexpr
- [ ] Original user input preserved byte-for-byte in output via `card->formulas` list
- [ ] Existing decks load and calculate identically
- [ ] 4nec2 decks with unit formulas work correctly
- [ ] External API (unit_val/unit_typ) works for simple GS cases
- [ ] No performance degradation
- [ ] Documentation updated
- [ ] Tests pass
