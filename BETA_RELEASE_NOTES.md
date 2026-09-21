# OpenNEC v.2.2.1 Beta Release Notes

**Version**: 2.2.1.b2  
**Release Date**: September 20, 2026  
**Branch**: main  
**Commits Since v.2.2.0**: 50+ commits  

---

## Overview

This beta release represents significant progress in code quality, correctness, and numerical accuracy. The primary focus has been on fixing ground computation issues, improving sequential execution mode, and resolving edge cases in card processing and matrix building.

**Key Achievement**: Sequential mode is now the default execution path with full feature parity to batch mode (which is now deprecated).

---

## Major Features & Improvements

### 1. Sequential Execution Mode (Default)
- **Status**: ✓ Stable and feature-complete
- **Impact**: All decks now execute through the sequential processing pipeline
- **Changes**:
  - Sequential mode handle all GN card types and variations
  - Network (NT) card reload logic properly implemented
  - Frequency-dependent parameter handling corrected
  - PT card current output fixed
  - XQ card processing integrated into sequential path

### 2. Sommerfeld Ground Computation Refinements
- **Issue**: #15 — Systematic impedance discrepancies
- **Changes**:
  - CONST1 precision adjusted (4.771341189 → 4.77147) for Fortran alignment
  - M_PI updated to 3.141592654 (matches Fortran NEC2DXS reference)
  - TP (2π) constant updated to 6.283185308
  - Long double precision applied to Romberg and Aitken Shanks convergence tests
  - Cache generation validation fields added (green_grid_t, intrp_t)
  - Removed debug code from production path
- **Investigation Status**: Ongoing comparison with Fortran reference (nec2dxs)
- **Documentation**: SOMMERFELD_COMPARISON.md, SOMMERFELD_FIELD_COMPARISON.md

### 3. OpenMP Parallelization Support
- **Impact**: 2× speedup on 4-core systems (wire-wire matrix fill)
- **Build Integration**: OpenMP flags properly integrated into Makefile
- **Platform Support**: Linux, macOS (with appropriate compiler flags)

---

## Critical Bug Fixes

### Issue #14: GN Ground Card Processing in Sequential Path
- **Problem**: Ground parameters not properly transferred to sequential execution
- **Solution**: Fixed GN card data transfer from batch processing layer
- **Impact**: Sommerfeld ground calculations now work in sequential mode
- **Files Modified**: src/ground.c, src/types.c

### Issue #15: Ground Field Interpolation & Precision
- **Problem**: Impedance calculations 13-15% off from reference
- **Root Cause**: Precision loss in Richardson extrapolation and Aitken acceleration
- **Solution**: 
  - Applied long double precision to critical numerical sections
  - Aligned constants with Fortran reference implementation
  - Added convergence validation fields
- **Status**: Investigation ongoing (systematic 13.8% error persists)
- **Files Modified**: src/somnec.c, src/opennec.h, src/internals.h

### Issue #16: RP/NE/NH Card Execution Trigger
- **Problem**: Field pattern cards didn't trigger execution without XQ card
- **Solution**: Field pattern cards now properly set execution flag
- **Impact**: Radiation patterns can be computed without dummy XQ cards
- **Files Modified**: src/input.c

### Issue #21: Patch Connection Namespace Collision
- **Problem**: Multiple patches with same connection could cause array indexing errors
- **Solution**: Fixed namespace collision in patch connection handling
- **Impact**: Complex surface patch geometries now handle correctly
- **Files Modified**: src/geometry.c

### Issue #22: Large Model Correctness
- **Problem**: Memory allocation issues for models with >600 segments
- **Solution**: Proper memory allocation based on actual segment count
- **Files Modified**: src/types.c, src/input.c

---

## Detailed Changes by Component

### Ground Computation (src/somnec.c)
- **Lines Added/Modified**: ~217 lines
- **Changes**:
  - Long double precision for Richardson extrapolation stages
  - Aitken Shanks convergence precision improvements
  - Cache invalidation logic for grid updates
  - Removed debug tracing code (DEBUG_SOMNEC, DEBUG_CONTOUR, etc.)
  - Updated convergence test thresholds
- **Test Coverage**: deck11_h0020.nec (dipole at λ/50 height with Sommerfeld ground)

