CC = gcc
CFLAGS = -I. -Isrc -g -O2 -Wall -Wno-unused-parameter
LDFLAGS =

# Debug build with AddressSanitizer
# Usage: make DEBUG=1
DEBUG ?= 0

ifeq ($(DEBUG),1)
    CFLAGS += -fsanitize=address -fno-omit-frame-pointer
    LDFLAGS += -fsanitize=address
    $(info Building with AddressSanitizer (DEBUG=1))
endif

# Detect platform
UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)

# Matrix library backend selection
# Usage: make BACKEND=<backend>
# Valid backends: auto, accelerate, openblas, mkl, atlas, blas, original
BACKEND ?= auto

ifeq ($(BACKEND),accelerate)
    # Accelerate framework (macOS only)
    ifneq ($(UNAME_S),Darwin)
        $(error ERROR: Accelerate framework only available on macOS. Current OS: $(UNAME_S))
    endif
    LDFLAGS += -framework Accelerate
    CFLAGS += -DHAVE_ACCELERATE -DACCELERATE_NEW_LAPACK
    $(info Building with Accelerate framework (macOS))
else ifeq ($(BACKEND),openblas)
    # OpenBLAS - check using pkg-config
    OPENBLAS_CHECK := $(shell pkg-config --exists openblas 2>/dev/null && echo yes)
    ifneq ($(OPENBLAS_CHECK),yes)
        # Fallback: try Homebrew OpenBLAS on macOS
        ifeq ($(UNAME_S),Darwin)
            ALT_OPENBLAS_PREFIX := /opt/homebrew/opt/openblas
            ifneq ($(wildcard $(ALT_OPENBLAS_PREFIX)/lib/libopenblas.*),)
                BREW_OPENBLAS_PREFIX := $(ALT_OPENBLAS_PREFIX)
            else
                HOMEBREW_PREFIX := $(shell brew --prefix 2>/dev/null)
                ifneq ($(HOMEBREW_PREFIX),)
                    BREW_OPENBLAS_PREFIX := $(HOMEBREW_PREFIX)/opt/openblas
                else
                    BREW_OPENBLAS_PREFIX := $(shell brew --prefix openblas 2>/dev/null)
                endif
            endif
            ifneq ($(wildcard $(BREW_OPENBLAS_PREFIX)/lib/libopenblas.*),)
                # Prevent arch mismatch on Apple Silicon when using Intel Homebrew paths
                ifeq ($(UNAME_M),arm64)
                    ifneq (,$(findstring /usr/local,$(BREW_OPENBLAS_PREFIX)))
                        $(error ERROR: Detected Intel Homebrew OpenBLAS at $(BREW_OPENBLAS_PREFIX) on Apple Silicon (arm64).\nInstall arm64 OpenBLAS with: brew install openblas (Homebrew prefix under /opt/homebrew) or use Conda:\n  conda create -n opennec-oblas -y && conda activate opennec-oblas && conda install -c conda-forge openblas pkg-config -y)
                    endif
                endif
                LDFLAGS += -L$(BREW_OPENBLAS_PREFIX)/lib -lopenblas
                CFLAGS += -I$(BREW_OPENBLAS_PREFIX)/include -DHAVE_OPENBLAS
                $(info Building with OpenBLAS (Homebrew) at $(BREW_OPENBLAS_PREFIX))
            else
                $(error ERROR: OpenBLAS not found via pkg-config or Homebrew. Install with: brew install openblas pkg-config)
            endif
        else
            $(error ERROR: OpenBLAS not found. Install with: apt install libopenblas-dev (Debian/Ubuntu) or yum install openblas-devel (RHEL/Fedora))
        endif
    else
        LDFLAGS += $(shell pkg-config --libs openblas)
        CFLAGS += $(shell pkg-config --cflags openblas) -DHAVE_OPENBLAS
        $(info Building with OpenBLAS)
    endif
else ifeq ($(BACKEND),mkl)
    # Intel MKL - check for installation
    MKL_ROOT ?= /opt/intel/mkl
    ifneq ($(wildcard $(MKL_ROOT)/lib),)
        LDFLAGS += -L$(MKL_ROOT)/lib -lmkl_rt -lpthread -lm -ldl
        CFLAGS += -I$(MKL_ROOT)/include -DHAVE_MKL
        $(info Building with Intel MKL)
    else
        $(error ERROR: Intel MKL not found at $(MKL_ROOT). Set MKL_ROOT or install MKL)
    endif
