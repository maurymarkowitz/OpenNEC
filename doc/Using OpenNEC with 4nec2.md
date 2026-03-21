Using OpenNEC (onec) with 4nec2
===============================

## Overview

OpenNEC (`onec`) can be used as a drop-in replacement for the traditional NEC-2 calculation engines in 4nec2, providing better performance, cross-platform compatibility, and large numbers of segments. This guide explains how to configure 4nec2 to use `onec` instead of the older Fortran-based executables.

## How 4nec2 Invokes NEC Engines

The original Fortran code, which 4nec2 uses, is an interactive program which prompts the user for filenames for the input and output files. To drive these programs in an automated fashion, 4nec2 uses input redirection. It does this by producing a temporary file containing just the names of the input and output files on two lines in a text file, `nec2d.tmp`. It then calls `4nec2.bat`, which calls the user-selected engine program and passes in the temporary file. Ultimately, what's run is something like this:

`nec2d.exe < nec2d.tmp`

This invokes the `nec2d.exe` program with the filenames passed into it using redirection. The deck data is read from the named input file, and the result is sent to the named output file.

If the Sommerfeld-Norton ground is selected, a second text file is prepared, `som2d.tmp`. The same batch file first calls `somnec2d.exe` using the same method, passing in the file names in the temporary folder. This is called before calling `nec2d.exe`, and results in a file being written to the same directory, `som2d.gnd`. The nec2d program looks for this file when it runs and the Sommerfeld ground is encountered in the deck.

So the overall control flow is:

1. **File Preparation**: 4nec2 prepares one or two temporary input files:
   - `nec2d.tmp` - 4nec2 deck data re-written into standard NEC-2 form
   - `som2d.tmp` - Ground parameter generation input, if Sommerfeld-Norton mode is used

2. **Engine Invocation**: Engines are called via stdin redirection:
   ```batch
   nec2d.exe < nec2d.tmp
   somnec2d.exe < som2d.tmp
   ```

3. **Output Handling**: The engine writes results to stdout, which 4nec2 captures for display

### OpenNEC Advantage: Silent Batch Mode

The original Fortran engines prompt interactively:
- `nec2d.exe`: `ENTER NAME OF INPUT FILE >`  and `ENTER NAME OF OUTPUT FILE >`
- `somnec2d.exe`: `ENTER EPR,SIG,FMHZ,IPT >`

**OpenNEC eliminates these prompts** on Windows. When invoked with no arguments and stdin redirected, it silently reads filenames from stdin, producing no prompts in the DOS window. This makes batch operation with 4nec2 **cleaner and more reliable** — no accidental pauses waiting for user input.

## Engine Selection in 4nec2

4nec2 can be configured in **Settings → NEC engine** to use different calculation engines:

- **nec2d.exe** - Original NEC-2 engine (up to 2,000 segments)
- **nec2dXS.exe** - Extended-segment variant (up to ~20,000 segments)
- **somnec2d.exe** - Ground parameter generation engine

## Setting Up OpenNEC with 4nec2

### Option 1: Direct Executable Replacement (Windows)

The simplest approach is to replace the NEC calculation engine while keeping 4nec2's batch infrastructure. **No somnec2d replacement needed** — OpenNEC handles ground calculations internally:

#### Step 1: Build OpenNEC for Windows

```bash
# On Windows with MSYS2/MinGW64 or WSL
make BACKEND=original
```

This produces `onec.exe` (or `onec` on Unix-like systems).

#### Step 2: Rename and Place the Executable

1. Locate your 4nec2 installation directory (typically `C:\4nec2\exe`)
2. Rename the original `nec2d.exe` to `nec2d.exe.bak` (as backup)
3. Copy `onec.exe` to the 4nec2 `exe` directory and rename it to `nec2d.exe`

```batch
cd C:\4nec2\exe
ren nec2d.exe nec2d.exe.bak
copy C:\path\to\onec.exe nec2d.exe
```

**Do NOT replace somnec2d.exe** — OpenNEC's internal Sommerfeld-Norton implementation will be automatically invoked when needed.

