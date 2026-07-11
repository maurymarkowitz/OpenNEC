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

# Release build (production binary, smaller size, no debug symbols)
# Usage: make RELEASE=1
RELEASE ?= 0

ifeq ($(DEBUG),1)
    # Debug builds use -O0 to preserve source-level debugging
    # while still enabling sanitizers.
    CFLAGS := -I. -Isrc -g -O0 -Wall -Wno-unused-parameter
    CFLAGS += -fsanitize=address -fno-omit-frame-pointer
    LDFLAGS += -fsanitize=address
    $(info Building with AddressSanitizer (DEBUG=1); optimization set to -O0)
else ifeq ($(RELEASE),1)
    # Release builds omit debug symbols at compile time without using -s (which can be problematic)
    CFLAGS := -I. -Isrc -O2 -Wall -Wno-unused-parameter
    $(info Building release binary (RELEASE=1); debug symbols omitted)
endif

# Detect platform
UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)

# Matrix library backend selection
# Usage: make BACKEND=<backend>
# Valid backends: auto, accelerate, openblas, mkl, atlas, blas, original
BACKEND ?= auto

# Static BLAS linking (Linux only): wraps BLAS/LAPACK/gfortran in -Wl,-Bstatic
# to produce a self-contained binary with no runtime dependency on these shared libs.
# Usage: make BACKEND=openblas STATIC_BLAS=1
STATIC_BLAS ?= 0

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
        # Windows/MINGW64: manually link OpenBLAS without pkg-config
        ifneq (,$(filter MINGW%,$(UNAME_S)))
            MINGW_OPENBLAS_PREFIX ?= /mingw64
            LDFLAGS += -L$(MINGW_OPENBLAS_PREFIX)/lib -lopenblas
            CFLAGS += -I$(MINGW_OPENBLAS_PREFIX)/include/openblas -DHAVE_OPENBLAS
            $(info Building with OpenBLAS (MINGW64 fallback) at $(MINGW_OPENBLAS_PREFIX))
        # Fallback: try Homebrew OpenBLAS on macOS
        else ifeq ($(UNAME_S),Darwin)
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
        ifeq ($(STATIC_BLAS),1)
            # Replace dynamic BLAS flags with a static-wrapped equivalent so the
            # release binary carries its own copies of OpenBLAS/LAPACK/gfortran.
            LDFLAGS := -Wl,-Bstatic -lopenblas -llapack -lgfortran -Wl,-Bdynamic
            $(info Static BLAS: OpenBLAS/LAPACK/gfortran linked statically)
        else
            LDFLAGS += -llapack -lgfortran
        endif
    else ifneq (,$(filter MINGW%,$(UNAME_S)))
        # Windows/MINGW64: the MSYS2 OpenBLAS DLL bundles the Fortran runtime
        # internally, so no separate -lgfortran is needed or available.
        # Static link OpenBLAS and pthread for portability
        ifeq ($(STATIC_BLAS),1)
            # Replace -lopenblas with direct path to static library
            LDFLAGS := $(filter-out -lopenblas,$(LDFLAGS))
            LDFLAGS += $(MINGW_OPENBLAS_PREFIX)/lib/libopenblas.a -Wl,-Bstatic -lpthread -Wl,-Bdynamic
            $(info MINGW64 Static: OpenBLAS and pthread linked statically)
        else
            $(info MINGW64: OpenBLAS bundles Fortran runtime; no extra flags needed)
        endif
    endif
    $(info OpenBLAS backend: LDFLAGS=$(LDFLAGS))

