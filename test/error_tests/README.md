# Error Test Cases

This directory contains test deck files that intentionally trigger various error conditions in OpenNEC to verify proper error handling.

## Test Files

### invalid_load_type.deck
Tests the "IMPROPER LOAD TYPE CHOSEN" error in calculations.c load() function.
- Uses LD card with type 9 (invalid, must be 0-5)
- Expected error: "IMPROPER LOAD TYPE CHOSEN, REQUESTED TYPE IS 9"

### load_no_matching_tag.deck
Tests the "LOADING DATA CARD ERROR" in calculations.c load() function.
- Uses LD card with tag 999 that doesn't exist in the geometry
- Expected error: "LOADING DATA CARD ERROR, NO SEGMENT HAS AN ITAG = 999"

### segment_below_ground.deck
Tests the "SEGMENT EXTENDS BELOW GROUND" error in geometry.c connect_segments().
- Wire extends from z=0.1 to z=-0.5 with ground plane enabled (GN 1)
- Expected error: "GEOMETRY DATA ERROR -- SEGMENT X EXTENDS BELOW GROUND"

### segment_in_ground_plane.deck
Tests the "SEGMENT LIES IN GROUND PLANE" error in geometry.c connect_segments().
- Wire lies exactly at z=0 with ground plane enabled (GN 1)
- Expected error: "GEOMETRY DATA ERROR -- SEGMENT X LIES IN GROUND PLANE"

### segment_data_error.deck
Tests the "SEGMENT DATA ERROR" in output.c write_segments().
- Wire has zero radius (bi = 0.0)
- Expected error: "SEGMENT DATA ERROR"

## Running Tests

To run all error tests:
```bash
cd test/error_tests
for f in *.deck; do
    echo "Testing $f"
    ../../onec "$f" 2>&1 | grep -A3 "Error"
done
```

## Error Handling Architecture

As of the error handling refactoring (January 2026), all calculation errors are:
1. Captured in `ctx->errors` (errors_list_t structure)
2. Displayed in main.c after the calculation phase
3. Cause program exit via stop() only from main.c

This allows for:
- Better error messages with context
- Batch processing of multiple files
- Library-style usage without forced exits
- Comprehensive error reporting before exit