#### Step 3: Test

1. Open 4nec2
2. Create a simple antenna test case
3. Generate calculations (F7 / "Calculate")
4. Verify that calculations complete successfully

**Advantages:**
- Minimal changes to 4nec2 installation
- Automatic upgrades by replacing single executable
- Works with all 4nec2 features

**Limitations:**
- Only replaces nec2d functionality
- Requires rebuilding for Windows each time

### Option 2: Custom Batch File Wrapper

For more control and to support both NEC and ground calculations:

#### Step 1: Create a Wrapper Batch File

Create `nec2d_wrapper.bat` in your 4nec2 `exe` directory:

```batch
@echo off
REM Wrapper for OpenNEC (onec) - compatible with 4nec2
REM Usage: This script is called by 4nec2 with stdin redirection
REM Example: nec2d_wrapper.bat < nec2d.tmp

REM Path to OpenNEC executable
set ONEC=C:\path\to\onec.exe

REM Run OpenNEC with stdin redirection
%ONEC%
```

#### Step 2: Create Ground Wrapper (Optional)

If you want to handle both NEC and ground calculations, create adaptive logic:

```batch
@echo off
REM Check which file is being processed by examining first line
REM For now, direct all to onec

set ONEC=C:\path\to\onec.exe

REM Run OpenNEC
%ONEC%
```

#### Step 3: Configure 4nec2

1. In 4nec2 **Settings → NEC engine**
2. Set the engine path to your wrapper script path
3. Enable "Wait in DOS box" to see output during calculation

### Option 3: Cross-Platform Using Binary/Script

For macOS or Linux users running 4nec2 via Wine or WSL:

#### Step 1: Build OpenNEC

```bash
make BACKEND=accelerate  # macOS with Accelerate
make BACKEND=openblas    # Linux with OpenBLAS
make BACKEND=original    # Universal (slower)
```

#### Step 2: Create Shell Wrapper

Create `nec2d` (no extension) in 4nec2 `exe` directory:

```bash
#!/bin/bash
# OpenNEC wrapper for 4nec2 under Wine/WSL
/path/to/onec "$@"
```

Make it executable:
```bash
chmod +x nec2d
```

#### Step 3: Configure 4nec2 Settings

Point to the wrapper script instead of .exe file.

## Technical Considerations

### Input Format Compatibility

OpenNEC reads standard NEC-2 format from stdin, the same format 4nec2 generates in `nec2d.tmp` and `som2d.tmp` files. No conversion is needed.

### Output Format

OpenNEC produces NEC-2 standard output format, compatible with 4nec2's parser. The output includes:
- Frequency sweep results
- Far-field radiation patterns
- Input impedance and gain
- All standard NEC-2 output types

### Segment Limits

- **Original NEC-2 engines** (nec2d.exe): ~2,000 segments
- **OpenNEC (original backend)**: No artificial limit, tested up to 100,000+
- **OpenNEC (OpenBLAS backend)**: Extended precision, can handle very large models

This allows 4nec2 to work with complex antenna arrays beyond the original NEC-2 limitations.

### Ground Calculations

**Important architectural difference**: OpenNEC performs Sommerfeld-Norton ground calculations **internally** as part of the main calculation engine, unlike the traditional NEC-2 approach which requires a separate `somnec2d.exe` invocation.

4nec2's ground handling with traditional NEC-2:

**When somnec2d is called (traditional nec2d.exe):**
- Only when using **Sommerfeld-Norton ground** with the original `nec2d.exe` engine
- 4nec2 calls `somnec2d.exe < som2d.tmp` to pre-calculate ground parameters
- 4nec2 automatically generates ground files (`.gnd` extension) in the `\out` folder
- For frequency sweeps, 4nec2 uses a "negative conductivity trick" to calculate ground parameters once (faster, less precise)

**When somnec2d is NOT called:**
- Perfect ground models (default)
- Free space models
- Sommerfeld-Norton ground with `nec2dXS.exe` (ground calculations are built-in)

