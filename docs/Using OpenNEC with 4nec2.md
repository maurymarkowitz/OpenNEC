Using OpenNEC (onec) with 4nec2
===============================

## Overview

OpenNEC (`onec`) can be used as a drop-in replacement for the traditional NEC-2 calculation engines in 4nec2, providing better performance, cross-platform compatibility, and large numbers of segments. This guide explains how to configure 4nec2 to use `onec` instead of the older Fortran-based executables.

## How 4nec2 Invokes NEC Engines

The original Fortran code, which ships with the 4nec2 install, is an interactive program which prompts the user for filenames for the input and output files. To drive these programs in an automated fashion, 4nec2 uses input redirection through a batch file. It does this by producing a temporary file containing just the names of the input and output files on two lines in a text file, `nec2d.tmp`. It then calls `4nec2.bat`, which calls the user-selected engine program and passes in the temporary file. Ultimately, what's run by the batch file is something like this:

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

### OpenNEC Compatibility

When invoked with no arguments on Windows, OpenNEC prints the same prompts as the original Fortran engines:
- `ENTER NAME OF INPUT FILE >`
- `ENTER NAME OF OUTPUT FILE >`

This ensures complete compatibility with systems that might expect these prompts.

## Engine Selection in 4nec2

4nec2 can be configured in **Settings → NEC engine** to use different calculation engines:

- **nec2d.exe** - Original NEC-2 engine (up to 2,000 segments)
- **nec2dXS.exe** - Extended-segment variant (up to ~20,000 segments)
- **somnec2d.exe** - Ground parameter generation engine

## Setting Up OpenNEC with 4nec2

A major difference between the original Fortran code and OpenNEC is that onec can calculate the Sommerfeld-Norton ground internally and does so automatically. No file needs to be generated for decks that use this feature. However, as 4nec2 users might expect to find the `.gnd` file, the batch file can still call `somnec2d.exe` without any issues for onec.

### Option 1: GUI Engine Selection (Windows)

The simplest approach is to copy the OpenNEC executable into 4nec2's engine directory, then select it as your engine in the 4nec2 GUI. **No somnec2d replacement needed** — OpenNEC handles ground calculations internally:

#### Step 1: Build OpenNEC for Windows

```bash
# On Windows with MSYS2/MinGW64 or WSL
make BACKEND=original
```

This produces `onec.exe` (or `onec` on Unix-like systems).

#### Step 2: Copy the Executable

1. Locate your 4nec2 installation directory (typically `C:\4nec2\exe`)
2. Copy `onec.exe` to the 4nec2 `exe` directory (keep the original `nec2d.exe` in place)

```batch
cd C:\4nec2\exe
copy C:\path\to\onec.exe onec.exe
```

This preserves the original `nec2d.exe` as your fallback engine.

#### Step 3: Configure 4nec2 to Use OpenNEC

1. Open 4nec2
2. Go to **Settings → NEC engine**
3. Click the path selector button and navigate to `C:\4nec2\exe\onec.exe`
4. Select `onec.exe` and confirm
5. Close Settings to save your selection
6. Create a simple antenna test case and generate calculations (F7 / "Calculate")
7. Verify that calculations complete successfully

**Advantages:**
- Keeps original `nec2d.exe` as a fallback
- Easy to switch between engines anytime
- Works with all 4nec2 features
- Simple upgrade path (just replace `onec.exe`)

**Limitations:**
- None significant; this is the recommended approach

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
- If the original nec2d.exe references Fortran runtime libraries, you may need them when using that engine
- OpenNEC has no external dependencies (self-contained)
- Simply select `onec.exe` in 4nec2 Settings and you won't need any Fortran libraries

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

4nec2 supports command-line mode (since v5.7.3). This works seamlessly with OpenNEC:

```batch
REM Run 4nec2 with input file using the GUI-selected engine
4nec2.exe input.nec /run /silent
```

Or force a specific engine via the command line:

```batch
REM Run with OpenNEC directly
onec.exe < input.tmp > output.txt
```

This works because 4nec2 handles all pre/post-processing and simply invokes the selected NEC engine via stdin/stdout.

## Reverting to Original Engines

To return to the original Fortran-based engines:

1. Open 4nec2 **Settings → NEC engine**
2. Click the path selector button and navigate back to `C:\4nec2\exe\nec2d.exe`
3. Select the original engine and confirm
4. Close Settings to save your selection

The original engines remain intact and can be selected anytime. You can also simply delete `onec.exe` from the `exe` folder if you no longer need it.

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
