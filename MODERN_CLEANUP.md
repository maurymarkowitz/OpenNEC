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
- **Status**: [SKIPPED] 2025-02-12
- **Note**: Decided to maintain generic `int` and `unsigned int` types to stay in keeping with the legacy codebase style.
- **Benefit**: Improved portability and memory safety across different architectures (ARM64 vs x86_64).

### Loop Optimization (`restrict`)
- **Status**: [DONE] 2025-02-12
- **Achievements**:
  - [x] Applied `restrict` to core calculation loops in `matrix.c`, `fields.c`, `calculations.c`, `radiation.c`, `ground.c`, and `network.c`.
  - [x] Updated function signatures in headers (`matrix.h`, `fields.h`, `calculations.h`, `ground.h`, `network.h`) to use `restrict` for improved compiler vectorization.
- **Benefit**: Unlocks advanced compiler auto-vectorization (SIMD) for dramatically improved performance.

### Designated Initializers
- **Status**: [DONE] 2025-02-12
- **Achievements**:
  - [x] Updated `src/tests.c` to use designated initializers for local test structures.
  - [x] Refactored `src/types.c` to use designated initializers for `ggrid_t` and `intrp_t` in `nec_context_init`.
  - [x] Updated `src/deck.c` and `src/input.c` to use designated initializers for `card_t`.
  - [x] Modernized `src/main.c` to use `{0}` initialization for local structures instead of `memset`.
- **Goal**: Use C99/C11 designated initializers for all structure initializations.
- **Benefit**: Clearer code and safer initialization of complex structs like `nec_context_t`.

## 3. Mathematics and Physics

### High-Precision Constants
- **Status**: [DONE] 2025-02-12
- **Achievements**:
  - [x] Standardized on high-precision constants in `opennec.h` and `internals.h` using `M_PI`.
  - [x] Derived physical constants (e.g., `CVEL`, `ETA`) from high-precision base values.
- **Benefit**: Improved numerical stability and consistency with other modern EM simulation tools.

### High-Resolution Timing
- **Status**: [DONE] 2025-02-12
- **Achievements**:
  - [x] Removed legacy `secnds()` and replaced it with `nec_get_time_ms()` using POSIX `clock_gettime(CLOCK_MONOTONIC)`.
  - [x] Standardized internal timing to use microseconds/milliseconds since start of context.
- **Benefit**: Accurate performance profiling on modern multi-core systems.

## 4. Build and Documentation

### CMake Migration
- **Status**: [SKIPPED] 2026-02-12
- **Note**: Decided to remain `make`-based to preserve the simplicity of the existing build system.
- **Benefit**: Easier integration into Xcode/Swift projects, better cross-platform support, and simplified dependency management.

### Doxygen Documentation
- **Status**: [DONE] 2026-02-12
- **Achievements**:
  - [x] Documented [src/opennec.h](src/opennec.h) with high-level module overviews and physical constants.
  - [x] Added Doxygen comments to `card_t` and `deck_t` structures in [src/types.h](src/types.h).
  - [x] Documented context lifecycle and logging callback APIs in [src/types.h](src/types.h).
  - [x] Provided Doxygen headers for [src/input.h](src/input.h) and [src/output.h](src/output.h) including parameter descriptions.
- **Goal**: Comment the public API in `opennec.h` and main headers using Doxygen syntax.
- **Benefit**: Enables IDE tooltips and automatically generated developer documentation.

## 5. Automated Testing

### Test Framework Integration
- **Status**: [SKIPPED] 2026-02-12
- **Note**: The functions in `src/tests.c` are intended for GUI-driven verification rather than standard automated CLI testing.
- **Benefit**: Better test isolation, detailed failure reporting, and CI/CD compatibility.