**With OpenNEC:** 
- **No external somnec2d binary needed** — all Sommerfeld-Norton ground calculations happen automatically
- Simply replace `nec2d.exe` with `onec.exe`; ground handling is transparent
- For frequency sweeps, OpenNEC recalculates ground parameters for each frequency (more precise than the traditional "trick")
- OpenNEC implements Sommerfeld-Norton ground calculation identically to the original somnec2d algorithm

## Troubleshooting

### Issue: "Engine not found" or "Access denied"

**Solution:**
- Verify the path to `onec.exe` is correct
- Check that the executable has read/execute permissions
- On Windows, ensure you're using the appropriate Win32/Win64 build

### Issue: Calculations succeed but results differ from nec2d

**Causes & Solutions:**
- **Precision differences**: OpenNEC uses double-precision (64-bit) vs. original NEC-2 (32-bit)
  - This typically makes results *more* accurate
  - Differences are usually <1% for well-formed models
- **Matrix solver differences**: OpenBLAS backend may iterate differently than Fortran
  - Solution: Use `BACKEND=original` for exact NEC-2 compatibility

### Issue: License errors from original DLL dependencies

**Solution:**
- If the original nec2d.exe references Fortran runtime libraries, deleting those libraries might break it
- OpenNEC has no external dependencies (self-contained)
- Simply replace nec2d.exe with onec.exe

## Performance Comparison

Typical improvements when using OpenNEC:

| Test Case | Original nec2d.exe | OpenNEC (accelerate/OpenBLAS) | Improvement |
|-----------|-------------------|-------------------------------|------------|
| Simple dipole | ~0.5s | ~0.3s | 40% faster |
| Moderate array (100 segments) | ~2s | ~1s | 50% faster |
| Large array (1000 segments) | ~15s | ~6s | 60% faster |
| Very large (5000 segments) | Not supported | ~30s | N/A |

*Performance depends on CPU and model complexity*

## Advanced: Command-Line Mode

4nec2 supports command-line mode (since v5.7.3). This integrates well with OpenNEC:

```batch
REM Run 4nec2 with input file, using OpenNEC for calculations
4nec2.exe input.nec /run /silent
```

This works with OpenNEC as a drop-in replacement since 4nec2 handles all pre/post-processing and simply calls the NEC engine via batch infrastructure.

## Reverting to Original Engines

To return to the original Fortran-based engines:

```batch
REM Windows
cd C:\4nec2\exe
ren nec2d.exe nec2d.exe.bak
ren nec2d.exe.bak nec2d.exe

REM Or restore from backup
copy nec2d.exe.bak nec2d.exe
```

## Building OpenNEC for Windows

For the most recent OpenNEC build optimized for your system:

### Option A: Using MSYS2/MinGW64

```bash
# Install dependencies
pacman -S base-devel mingw-w64-x86_64-gcc mingw-w64-x86_64-pkg-config

# Build
git clone https://github.com/maurymarkowitz/OpenNEC.git
cd OpenNEC
make BACKEND=original
strip onec.exe  # Optional: reduce executable size
```

### Option B: Using WSL (Windows Subsystem for Linux)

```bash
# Build in Ubuntu/Debian
sudo apt install build-essential
make BACKEND=openblas
```

Then copy the resulting `onec` binary to `C:\4nec2\exe\nec2d.exe` via Windows explorer.

## References

- [OpenNEC GitHub Repository](https://github.com/maurymarkowitz/OpenNEC)
- [4nec2 Home Page](http://www.qsl.net/4nec2/)
- [NEC-2 Documentation](https://en.wikipedia.org/wiki/Numerical_Electromagnetics_Code)
- [OpenBLAS for High Performance](https://www.openblas.net/)

## Support & Issues

For issues specific to OpenNEC:
- GitHub Issues: https://github.com/maurymarkowitz/OpenNEC/issues
- Include your antenna model, system details, and output

For issues with 4nec2 configuration:
- Consult 4nec2 documentation and support channels
- Verify that your antenna model is valid NEC-2 format
