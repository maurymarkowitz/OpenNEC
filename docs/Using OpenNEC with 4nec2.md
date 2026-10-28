Using OpenNEC (onec) with 4nec2
===============================

Introduction
------------

The OpenNEC command-line program, `onec`, can be used as a drop-in replacement for the traditional NEC-2 calculation engines in 4nec2. OpenNEC provides much better performance, cross-platform compatibility, and supports models with large numbers of segments. This guide explains how to [configure 4nec2 to use onec](#configuring-4nec2-to-use-opennec) instead of the older Fortran-based executables that are supplied with 4nec2.

4nec2's internal file format is a modified version of the format used by the Fortran code. It works by writing out a temporary deck with those modifications replaced with values the Fortran code can work with. OpenNEC can read these Fortran-style temporary files and produce output like what 4nec2 expects. See the section one [how 4nec2 invokes NEC engines](#how-4nec2-invokes-nec-engines) for details on this process.

OpenNEC is also designed to be able to process 4nec2 files directly, without this conversion. This allows you to run 4nec2 decks on machines where 4nec2 is not available, like macOS and Linux. However, "raw" 4nec2 decks often lack output cards, so while the deck will be processed and calculated, it often produces no useful output. To deal with this case, `onec` has the `-a` command line switch to generate default output. See the section on [default RP addition](#default-rp-addition) for details.

Configuring 4nec2 to use OpenNEC
--------------------------------

4nec2 ships with several calculation engines based on variations of the original Fortran code:

- **nec2d.exe** - Original NEC-2 engine (up to 2,000 segments)
- **nec2dXS.exe** - Extended-segment variant (up to ~20,000 segments)
- **somnec2d.exe** - Ground parameter generation engine

4nec2's GUI includes a screen where you can specify which of these programs you wish to use. OpenNEC is designed to emulate these programs so it can be used in their place. The simplest approach is to copy the OpenNEC executable, `onec.exe`, into 4nec2's engine directory, then select it as your engine in the 4nec2 GUI.

#### Step 1: Copy the Executable

1. Locate your 4nec2 installation directory (typically `C:\4nec2\exe`)
2. Locate the installation directory for OpenNEC, and the `exe` directory inside it
3. Copy `onec.exe` from the OpenNEC `exe` directory to the 4nec2 `exe` directory

```batch
cd C:\4nec2\exe
copy C:\path\to\onec.exe onec.exe
```

This preserves the original `nec2d.exe` as your fallback engine.

#### Step 2: Configure 4nec2 to Use OpenNEC

1. Open 4nec2
2. Go to **Settings → NEC engine**
3. In the menu, select the **Manual select** option
    NOTE: if the checkbox beside it is turned on, select it to turn if off and then do this step again
4. A dialog will appear allowing you to select the executable, type in `onec.exe` (you do not need the complete path, just the name). Click OK to accept it.
5. A new window appears asking for the number of segments. OpenNEC can handle very large numbers, but 11000 appears to work well. Click OK to continue.
6. Create a simple antenna test case and generate calculations (F7 / "Calculate")
7. Verify that calculations complete successfully

### Reverting to Original Engines

To return to the original Fortran-based engines:

1. Open 4nec2 **Settings → NEC engine → Manual select**
2. Type in `nec2d.exe`. Click OK.
3. Type in an appropriate number of segments. Click OK.
4. Close Settings to save your selection

The original engines remain intact and can be selected anytime. You can also simply delete `onec.exe` from the `exe` folder if you no longer need it.

How 4nec2 invokes NEC engines
-----------------------------

The original Fortran code, which ships with the 4nec2 install, is an interactive program which prompts the user for filenames for the input and output files. To drive these programs in an automated fashion, 4nec2 writes the input file to disk with the `.inp` extension and has the Fortran code write the results to a file with the `.out` extension. It then reads the `.out` file and parses it to get the results.

To do this, 4nec2 uses "redirection" through a batch file. First it writes another file, `nec2d.tmp`, which contains the name of the input and output files. It then runs `4nec2.bat`, which feeds the `nec2d.tmp` file into the Fortran program. Ultimately, what's run by the batch file is something like this:

`nec2d.exe < nec2d.tmp`

This invokes the `nec2d.exe` program with the filenames in `nec2d.tmp` passed into it using redirection, `<`. The deck data is read from the named input file, and the result is sent to the named output file.

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

3. **Output Handling**: The engine writes results to the `.out` file, which 4nec2 captures for display

Default RP addition
-------------------

Many antenna models in 4nec2 don't include explicit radiation pattern (RP) requests in their NEC decks. This is because the 4nec2 GUI (normally) adds one to the `.inp` file when it's being written out. These RP's (or NH or NE) cards are not added to the `.nec` deck that 4nec2 saves out when the user saves their file, they are only added on-the-fly during processing of the `.inp`.

So if you download a deck that was created in 4nec2, it probably doesn't contain any reporting cards. This doesn't break anything in OpenNEC, the deck will run fine, but when you look at the `.out` report it might seem "odd" to someone expecting a complete report like the `.out` files that 4nec2 would generate in this case. 

To help out in this situation, and especially when batch-running multiple files that may include 4nec2 decks, OpenNEC provides the `--add-rp` (short form `-a`) option to automatically inject a default RP if there are no reporting cards in the deck. This generates a horizontal pattern (slice at 90° elevation) with 1° azimuth resolution, covering the full 360° slice.

If the deck already has reporting cards in it, this option does nothing. This means it's safe to use with "real" NEC-2 decks which will almost always include one of these cards, 4nec2 with explicit reporting (which is not *that* uncommon), as well as 4nec2 decks lacking the reports. A mixed collection of such files will produce useful output for all of these cases.

NOTE: this issue only comes up if you're running the file in OpenNEC "by hand" in the terminal or some other program, if you're using it *within* 4nec2, as this document describes, then it will be handed the .inp file that 4nec2 built, and that file *will* have the RP in it.

Technical Considerations
------------------------

### OpenNEC compatibility with nec2d

When invoked with no arguments on Windows, OpenNEC prints the same prompts as the original Fortran engines:
- `ENTER NAME OF INPUT FILE >`
- `ENTER NAME OF OUTPUT FILE >`

This ensures complete compatibility with systems that might expect these prompts.

### Input Format

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

**Important difference**: OpenNEC performs Sommerfeld-Norton ground calculations **internally** as part of the main calculation engine, unlike the traditional NEC-2 approach which requires a separate `somnec2d.exe` invocation.

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

Troubleshooting
---------------

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

Performance Comparison
----------------------

Typical improvements when using OpenNEC:

| Test Case | Original nec2d.exe | OpenNEC (accelerate/OpenBLAS) | Improvement |
|-----------|-------------------|-------------------------------|------------|
| Simple dipole | ~0.5s | ~0.3s | 40% faster |
| Moderate array (100 segments) | ~2s | ~1s | 50% faster |
| Large array (1000 segments) | ~15s | ~6s | 60% faster |
| Very large (5000 segments) | Not supported | ~30s | N/A |

*Performance depends on CPU and model complexity*

Advanced: Command-Line Mode
---------------------------

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

References
----------

- [OpenNEC GitHub Repository](https://github.com/maurymarkowitz/OpenNEC)
- [4nec2 Home Page](http://www.qsl.net/4nec2/)
- [NEC-2 Documentation](https://en.wikipedia.org/wiki/Numerical_Electromagnetics_Code)
- [OpenBLAS for High Performance](https://www.openblas.net/)