else ifeq ($(BACKEND),atlas)
    # ATLAS BLAS - check for libraries
    ATLAS_CHECK := $(shell pkg-config --exists atlas 2>/dev/null && echo yes)
    ifneq ($(ATLAS_CHECK),yes)
        $(error ERROR: ATLAS not found. Install with: apt install libatlas-base-dev (Debian/Ubuntu))
    endif
    LDFLAGS += $(shell pkg-config --libs atlas)
    CFLAGS += $(shell pkg-config --cflags atlas) -DHAVE_ATLAS
    $(info Building with ATLAS)
else ifeq ($(BACKEND),blas)
    # Reference BLAS/LAPACK
    BLAS_CHECK := $(shell pkg-config --exists blas 2>/dev/null && echo yes)
    LAPACK_CHECK := $(shell pkg-config --exists lapack 2>/dev/null && echo yes)
    ifneq ($(BLAS_CHECK),yes)
        $(error ERROR: Reference BLAS not found. Install with: apt install libblas-dev liblapack-dev)
    endif
    ifneq ($(LAPACK_CHECK),yes)
        $(error ERROR: Reference LAPACK not found. Install with: apt install libblas-dev liblapack-dev)
    endif
    LDFLAGS += $(shell pkg-config --libs blas lapack)
    CFLAGS += $(shell pkg-config --cflags blas lapack) -DHAVE_BLAS
    $(info Building with reference BLAS/LAPACK)
else ifeq ($(BACKEND),original)
    # Use original matrix implementation only
    CFLAGS += -DUSE_ORIGINAL_MATRIX
    $(info Building with original matrix implementation)
else ifeq ($(BACKEND),auto)
    # Auto-detect best available library
    ifeq ($(UNAME_S),Darwin)
        # macOS - use Accelerate
        LDFLAGS += -framework Accelerate
        CFLAGS += -DHAVE_ACCELERATE -DACCELERATE_NEW_LAPACK
        $(info Auto-detected: Accelerate framework (macOS))
    else
        # Try OpenBLAS first
        OPENBLAS_CHECK := $(shell pkg-config --exists openblas 2>/dev/null && echo yes)
        ifeq ($(OPENBLAS_CHECK),yes)
            LDFLAGS += $(shell pkg-config --libs openblas)
            CFLAGS += $(shell pkg-config --cflags openblas) -DHAVE_OPENBLAS
            $(info Auto-detected: OpenBLAS)
        else
            # Try Intel MKL
            MKL_ROOT ?= /opt/intel/mkl
            ifneq ($(wildcard $(MKL_ROOT)/lib),)
                LDFLAGS += -L$(MKL_ROOT)/lib -lmkl_rt -lpthread -lm -ldl
                CFLAGS += -I$(MKL_ROOT)/include -DHAVE_MKL
                $(info Auto-detected: Intel MKL)
            else
                # Try reference BLAS/LAPACK via pkg-config
                BLAS_CHECK := $(shell pkg-config --exists blas 2>/dev/null && echo yes)
                LAPACK_CHECK := $(shell pkg-config --exists lapack 2>/dev/null && echo yes)
                ifeq ($(BLAS_CHECK)$(LAPACK_CHECK),yesyes)
                    LDFLAGS += $(shell pkg-config --libs blas lapack)
                    CFLAGS += $(shell pkg-config --cflags blas lapack) -DHAVE_BLAS
                    $(info Auto-detected: reference BLAS/LAPACK)
                else
                    # Fallback to original implementation
                    CFLAGS += -DUSE_ORIGINAL_MATRIX
                    $(info Auto-detected: No optimized libraries found, using original matrix implementation)
                endif
            endif
        endif
    endif
else
    $(error ERROR: Unknown BACKEND=$(BACKEND). Valid options: auto, accelerate, openblas, mkl, atlas, blas, original)
endif

