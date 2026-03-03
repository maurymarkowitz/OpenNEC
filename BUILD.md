OpenNEC Build Notes
===================

OpenNEC is designed to build on any system with a working makefile system. It uses standard ANSI-C99, and has no *required* external dependencies. It does not need a `make configure` or other setup, a simple `make` should produce a runnable binary. If it does not, please file bug reports and/or pull requests.

The original NEC-2 used internal code to perform various matrix calculations. OpenNEC allows the optional use of a number of math libraries that can dramatically improve performance on large models, especially those over 1000 segments. These optional components can be specified during `make` using the `BACKEND` directive on the command line.

The following sections describe the backends that OpenNEC supports on different platforms, and how to install and configure them if they are not included by default.

Regression testing
------------------
The `regression` target builds available backends and compares outputs across decks, normalizing timing. Artifacts are saved under test/regression.

```bash
make regression
```

It is highly recommended you run these tests on your platform after building to ensure the different libraries produce the same results.

Linux Setup
-----------
On Linux, you can use OpenBLAS for best performance, MKL, or the reference BLAS/LAPACK libraries. Auto-detection (a bare `make`) on Linux prefers OpenBLAS, then MKL (if `MKL_ROOT` is set), then reference BLAS/LAPACK via pkg-config, and finally the original built-in backend.

- OpenBLAS (Debian/Ubuntu):

```bash
sudo apt update
sudo apt install -y build-essential pkg-config libopenblas-dev
```

- OpenBLAS (RHEL/Fedora):

```bash
sudo dnf install -y make gcc pkgconf-pkg-config openblas-devel
```

- Reference BLAS/LAPACK (Debian/Ubuntu):

```bash
sudo apt update
sudo apt install -y build-essential pkg-config libblas-dev liblapack-dev
```

- Reference BLAS/LAPACK (RHEL/Fedora):

```bash
sudo dnf install -y make gcc pkgconf-pkg-config blas-devel lapack-devel
```

### Build and Test on Linux

Choose a backend and build:

```bash
# original fortran matrix solver
make clean
make BACKEND=original

# OpenBLAS
make clean
make BACKEND=openblas

# reference BLAS/LAPACK
make clean
make BACKEND=blas
```

Run a quick test:

```bash
./onec test/example5.deck
```

### Troubleshooting

- pkg-config missing:
  - Debian/Ubuntu: `sudo apt install -y pkg-config`
  - RHEL/Fedora: `sudo dnf install -y pkgconf-pkg-config`

- OpenBLAS not detected on Linux via pkg-config: verify the `.pc` file exists and is visible to pkg-config.

```bash
pkg-config --modversion openblas
pkg-config --cflags --libs openblas
```

macOS Setup
-----------
On macOS, the default backend is Apple Accelerate. You can also use OpenBLAS on Apple Silicon via Homebrew under `/opt/homebrew`, or Intel machines under `/usr/local`.

- Accelerate (default):

```bash
make clean
make BACKEND=accelerate
./onec test/example5.deck
```

- As Accelerate is the default, this is equivalent to:

```bash
make clean
make
./onec test/example5.deck
```

- OpenBLAS via Homebrew (Apple Silicon):

```bash
# Ensure Apple Silicon Homebrew is installed under /opt/homebrew
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install OpenBLAS and pkg-config
brew install openblas pkg-config

# Build with OpenBLAS
make clean
make BACKEND=openblas
./onec test/example5.deck
```

Important:
- The build strictly rejects Intel Homebrew paths (`/usr/local/opt/openblas`) **on Apple Silicon**. Install OpenBLAS under `/opt/homebrew/opt/openblas`.
- If you prefer Conda, you can install OpenBLAS and pkg-config there:

```bash
conda create -n opennec-oblas -y
conda activate opennec-oblas
conda install -c conda-forge openblas pkg-config -y
make clean
make BACKEND=openblas
```

### Performance Notes

Recent timing runs on macOS (Apple Silicon) using a batch of files on the command line show that OpenBLAS is noticably faster than Accelerate, with the original built-in backend being significantly slower on large models:

- Accelerate: average real ≈ 0.477s across 7 decks
- OpenBLAS: average real ≈ 0.383s across 7 decks
- Original (internal): average real ≈ 3.139s across 7 decks

These times include the time needed to open and parse the deck and write the results, which is generally on the order of 50 to 100 ms. This means the improvement under OpenBLAS is fairly significant, especially when used as a library as opposed to the command line.

### Troubleshooting

- pkg-config missing: `brew install pkg-config`
  
- Homebrew PATH (Apple Silicon): ensure `/opt/homebrew/bin` is on your `PATH` and shell environment is initialized so `pkg-config` can find OpenBLAS.

```bash
# One-time setup
echo 'eval "$(/opt/homebrew/bin/brew shellenv)"' >> ~/.zprofile
eval "$(/opt/homebrew/bin/brew shellenv)"

# Ensure pkg-config can locate OpenBLAS .pc files
export PKG_CONFIG_PATH="/opt/homebrew/lib/pkgconfig:/opt/homebrew/opt/openblas/lib/pkgconfig:${PKG_CONFIG_PATH}"
```

- Builds are blocked on Apple Silicon if OpenBLAS resides under `/usr/local/opt/openblas`, which is the Intel install path. For these machines, install OpenBLAS with Apple Silicon Homebrew in `/opt/homebrew`. If you use Homebrew to install OpenBLAS, and it does not install it in `/opt/homebrew`, you are running the Intel version of the Homebrew code and need to re-install the Apple Silicon version.

- Accelerate deprecation warnings: you may see CLAPACK deprecation notes when using auto-detected Accelerate; these are benign for builds here.

Windows Setup
-------------
There are two practical ways to build and run OpenNEC on Windows:

- Recommended: Windows Subsystem for Linux (WSL)

  1) Install WSL and a Linux distro (e.g., Ubuntu) from the Microsoft Store.
  2) Inside WSL, follow the Linux Setup above (OpenBLAS or reference BLAS/LAPACK).

  ```bash
  # Example with OpenBLAS on Ubuntu (within WSL)
  sudo apt update
  sudo apt install -y build-essential pkg-config libopenblas-dev
  make clean
  make BACKEND=openblas
  ./onec test/example5.deck
  ```

- Native MSYS2 (OpenBLAS)

  1) Install MSYS2 (https://www.msys2.org/) and open an MSYS2 MinGW64 shell.
  2) Install toolchain and OpenBLAS:

  ```bash
  pacman -Syu --noconfirm
  pacman -S --noconfirm mingw-w64-x86_64-toolchain mingw-w64-x86_64-openblas pkgconf
  ```

  3) Build with OpenBLAS:

  ```bash
  make clean
  make BACKEND=openblas
  ./onec test/example5.deck
  ```

### Intel MKL (Advanced)

OpenNEC can link against Intel MKL on Linux/WSL when `MKL_ROOT` points to the MKL install. MKL setup varies by platform; prefer OpenBLAS unless you specifically need MKL.

- Install Intel oneAPI Base Toolkit (includes MKL) and set `MKL_ROOT`.
- Example (Linux/WSL):

```bash
# After installing oneAPI (adjust path to your installation)
export MKL_ROOT="/opt/intel/oneapi/mkl/latest"
make clean
make BACKEND=mkl
./onec test/example5.deck
```

Notes:
- MKL linking flags and library names differ on native Windows with MSVC; this Makefile targets GCC/Unix-like environments. For native Visual Studio builds, a separate project configuration is required.
