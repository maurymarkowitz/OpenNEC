BUILDING ON WINDOWS (MSYS2 / MinGW-w64)
========================================

This guide shows a minimal path to build OpenNEC on Windows using MSYS2 / MinGW-w64.
It targets the MinGW-w64 GCC/Clang toolchains provided by MSYS2 (64-bit). This
is the fastest path to get a working `onec` CLI without converting the project
to MSVC.

1. Install MSYS2

- Download and install MSYS2 from https://www.msys2.org/
- Open the `MSYS2 MSYS` shell and update the package DB and core packages:

```bash
pacman -Syu
# close the shell, reopen the MSYS2 shell, then:
pacman -Su
```

2. Install the toolchain and common build tools (64-bit target)

```bash
# From MSYS2 shell (or open the 'MSYS2 MinGW 64-bit' shell)
# Install base dev tools and the MinGW-w64 toolchain
pacman -S --needed base-devel mingw-w64-x86_64-toolchain mingw-w64-x86_64-pkg-config
# Optional: install clang
pacman -S mingw-w64-x86_64-clang
```

3. Clone and prepare the repo

```bash
# From MSYS2 MinGW 64-bit shell
git clone <repo-url>
cd OpenNEC
# Optionally create and switch to the compatibility branch
# git checkout -b pc-compatibility
```

4. Build

```bash
# Use the MinGW-w64 shell so 'gcc' and friends are the MinGW versions
make
```

Notes:
- The project uses `clock_gettime` and a small shim is provided in `src/compat.h`.
  `compat_time.c` implements a Windows version relying on `QueryPerformanceCounter`.
- The code assumes a POSIX-like environment for filesystem APIs; MSYS2 provides
  the necessary POSIX layer for most builds. Avoid using MSVC unless you are
  prepared to address C99 and GNU-extension differences.

Testing:

```bash
# Run a quick smoke test using the included example deck
./onec test/example5.deck
```

Troubleshooting:
- If you see missing symbols for threading or pthreads, install `mingw-w64-x86_64-pthreads`/`winpthreads` or prefer building without threading support.
- If `clock_gettime` collisions occur, ensure the `compat_time.c` is being compiled for Windows and that `_WIN32` is defined by the toolchain.

Further work:
- Consider adding a Windows CI job (AppVeyor/GitHub Actions with MSYS2) to routinely test the build.
