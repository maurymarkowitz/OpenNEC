# Plan: Support for Segmenting "Ignored" Geometry

To add support for segmenting "ignored" geometry, consider moving ignored items to a dedicated `ignored_geometry` structure rather than mixing them with the main geometry and filtering later. This approach can isolate logic, reduce code changes, and improve clarity.

## Steps
1. Make a new geometry_t called `ignored_geometry` in ctx to store ignored geometry items.
2. Update geometry parsing in `src/geometry.c` to add ignored items directly to `ignored_geometry` instead of the main geometry structure.
3. Other functions should need no changes, they do not look for ignored_geometry now.
5. Add or update tests to verify that ignored geometry is handled and segmented separately.

## Further Considerations
- Consider if any downstream code (e.g., output, error reporting) needs to reference or display ignored geometry.
- Ensure memory management and lifecycle of `ignored_geometry` matches the main geometry structure.