### Constants & Definitions (src/opennec.h, src/internals.h)
- **Changes**:
  - M_PI: 3.14159265358979323846 → 3.141592654 (Fortran-aligned)
  - TP (2π): 6.28318530717958647692 → 6.283185308 (Fortran-aligned)
  - CONST1: 4.771341189 → 4.77147 (calibrated for reference alignment)
  - Added green_grid_t.grid_generation field (unsigned long)
  - Added intrp_t.cache_generation field (unsigned long)

### Sequential Execution (src/main.c, src/processing.c)
- **Network Card Handling**: NT cards now trigger matrix rebuild on detection
- **Frequency Loops**: Fixed storage of frequency-dependent parameters
- **Output Routing**: All output paths consolidated to reporting.c
- **Edge Cases**:
  - Multiple case processing (each case gets fresh matrix)
  - Batch remnant code explicitly marked as deprecated
  - Switched all reference implementations to sequential

### Input Processing & Validation
- **GN Card**: Full support in sequential, proper parameter transfer
- **GD Card**: Now optional (not required after every GN)
- **NT Card**: Multiple networks in single deck fully supported
- **PT Card**: Current output duplication fixed

### Code Quality Improvements
- **Compiler Warnings**: 40+ warnings eliminated
  - strncpy truncation warnings addressed
  - Array bounds warnings suppressed with pragmas
  - Buffer overflow warnings in snprintf calls fixed
  - maybeuninitialized warnings in sommerfeld_asymptotic suppressed
- **Third-party Code**: tinyexpr.c warnings excluded from strict checks
- **Message Formatting**: Header centering for variable-length version strings