else ifeq ($(BACKEND),mkl)
    # Intel MKL - check for installation via pkg-config or fallback to default path
    MKL_PKG_CHECK := $(shell pkgconf --exists mkl-sdl 2>/dev/null && echo yes)
    ifeq ($(MKL_PKG_CHECK),yes)
        MKL_ROOT ?= $(shell pkgconf --libs-only-L mkl-sdl | cut -c3- | xargs dirname)
    else
        MKL_ROOT ?= /opt/intel/mkl
    endif
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
            # Try Intel MKL via pkg-config or fallback to default path
            MKL_PKG_CHECK := $(shell pkgconf --exists mkl-sdl 2>/dev/null && echo yes)
            ifeq ($(MKL_PKG_CHECK),yes)
                MKL_ROOT ?= $(shell pkgconf --libs-only-L mkl-sdl | cut -c3- | xargs dirname)
            else
                MKL_ROOT ?= /opt/intel/mkl
            endif
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
          src/tinyexpr.c src/control.c src/reporting.c \
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

# Installation paths (configurable via PREFIX variable)
ifeq ($(OS),Windows_NT)
    # Windows installation paths
    PREFIX ?= $(PROGRAMFILES)\OpenNEC
    INSTALL_DIR = $(PREFIX)
    BIN_DIR = $(INSTALL_DIR)\bin
    LIB_DIR = $(INSTALL_DIR)\lib
    DOC_DIR = $(INSTALL_DIR)\doc
    # Windows doesn't use man pages in the same way, but provide the path anyway
    MAN_DIR = $(INSTALL_DIR)\man
else
    # Unix-like installation paths
    PREFIX ?= /usr/local
    INSTALL_DIR = $(PREFIX)
    BIN_DIR = $(INSTALL_DIR)/bin
    LIB_DIR = $(INSTALL_DIR)/lib
    MAN_DIR = $(INSTALL_DIR)/share/man/man1
    DOC_DIR = $(INSTALL_DIR)/share/doc/onec
endif

all: $(EXECUTABLE)

$(LIBRARY): $(LIB_OBJECTS)
	$(AR) rcs $@ $^
	$(RANLIB) $@

$(EXECUTABLE): src/main.o $(LIBRARY)
	$(CC) src/main.o $(LIBRARY) $(LDFLAGS) -o $@ -lm
	@if [ "$(RELEASE)" = "1" ]; then \
		ACTUAL_EXE="$@"; \
		if [ ! -f "$$ACTUAL_EXE" ]; then \
			if [ -f "$$${ACTUAL_EXE}.exe" ]; then \
				ACTUAL_EXE="$$${ACTUAL_EXE}.exe"; \
			elif [ -f "$$${ACTUAL_EXE}.EXE" ]; then \
				ACTUAL_EXE="$$${ACTUAL_EXE}.EXE"; \
			fi; \
		fi; \
		if [ -f "$$ACTUAL_EXE" ]; then \
			echo "Stripping symbols from release binary..."; \
			strip "$$ACTUAL_EXE" || true; \
			echo "Release binary size: $$(du -h "$$ACTUAL_EXE" | cut -f1)"; \
		fi; \
	fi

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(LIB_OBJECTS) src/main.o $(LIBRARY) $(EXECUTABLE)

help:
	@echo "OpenNEC Build System"
	@echo "===================="
	@echo ""
	@echo "Usage: make [BACKEND=<backend>] [DEBUG=<0|1>] [RELEASE=<0|1>] [PREFIX=<path>] [target]"
	@echo ""
	@echo "Options:"
	@echo "  DEBUG=1     - Enable AddressSanitizer for memory debugging"
	@echo "  RELEASE=1   - Build smaller release binary (strips debug symbols)"
	@echo "  PREFIX      - Installation prefix"
	@echo "                Unix/Linux/macOS: default /usr/local"
	@echo "                Windows: default %%PROGRAMFILES%%\OpenNEC"
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
	@echo "Targets:"
	@echo "  all         - Build everything (default)"
	@echo "  install     - Install binaries, library, docs, and man page"
	@echo "  uninstall   - Remove installed files"
	@echo "  clean       - Remove build artifacts"
	@echo "  debug       - Same as make DEBUG=1"
	@echo ""
	@echo "Examples:"
	@echo "  make                      # Build with auto-detection"
	@echo "  make BACKEND=accelerate   # Require Accelerate (fails if unavailable)"
	@echo "  make BACKEND=original     # Use original implementation"
	@echo "  make DEBUG=1              # Build with AddressSanitizer"
	@echo "  make debug                # Same as make DEBUG=1"
	@echo "  make install              # Install to default location"
	@echo "  make PREFIX=/opt install  # Install to /opt (Unix/Linux/macOS)"
	@echo "  make PREFIX=D:\MyApps install  # Install to D:\MyApps (Windows)"
	@echo "  make uninstall            # Remove installed files"
	@echo "  make clean                # Remove build artifacts"
	@echo ""
	@echo "Current platform: $(UNAME_S) / OS: $(OS)"

