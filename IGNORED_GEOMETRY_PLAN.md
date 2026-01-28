# Plan: Support for Segmenting "Ignored" Geometry

To add support for segmenting "ignored" geometry, consider moving ignored items to a dedicated `ignored_geometry` structure rather than mixing them with the main geometry and filtering later. This approach can isolate logic, reduce code changes, and improve clarity.

## Steps
1. Define a new `ignored_geometry` structure in `src/opennec.h` to store ignored geometry items.
2. Update geometry parsing in `src/geometry.c` to add ignored items directly to `ignored_geometry` instead of the main geometry structure.
3. Refactor functions in `src/geometry.c` and related files to process only the main geometry, skipping `ignored_geometry`.
4. Update any logic that previously filtered ignored items from the main geometry to reference `ignored_geometry` as needed (e.g., for reporting or diagnostics).
5. Add or update tests to verify that ignored geometry is handled and segmented separately.

## Further Considerations
- This approach is likely cleaner and requires fewer changes, as it avoids repeated filtering and keeps ignored logic isolated.
- Consider if any downstream code (e.g., output, error reporting) needs to reference or display ignored geometry.
- Ensure memory management and lifecycle of `ignored_geometry` matches the main geometry structure.