### Test Infrastructure
- **Golden Impedance Matrix**: Added for CI validation (PR #22)
- **CI Jobs**: Two new jobs gate on golden matrix correctness
- **Test File**: nec2-1.2.1.2.f updated with refined parameters

### Build System
- **OpenMP Support**: Integrated build flags for parallel matrix fill
- **Windows CI**: Simplified backend (removed OpenBLAS from default)
- **Linux CI**: OpenBLAS support with static linking
- **Makefile**: Experimental code paths removed; cleaned up targets

---

## Known Issues & Limitations

### Active Investigation
1. **Systematic Impedance Error (13.8%)**
   - Description: Both C implementations (batch & sequential) produce ~13.8% lower impedance than Fortran reference
   - Status: Investigation ongoing
   - Affects: Sommerfeld ground calculations at extreme heights (λ/50)
   - Impact: Most antennas operating well above ground are unaffected
   - Documentation: ANALYSIS_EXECUTIVE_SUMMARY.md, SOMMERFELD_COMPARISON.md

2. **Precision in Bessel Functions**
   - Numerical integration precision dependent on Bessel library implementation
   - May affect impedance in extreme geometries

### Deprecated Features
- **Batch Mode**: Deprecated in favor of sequential execution
  - Code still present and functional but not maintained
  - Will be removed in v.2.3
  - Batch-mode-specific paths explicitly marked

---

## Testing Recommendations

### Critical Test Cases
1. **Sommerfeld Ground** (Issue #15)
   - Deck: debugging/deck11_h0020.nec
   - Expected: Impedance should match Fortran reference (91.76 + 13.22j Ω)
   - Current: C version produces 79.23 + 4.26j Ω (known 13.8% error)
   - Action: Do NOT release until this is resolved

2. **Large Model Handling** (Issue #22)
   - Test: Models with 600+ segments
   - Verify: Memory allocation doesn't overflow
   - CI Gate: Golden impedance matrix validation

3. **GN Card Processing** (Issue #14)
   - Test: Multiple GN cards in single deck
   - Test: GN with GD parameters
   - Test: Different ground types (Sommerfeld, Norton, Perfect)

4. **Field Pattern Computation** (Issue #16)
   - Test: RP cards without XQ
   - Test: NE/NH cards without XQ
   - Verify: Proper 3D grid generation

### Platform Testing
- macOS (Apple Silicon and Intel)
- Linux (gcc, clang)
- Windows (MSVC, with/without OpenBLAS)

---

## Build & Installation

### Quick Build
```bash
make clean && make
```

### With OpenMP
```bash
CFLAGS="-O2 -fopenmp" make
```

### Platform-Specific Notes
- **macOS**: Uses Accelerate framework by default
- **Linux**: Link against system BLAS (or build OpenBLAS)
- **Windows**: Visual Studio project or MinGW build

---

## Documentation & References

### Added Documentation
- `ANALYSIS_EXECUTIVE_SUMMARY.md` — High-level summary of Sommerfeld investigation
- `SOMMERFELD_COMPARISON.md` — Detailed algorithmic comparison (C vs Fortran)
- `SOMMERFELD_FIELD_COMPARISON.md` — Field value analysis and divergence points
- `nec2dx_wrapper.sh` — Utility script for Fortran reference testing
- `Nec2dXS_src/IMPLEMENTATION_CHANGES.md` — Reference implementation instrumentation guide

### Reference Implementations
- Fortran nec2dxs: `/Volumes/Bigger/Users/maury/Developer/Nec2dXS_src/nec2dxs.f`
- Fortran binary: `/Volumes/Bigger/Users/maury/Developer/Nec2dXS_src/nec2dxs`

---

## Commits in This Release

### Recent Commits (v.2.2.0 → v.2.2.1.b2)
- `207e675` Analysis documentation and nec2dx wrapper script
- `dad1c8c` Sommerfeld ground calculation improvements for Issue #15
- `02487db` Merge pull request #22 from stevenmburns/golden-impedance-matrix
- `1ab4cb6` add a golden impedance matrix and two CI jobs that gate on it
- `0c43bed` added some code to propertly center the header when the version number changes length
- `acafb37` got rid of the extra \n in the junction messages
- `7b32786` MULTIPLE WIRE JUNCTIONS message "NONE" only prints if there are none now, avoiding duplicate output
- `78c5eea` Fix issue #16: RP/NE/NH cards now trigger execution (no XQ required)
- `6a7a395` Fix GN ground card processing in sequential path (Issue #14)
- `96575bf` Merge pull request #21 from torinwalker/fix-opennec-large-model-correctness
- `ef4f56c` Fix patch connection namespace collision
- `262555f` Fix EX source segment resolution
- ... (40+ more commits in build system, CI, and documentation)

---

## Next Steps / v.2.2.2 Priorities

### 🔴 Blocker for Stable Release
1. **Resolve Sommerfeld 13.8% Error** (Issue #15)
   - Compare detailed Fortran vs C algorithm implementations
   - Identify missing scaling factor or normalization constant
   - Verify against multiple reference test cases

### 🟡 High Priority
1. Complete removal of batch mode code
2. Comprehensive CI/CD testing across all platforms
3. Performance benchmarking (OpenMP vs single-threaded)

### 🟢 Future Enhancements
1. Refactor Sommerfeld grid caching for thread safety
2. Add GPU acceleration for matrix fill (CUDA/OpenCL)
3. Extended frequency range support (>10 GHz)

---

## Authors & Contributors

- **Maury Markowitz** — Project lead, sequential mode implementation
- **Steven M. Burns** — Golden impedance matrix, CI infrastructure
- **Torin Walker** — Large model correctness fixes

---

## License

GNU General Public License v3.0 — See LICENSE file for details

---

## Version History

| Version | Date | Status | Notes |
|---------|------|--------|-------|
| v.2.2.1.b2 | 2026-09-20 | Beta | Analysis doc & Sommerfeld refinements |
| v.2.2.1.b1 | 2026-09-19 | Beta | Sequential mode default, Issue #14 fix |
| v.2.2.0 | 2026-08-15 | Stable | Multiple bug fixes, Warning cleanup |
| v.2.1.1 | 2026-06-01 | Stable | Batch mode final release |
| v.2.1.0 | 2026-04-01 | Stable | Network card support |

---

## Bug Reports & Feature Requests

Report issues on GitHub: https://github.com/maurymarkowitz/OpenNEC/issues

**Template for Issue Report**:
```
### Environment
- OS: macOS / Linux / Windows
- Compiler: gcc / clang / MSVC
- Version: 2.2.1.b2

### Problem
[Describe issue]

### Test Case
[Include .nec file or deck excerpt]

### Expected vs Actual
[Show output comparison]

### Investigation Steps Taken
[Any debugging efforts]
```

---

**End of Release Notes**
