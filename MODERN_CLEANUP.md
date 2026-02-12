# OpenNEC Modernization and Cleanup Plan

This document outlines a roadmap for bringing the OpenNEC codebase up to modern C standards (C11/C17) and improving its suitability as a robust library for modern GUI integrations (e.g., Swift/macOS/iOS).

## 1. API Hardening and Safety

### Const-Correctness
- **Status**: [DONE] 2025-02-11
- **Achievements**:
  - [x] Update `src/output.h`/`.c` with `const` for `nec_context_t` and `deck_t`.
  - [x] Update `src/tests.h`/`.c` with `const` for `nec_context_t` and `deck_t`.
  - [x] Applied `const` to core memory and utility functions in `src/misc.h`/`.c`.
  - [x] Applied `const` to matrix solvers in `src/matrix.h`/`.c`.
  - [x] Applied `const` to pure math getters in `src/calculations.h`/`.c`.
  - [x] Performed global audit: most core calculation functions (fields, ground) utilize the context as a mutable scratchpad/cache and cannot be `const` without a major data architecture refactor.
- **Goal**: Update function signatures in all headers (`input.h`, `output.h`, `geometry.h`, etc.) to use `const` for read-only parameters.
- **Benefit**: Compiler optimizations and prevention of accidental state mutation.

### Opaque Handles
- **Status**: [DONE] 2025-02-11
- **Achievements**:
  - [x] Forward declared `nec_context_t` in `types.h`.
  - [x] Moved full struct definition to `internals.h`.
  - [x] Implemented `nec_create_context()` and `nec_destroy_context()`.
  - [x] Refactored `main.c` (CLI tool) to use new handle API.
- **Benefit**: Hide the implementation details of `nec_context_t` behind an opaque pointer in the public API. Ensure source and binary compatibility when updating internal structures.

### Logging Callbacks
- **Status**: [DONE] 2025-02-11
- **Achievements**:
  - [x] Defined `nec_log_callback_t` for UI event-driven logging.
  - [x] Defined `nec_severity_t` (INFO, WARNING, ERROR, FATAL).
  - [x] Implemented `nec_report()` unified logging helper.
  - [x] Refactored `add_error()` and `add_message()` to trigger callbacks.
  - [x] Added `outputs` message tracking to `nec_context_t` to ensure informational messages reach the `.out` file.
  - [x] Suppressed library `INFO` logs from the console by default while keeping them available for GUI observers.
- **Benefit**: Allows modern GUIs to capture and display output in real-time UI components without disk I/O.

## 2. Modern C Types and Standards

### Standard Integer Types
- **Goal**: Replace generic `int` with explicitly sized types from `<stdint.h>` (e.g., `int32_t`, `size_t`) where appropriate.
- **Benefit**: Improved portability and memory safety across different architectures (ARM64 vs x86_64).

### Loop Optimization (`restrict`)
- **Goal**: Use the `restrict` keyword in core calculation loops in `matrix.c`, `fields.c`, and `calculations.c`.
- **Benefit**: Unlocks advanced compiler auto-vectorization (SIMD) for dramatically improved performance.

### Designated Initializers
- **Goal**: Use C99/C11 designated initializers for all structure initializations.
- **Benefit**: Clearer code and safer initialization of complex structs like `nec_context_t`.

## 3. Mathematics and Physics

### High-Precision Constants
- **Goal**: Standardize on high-precision constants in `opennec.h` and leverage `<math.h>` standard defines (e.g., `M_PI`).
- **Benefit**: Improved numerical stability and consistency with other modern EM simulation tools.

### High-Resolution Timing
- **Goal**: Replace the FORTRAN-style `secnds()` with C11 `timespec_get()` or POSIX `clock_gettime`.
- **Benefit**: Accurate performance profiling on modern multi-core systems.

## 4. Build and Documentation

### CMake Migration
- **Goal**: Move from a static `Makefile` to `CMake`.
- **Benefit**: Easier integration into Xcode/Swift projects, better cross-platform support, and simplified dependency management.

### Doxygen Documentation
- **Goal**: Comment the public API in `opennec.h` using Doxygen syntax.
- **Benefit**: Enables IDE tooltips and automatically generated developer documentation.

## 5. Automated Testing

### Test Framework Integration
- **Goal**: Migrate the internal unit tests in `src/tests.c` to a lightweight framework like **Unity** or **Check**.
- **Benefit**: Better test isolation, detailed failure reporting, and CI/CD compatibility.
