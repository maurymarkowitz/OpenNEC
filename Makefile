## Allow user override; auto-detect preferred compiler if CC not set.
ifndef CC
    ifneq ($(shell command -v clang 2>/dev/null),)
        CC := clang
    else ifneq ($(shell command -v gcc 2>/dev/null),)
        CC := gcc
    else
        CC := cc
    endif
endif
$(info Using C compiler: $(CC))
CFLAGS = -I. -Isrc -g -O2 -Wall -Wno-unused-parameter
LDFLAGS =
AR ?= ar
RANLIB ?= ranlib

# Debug build with AddressSanitizer
# Usage: make DEBUG=1
DEBUG ?= 0

ifeq ($(DEBUG),1)
    # Debug builds use -O0 to preserve source-level debugging
    # while still enabling sanitizers.
    CFLAGS := -I. -Isrc -g -O0 -Wall -Wno-unused-parameter
    CFLAGS += -fsanitize=address -fno-omit-frame-pointer
    LDFLAGS += -fsanitize=address
    $(info Building with AddressSanitizer (DEBUG=1); optimization set to -O0)
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
        OPENBLAS_LIBS := $(shell pkg-config --libs openblas)
        LDFLAGS += $(OPENBLAS_LIBS)
        CFLAGS += $(shell pkg-config --cflags openblas) -DHAVE_OPENBLAS
        $(info Building with OpenBLAS via pkg-config)
    endif

    # Ensure lapack symbols get resolved for builds where OpenBLAS may not expose full LAPACK API
    ifeq ($(UNAME_S),Linux)
        LDFLAGS += -llapack -lgfortran
    endif
    $(info OpenBLAS backend: LDFLAGS=$(LDFLAGS))

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

SOURCES = src/main.c src/input.c src/output.c src/deck.c \
          src/deck_validations.c src/card_validation.c src/geometry.c \
          src/calculations.c src/fields.c src/ground.c src/matrix.c \
          src/network.c src/radiation.c src/somnec.c src/misc.c src/types.c \
          src/tinyexpr.c src/control.c \
          src/import-export/maa-support.c src/import-export/yo-support.c \
          src/import-export/nc-support.c src/import-export/nec2-support.c \
          src/import-export/nec4-support.c src/compat_time.c

# compat_time.c is only needed on native Windows (MSVC/clang-cl).
# MinGW CRT already provides clock_gettime, and POSIX platforms have it natively.
# Exclude the file on any non-Windows host to avoid an empty object file.
ifneq ($(OS),Windows_NT)
    SOURCES := $(filter-out src/compat_time.c,$(SOURCES))
endif

# If building with a MinGW cross-compiler, also skip compat_time.c
MINGW_CC := $(findstring mingw,$(CC))
ifeq ($(MINGW_CC),mingw)
    SOURCES := $(filter-out src/compat_time.c,$(SOURCES))
    $(info Detected MinGW toolchain; excluding src/compat_time.c)
    # Prefer the cross-toolchain's archiver, but fall back to plain `ar`/`ranlib`
    # if the prefixed tools are not present in the runner's PATH.
    ifneq ($(shell command -v $(patsubst %gcc,%ar,$(CC)) 2>/dev/null),)
        AR := $(patsubst %gcc,%ar,$(CC))
    else ifneq ($(shell command -v ar 2>/dev/null),)
        AR := ar
    else
        AR := $(patsubst %gcc,%ar,$(CC))
    endif

    ifneq ($(shell command -v $(patsubst %gcc,%ranlib,$(CC)) 2>/dev/null),)
        RANLIB := $(patsubst %gcc,%ranlib,$(CC))
    else ifneq ($(shell command -v ranlib 2>/dev/null),)
        RANLIB := ranlib
    else
        RANLIB := $(patsubst %gcc,%ranlib,$(CC))
    endif
endif

LIB_SOURCES = $(filter-out src/main.c, $(SOURCES))
LIB_OBJECTS = $(LIB_SOURCES:.c=.o)
LIBRARY = libonec.a

EXECUTABLE = onec

all: $(EXECUTABLE)

$(LIBRARY): $(LIB_OBJECTS)
	$(AR) rcs $@ $^
	$(RANLIB) $@

$(EXECUTABLE): src/main.o $(LIBRARY)
	$(CC) src/main.o $(LIBRARY) $(LDFLAGS) -o $@ -lm

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(LIB_OBJECTS) src/main.o $(LIBRARY) $(EXECUTABLE)

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

.PHONY: debug
debug:
	$(MAKE) DEBUG=1

.PHONY: all clean help debug