SOURCES = src/main.c src/input.c src/output.c src/deck.c src/deck_validations.c src/card_validation.c src/geometry.c src/calculations.c src/fields.c src/ground.c src/matrix.c src/network.c src/radiation.c src/somnec.c src/misc.c src/types.c src/tinyexpr.c src/control.c src/mma-support.c

LIB_SOURCES = $(filter-out src/main.c, $(SOURCES))
LIB_OBJECTS = $(LIB_SOURCES:.c=.o)
LIBRARY = libonec.a

EXECUTABLE = onec

all: $(EXECUTABLE)

$(LIBRARY): $(LIB_OBJECTS)
	ar rcs $@ $^

$(EXECUTABLE): src/main.o $(LIBRARY)
	$(CC) $(LDFLAGS) src/main.o $(LIBRARY) -o $@ -lm

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(LIB_OBJECTS) src/main.o $(LIBRARY) $(EXECUTABLE) bench_estimate roundtrip_test

# ---- round-trip test ---------------------------------------------------------
# Build the read→parse→write round-trip tester (links libonec.a)
roundtrip_test: test/roundtrip_test.c $(LIBRARY)
	$(CC) $(CFLAGS) test/roundtrip_test.c $(LIBRARY) $(LDFLAGS) -o roundtrip_test -lm

# Build the mma converter utility (links libonec.a and includes mma-support)
maa_convert: test/maa_convert.c $(LIBRARY) src/mma-support.c
	$(CC) $(CFLAGS) test/maa_convert.c src/mma-support.c $(LIBRARY) $(LDFLAGS) -o maa_convert -lm

.PHONY: maa_convert

# Run the round-trip test on all .nec/.deck files in test/
.PHONY: roundtrip
roundtrip: roundtrip_test
	./roundtrip_test $$(find test -maxdepth 1 \( -name '*.nec' -o -name '*.NEC' \) | sort)

# ---- estimate benchmark -------------------------------------------------------
# Build the estimate accuracy benchmarker (links libonec.a, walks 4nec2 examples)
bench_estimate: test/bench_estimate.c $(LIBRARY)
	$(CC) $(CFLAGS) test/bench_estimate.c $(LIBRARY) $(LDFLAGS) -o bench_estimate -lm

# Run the benchmark, then plot the results
# Usage: make benchmark          (uses default CSV / PNG paths)
#        make benchmark ARGS=my.csv
.PHONY: benchmark
benchmark: bench_estimate
	./bench_estimate test/estimate_benchmark.csv
	@echo ""
	@echo "To plot: python3 test/plot_estimate.py test/estimate_benchmark.csv test/estimate_plot.png"

help:
	@echo "OpenNEC Build System"
	@echo "===================="
	@echo ""
	@echo "Usage: make [BACKEND=<backend>] [DEBUG=<0|1>] [target]"
	@echo ""
	@echo "Options:"
	@echo "  DEBUG=1     - Enable AddressSanitizer for memory debugging"
	@echo ""
	@echo "Backends:"
	@echo "  auto        - Auto-detect best available library (default)"
	@echo "  accelerate  - Apple Accelerate framework (macOS only)"
	@echo "  openblas    - OpenBLAS library"
	@echo "  mkl         - Intel Math Kernel Library"
	@echo "  atlas       - ATLAS BLAS library"
	@echo "  blas        - Reference BLAS/LAPACK"
	@echo "  original    - Original NEC-2 verson (no external dependencies)"
	@echo ""
	@echo "Examples:"
	@echo "  make                      # Build with auto-detection"
	@echo "  make BACKEND=accelerate   # Require Accelerate (fails if unavailable)"
	@echo "  make BACKEND=original     # Use original implementation"
	@echo "  make DEBUG=1              # Build with AddressSanitizer"
	@echo "  make debug                # Same as make DEBUG=1"
	@echo "  make clean                # Remove build artifacts"
	@echo ""
	@echo "Current platform: $(UNAME_S)"
	@echo "\nOther targets:"
	@echo "  regression  - Run regression harness across decks and backends"

.PHONY: regression
regression:
	@echo "Running regression harness..."
	@bash test/regression_harness.sh

.PHONY: debug
debug:
	$(MAKE) DEBUG=1

.PHONY: all clean help