.PHONY: debug
debug:
	$(MAKE) DEBUG=1

.PHONY: install
ifeq ($(OS),Windows_NT)
install: $(EXECUTABLE) $(LIBRARY)
	@echo Installing OpenNEC to $(INSTALL_DIR)...
	@if not exist "$(BIN_DIR)" mkdir "$(BIN_DIR)"
	@if not exist "$(LIB_DIR)" mkdir "$(LIB_DIR)"
	@if not exist "$(DOC_DIR)\examples" mkdir "$(DOC_DIR)\examples"
	@if not exist "$(MAN_DIR)" mkdir "$(MAN_DIR)"
	copy /Y "$(EXECUTABLE)" "$(BIN_DIR)\"
	copy /Y "$(LIBRARY)" "$(LIB_DIR)\"
	copy /Y "docs\onec.1" "$(MAN_DIR)\"
	@echo.
	@echo Copying documentation...
	@robocopy docs "$(DOC_DIR)\doc" /S /E /Y 2>nul
	@robocopy examples "$(DOC_DIR)\examples" /S /E /Y 2>nul
	@echo.
	@echo Installation complete!
	@echo   Binary: $(BIN_DIR)\$(EXECUTABLE)
	@echo   Library: $(LIB_DIR)\$(LIBRARY)
	@echo   Documentation: $(DOC_DIR)\
	@echo.
	@echo To uninstall, run: make uninstall
else
install: $(EXECUTABLE) $(LIBRARY)
	@echo "Installing OpenNEC to $(INSTALL_DIR)..."
	mkdir -p $(BIN_DIR) $(LIB_DIR) $(MAN_DIR) $(DOC_DIR)/examples
	install -m 755 $(EXECUTABLE) $(BIN_DIR)/
	install -m 644 $(LIBRARY) $(LIB_DIR)/
	install -m 644 docs/onec.1 $(MAN_DIR)/
	cp -r docs $(DOC_DIR)/
	cp -r examples $(DOC_DIR)/
	@echo "Installation complete!"
	@echo "  Binary: $(BIN_DIR)/$(EXECUTABLE)"
	@echo "  Library: $(LIB_DIR)/$(LIBRARY)"
	@echo "  Man page: $(MAN_DIR)/onec.1"
	@echo "  Documentation: $(DOC_DIR)/"
	@echo ""
	@echo "To uninstall, run: make uninstall"
endif

.PHONY: uninstall
ifeq ($(OS),Windows_NT)
uninstall:
	@echo Uninstalling OpenNEC from $(INSTALL_DIR)...
	@if exist "$(BIN_DIR)\$(EXECUTABLE)" del /Q "$(BIN_DIR)\$(EXECUTABLE)"
	@if exist "$(LIB_DIR)\$(LIBRARY)" del /Q "$(LIB_DIR)\$(LIBRARY)"
	@if exist "$(MAN_DIR)\onec.1" del /Q "$(MAN_DIR)\onec.1"
	@if exist "$(DOC_DIR)" rmdir /S /Q "$(DOC_DIR)"
	@echo Uninstall complete!
else
uninstall:
	@echo "Uninstalling OpenNEC from $(INSTALL_DIR)..."
	rm -f $(BIN_DIR)/$(EXECUTABLE)
	rm -f $(LIB_DIR)/$(LIBRARY)
	rm -f $(MAN_DIR)/onec.1
	rm -rf $(DOC_DIR)
	@echo "Uninstall complete!"
endif

.PHONY: all clean help debug install uninstall
