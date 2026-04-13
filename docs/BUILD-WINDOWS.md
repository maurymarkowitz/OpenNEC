
BUILDING ON WINDOWS
===================

OpenNEC relies on several features of C99 for its calculations. As Microsoft has not yet (as of 2026) released a C99 compiler, building OpenNEC on that platform requires the use of a 3rd party toolchain. This guide shows how to build OpenNEC on Windows using the MinGW-w64 toolchain (64-bit).

Getting started
---------------

1. Install a MinGW-w64 toolchain

- Option A (recommended): download a standalone toolchain such as the
  Winlibs standalone builds (https://winlibs.com/) or the mingw-w64 installer
  builds. Choose an x86_64 target and the POSIX/SEH variant if offered.
- Option B: use a packaged installer for MinGW-w64 appropriate for your
  environment. Ensure the installed toolchain provides `x86_64-w64-mingw32-gcc`,
  `x86_64-w64-mingw32-ar`, and `x86_64-w64-mingw32-ranlib` (or equivalent prefixed
  binaries) and add the toolchain `bin/` directory to your `PATH`.

2. Prepare the build environment

- Ensure the MinGW-w64 `bin` directory is on your `PATH` so commands like
  `x86_64-w64-mingw32-gcc` and `x86_64-w64-mingw32-ar` are resolvable.
- If you do not have a POSIX `make`, use the shipped `mingw32-make` or a
  GNU `make` that works with MinGW. Example usage below uses the `make`
  invocation the Makefile expects; if your system provides only
  `mingw32-make`, use that name instead.

3. Clone and prepare the repo

Building
--------

On Windows you can use PowerShell or the legacy Command Prompt. If you prefer
`bash` you can install Git for Windows (which provides Git Bash). Examples:

PowerShell:

```powershell
git clone <repo-url>
Set-Location OpenNEC
# optionally: git checkout pc-compatibility
```

cmd.exe (Command Prompt):

```cmd
git clone <repo-url>
cd OpenNEC
REM optionally: git checkout pc-compatibility
```

Git Bash (if installed via Git for Windows):

```bash
git clone <repo-url>
cd OpenNEC
# optionally: git checkout pc-compatibility
```

4. Build (use original matrix backend to avoid optional external libraries)

```bash
# Prefer the MinGW-w64 cross-toolchain binaries on PATH
make CC=x86_64-w64-mingw32-gcc BACKEND=original
```

Notes
-----
- The project includes a small `clock_gettime` shim; on MinGW-w64 this is
  usually provided by the CRT/pthreads implementation. The Makefile excludes
  the bundled `src/compat_time.c` when a MinGW toolchain is detected to avoid
  collisions.
- For most builds you can use `BACKEND=original` to avoid requiring OpenBLAS
  or other numeric libraries. If you need high-performance BLAS on Windows,
  install a native Windows build of OpenBLAS and make sure `pkg-config` and
  the library are available.

Testing
-------

After building you can run a smoke test on Windows natively:

```powershell
.\onec.exe examples\example5.deck
```

Or from a Unix-like shell in the toolchain environment:

```bash
./onec.exe examples/example5.deck
```

Help and shell notes
--------------------
- Preferred: use `-h` or `--help` to show usage on any platform.
- On Windows `cmd.exe`, `onec /?` will also display help (traditional Windows form).
- On Unix-like shells `?` and `*` are shell globs — quote them if you intend to pass a literal `?` (e.g. `onec '?')`.

Troubleshooting
---------------
- If you see missing symbols for threading or pthreads, ensure your MinGW
  distribution includes winpthreads. Some installers provide separate
  runtime packages for pthreads support.
- If `clock_gettime` conflicts occur, the Makefile will normally avoid
  compiling the project's shim for MinGW; ensure you are using the prefixed
  cross-toolchain (e.g. `x86_64-w64-mingw32-gcc`) so the Makefile detection
  works correctly.
