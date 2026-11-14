OpenNEC User Manual
===================

Introduction
------------

This manual describes how to use the OpenNEC command-line program, `onec`, to analyze antenna models and convert between file formats. It covers installation, command-line options, practical examples, and integration with other antenna modeling tools.

For instructions on creating and modeling antenna designs, see the [OpenNEC modeling manual](OpenNEC%20modeling%20manual.md). For information on using OpenNEC as a library in your own programs, see the [OpenNEC programmer manual](OpenNEC%20programmer%20manual.md).

What is OpenNEC?
----------------

OpenNEC is a high-performance implementation of the NEC-2 antenna simulation code. It reads antenna designs from text files called "decks", and calculates their electrical properties such as radiation patterns, impedance, and gain.

**Key features:**

- **Fast** — Uses multi-core processors and optimized math libraries (BLAS/LAPACK)
- **Cross-platform** — Identical behavior on Windows, macOS, and Linux
- **Compatible** — Drop-in replacement for nec2d, nec2c, and other NEC-2 engines
- **Flexible** — Reads and writes multiple file formats (EZNEC, MMANA-GAL, Yagi Optimizer, etc.)
- **Extensible** — Supports 4nec2 extensions like symbols and formulas
- **Unlimited** — No hard-coded limits on model size

OpenNEC comes in two forms, a library intended to be used directly inside other programs, and as a stand-alone program, `onec`, which lets you run calculations by hand.

Installation
------------

OpenNEC can be installed using pre-built binaries or compiled from source. Detailed installation instructions are available in the main [README](../README.MD).

### Quick Install

**macOS and Linux (using Homebrew):**
```bash
brew tap maurymarkowitz/tap https://github.com/maurymarkowitz/homebrew-tap
brew install maurymarkowitz/tap/onec
```

**Windows (using Scoop):**
```cmd
scoop bucket add maurymarkowitz https://github.com/maurymarkowitz/scoop-bucket
scoop install onec
```

**Manual Installation:**

Download the latest release from https://github.com/maurymarkowitz/OpenNEC/releases and extract to a convenient location.

On macOS, you'll need to remove the quarantine flag:
```bash
xattr -d com.apple.quarantine onec
```

Command line details
--------------------

OpenNEC was developed to match older command-line interface styles, as well as modernizing it to make it easier to use for new users. This leads to some complexity and gotchas.

When NEC was first created, as BRACT, computers generally did not have files in the modern sense. Instead, input data was punched onto cards and the output was a line printer. There was no need to specify the input and output, that was hard coded into the programs.

When NEC moved to more modern machines, files and the hard drives that stored them were becoming common, but there was no standard for passing those filenames into programs. Instead of trying to make it work across platforms, they added a couple of lines of code to print out a prompt and then read in user provided input and output filenames. So when you're using `nec2dx` on Windows for instance, it will look something like this, where the lowecase is what the user types:

    nec2d.exe
     INPUT FILE: myinput.nec
     OUTPUT FILE: myoutput.out

Most users today do not run the NEC code directly, they use a GUI shell program like 4nec2 that does it for them. To do this, the programs create a text file with the two responses in it, and then uses "redirection" to pass those in as if the user typed them, something like:

`nec2d.exe < nec2d.tmp`

Modern systems expect to be able to type in the file names on the command line. When nec2c was written, it added this feature using the `-i` and `-o` switches, so:

`nec2c -i myinput.nec -o myoutput.out`

Later authors working with NEC-4 and NEC-5 decided not to use the switches, more in keeping with DOS/Windows style, and used:

`nec5 myinput.nec myoutput.out`

So there's already multiple styles you need to handle.

This leads to another problem. on Unix systems it is generally expected that programs allow you to pass in the *contents* of file using redirection, instead of the names. The equivalent syntax for the command above using these Unix features would be:

`nec2c < myinput.nec > myoutput.out`

This sort of redirection is extremely useful in some workflows. For instance, you can redirect the report output to the many Unix tools to do things like extract certain values, look for problems, etc. This is especially useful during testing.

Unfortunately this is the same syntax as the original DOS interface using redirection. To avoid potential problems, OpenNEC currently treats a redirect on Windows systems as the nec2d-like "passing in the names", and on Unix it treats them as redirecting the contents.

Basic usage
-----------
The general syntax using positional parameters is similar to traditional command-line programs:

```
onec [options] <input_file_or_directory>...
```

### Simple Examples

The typical operation is to parse and calculate a single antenna model and send the results to stdout. In OpenNEC, the simplest way to do this is to use a positional parameter to provide a filename:

```bash
onec dipole.nec
```

You can add the `-o` command-line switch to send the output to a named file instead:

```bash
onec -o out_test.out dipole.nec
```

To allow OpenNEC to be used in the same fashion as nec2c, OpenNEC also allows you to use the `-i` command-line switch to define the input filename anywhere on the line:

```bash
onec -i dipole.nec
```

When using the -i switch, the -o switch can also make up a name based on the input name by replacing the original extension with `.out`. For instance, this will write the output to the file `dipole.out`:,

```bash
onec -o -i dipole.nec
```

On Unix and macOS, you can provide input and output files using redirection or piping. For instance, you can perform the same action as the previous example this way:

```bash
onec dipole.nec > dipole.out
```

Or if you prefer to be explicit:

```bash
onec < dipole.nec > dipole.out
```

Or you can use pipes:

```bash
cat example.nec | onec > result.out
```

Note that in these Unix-like examples there are no parameters being passed to OpenNEC, the shell is handling the redirection. In the case that you invoke OpenNEC without any identifiable input filename, and there is no file being piped or redirected into it, it will report an error and exit. This allows it to remain compatible with nec2c, which will do the same.

The Windows version differs here. If no input filename can be identified it will enter "interactive mode" and prompt for the input and output filenames. This matches the behaviour of the original Fortran code, which always interactively asks for these filenames. This style of interaction has been retained in more recent versions of that code like nec2dx, and various Windows applications rely on it.

At the time of writing, it is not clear how widespread the original Fortran style code is on the Unix side. If it turns out it *is* common, then the Unix version will be changed to work the same way and ask for files if none are provided.

In addition to single-file input and output, OpenNEC also allows you to specify multiple input files. For instance, to run several models you can:

```bash
onec dipole.nec loop.nec ldpa.nec
```

When used with multiple inputs, the output filename cannot be separately specified and the output will be sent directly to files instead of stdout. stdout is instead used to report progress of each file. Since this capability does not exist in other engines, there is no backward compability to be retained here, and the interface is the same on all platforms.

The real purpose of this feature is to allow the command shell to expand wildcards and globbing expressions into multiple input files. For instance, you can process all of the decks in the examples folder using:

```bash
onec examples/*.nec
```

You can also force OpenNEC to recursively decend all subfolders using the `-r` switch:

```bash
onec -r /path/to/antenna/models/
```

The `-r` switch was added for convinience, but you can do the same with traditional shell programming if you prefer:

```bash
find "$dir" -type f -iname "*.nec" -print0 |
while IFS= read -r -d '' file; do
    echo "Processing: $file"
    onec "$file"
done
```

### Summary

**Single file mode:**
- If no `-o` option is provided, output goes to `stdout` (the terminal)
- With `-o filename`, output is written to the specified file
- With `-o` but no explicit output filename, output goes to `<input>.out`

**Multiple file mode:**
- Each input file generates a corresponding `.out` file automatically
- Progress and summary information is written to `stdout`
- The `-o` option is not allowed with multiple files

**No input file:**
- On Unix/Linux/macOS: reads the deck from `stdin`, allowing piped input
- On Windows: prompts interactively for input and output filenames (for nec2d compatibility)

Command-Line options
--------------------

### Help and Information

**`-h`, `--help`**  
Display usage information and exit.

```bash
onec --help
```

On Windows, `/?` is also supported for help:
```cmd
onec /?
```

**`-v`, `--version`**  
Display version information and exit.

```bash
onec --version
```

### Input and Output Files

**`-i FILE`, `--input-file=FILE`**  
Specify the input file. This option is rarely needed since you can provide the filename as a positional argument.

```bash
onec -i antenna.nec
onec antenna.nec              # Equivalent
```

**`-o FILE`, `--output-file=FILE`**  
Specify the output file for simulation results. Only valid when processing a single input file.

```bash
# Write to a specific file
onec -o results.txt antenna.nec

# Use default name (antenna.out)
onec antenna.nec

# Write to stdout (default for single file)
onec antenna.nec
```

**`-e FILE`, `--error-file=FILE`**  
Redirect error messages and warnings to a file instead of `stderr`.

```bash
onec -e errors.log antenna.nec
```

This is particularly useful when processing many files to capture all warnings in one place:

```bash
onec -r -e validation.log /antenna/collection/
```

### Processing Control

**`-n`, `--no-run`**  
Parse the deck and perform validation, but skip the simulation. Useful for syntax checking, format conversion, or testing.

```bash
# Check syntax only
onec -n antenna.nec

# Combined with testing
onec -nt antenna.nec
```

**`-t`, `--test-deck`**  
Run comprehensive sanity checks on the deck: segment count validation, geometry checks, frequency range tests, and more. Often used with `-n` to validate without simulating.

```bash
# Validate deck without running simulation
onec -nt problematic.nec

# Run simulation with validation warnings
onec -t antenna.nec
```

**`-r`, `--recursive`**  
Recursively process all matching files in subdirectories.

```bash
# Process all .nec files in directory tree
onec -r /antenna/models/

# Process all .maa files recursively
onec -r '/antenna/models/*.maa'
```

When using `-r` with a bare directory, only `.nec` and `.deck` files are processed. To process other file types recursively, use a quoted glob pattern — the filter is inherited by all subdirectories.

**`--skip-large`**  
When processing multiple files, skip models with estimated complexity T ≥ 10¹¹. Useful for batch processing when you want to avoid long-running simulations.

```bash
onec -r --skip-large /large/antenna/collection/
```

The complexity estimate T is based on the number of segments and frequencies. Very large models may take minutes or hours to complete. See the [programmer manual](OpenNEC%20programmer%20manual.md) for further details.

**`-j N`, `--jobs N`**  
Process up to N files in parallel when multiple input files are specified. Default is 1 (sequential processing).

```bash
# Process up to 4 files at once
onec -j 4 examples/*.nec

# Use all available CPU cores
onec -j 8 -r /antenna/collection/
```

This option is ignored when processing a single file. The optimal value depends on your CPU core count and available memory. Generally you should use a value of N that matches the number of physical cores in your system. There is no standard way to look that up in code, you will have to find it online. A value of 4 will likely work well enough on any modern machine.

**`-a`, `--add-rp`**
One of the goals for OpenNEC is to run 4nec2 decks without modification. However, 4nec2 modifies decks at runtime in order to make them work in NEC-2, so the 4nec2 deck you open is not the one that actually gets processed.

Among the modifications 4nec2 makes is the addition of reporting cards like RP, NE, or NH, depending on what the user selects when running a calculation. Since these are not saved into the original 4nec2 deck, decks downloaded from the 'net will often produce no output.

To produce something useful, the `-a` flag can be turned on when using 4nec2 decks. If the reporting engine reaches the end of the deck and it has not seen an RP, NE, or NH, it will pretend there was an RP at the end. This produces some useful output without having to modify the original deck.

The RP is a single horizontal slice at 90 degrees (flat) with samples every 1 degree for the entire 360 degrees. There is no obvious "default" pattern, so this one was selected as it gives you reasonable estimates for gain in the most common use-cases, and you can get the beamwidth at a glance.

Note that this option looks for existing reporting cards first, so if they are already in the deck this option will be skipped. This means you can safely turn it on for almost any deck.

```bash
# Add default pattern if missing
onec --add-rp antenna.nec

# Or use short form
onec -a antenna.nec

# Combine with other options
onec -a -f nec2c antenna.nec
```

### File Format Conversion

**`-w FILE`, `--write-file=FILE`**  
Write the parsed deck to a file in the format indicated by the file extension. This can be used to convert between different antenna modeling formats.

**Supported output formats:**
- `.nec` or `.deck` — OpenNEC/4nec2 format (extended NEC-2)
- `.nec2` — Pure NEC-2 format (no extensions)
- `.nec4` — NEC-4 format
- `.maa` or `.mma` — MMANA-GAL format
- `.yo`, `.ant`, or `.yag` — Yagi Optimizer format
- `.nc` — cocoaNEC format

```bash
# Convert MMANA file to NEC format
onec -n -w antenna.nec antenna.maa

# Convert NEC to Yagi Optimizer
onec -n -w antenna.yo antenna.nec

# Batch convert entire directory
onec -n -w .nec -r /mmana/files/*.maa
```

When the filename is a bare extension (e.g., `-w .nec`), OpenNEC converts multiple files in place, generating output files with the same base name but different extension.

**Combined with simulation:**
```bash
# Convert and simulate in one step
onec -w antenna.nec antenna.maa
```

This converts the input file, runs the simulation, and produces both the converted `.nec` file and the `.out` results file.

### Output Format Control

**`-f FORMAT`, `--format=FORMAT`**  
Select the output format for `.out` files. Valid values are:
- `original` — Fortran NEC-2 format (default on all platforms)
- `nec2c` — Modern nec2c format with improved readability

```bash
# Use modern nec2c output format
onec -f nec2c antenna.nec

# Use original Fortran format (default)
onec -f original antenna.nec
```

The `original` format matches the output from the Fortran NEC-2 engines and is required for compatibility with some older tools. The `nec2c` format is easier to read and parse programmatically, not not widely used.

**`-l ENDING`, `--line-ending=ENDING`**  
Override the line ending style in output files. Valid values are:
- `lf` — Unix/Linux/macOS line endings (default on Unix)
- `crlf` — Windows line endings (default on Windows)

```bash
# Force Unix line endings
onec -l lf antenna.nec

# Force Windows line endings
onec -l crlf antenna.nec
```

By default, OpenNEC matches the line endings of the input file, except when writing to `stdout`. This option forces a specific style regardless of input or platform.

### Advanced Output Options

**`-g [FILE]`, `--greens[=FILE]`**  
Generate a Green's function output file. If no filename is provided, uses the input filename with `.ngf` extension.

```bash
# Generate Green's function file
onec -g antenna.nec              # Creates antenna.ngf
onec -g custom.ngf antenna.nec   # Creates custom.ngf
```

Green's function files are used for advanced electromagnetic analysis and coupling calculations between antennas.

Examples
--------

### Example 1: Basic Simulation

Simulate a simple dipole antenna and view results in the terminal:

```bash
onec examples/dipole.nec
```

The output includes:
- Antenna input impedance at each frequency
- Gain and directivity
- Radiation patterns (if RP card is present)
- Efficiency and other performance metrics

### Example 2: Save Output to File

Run simulation and save detailed results:

```bash
onec -o dipole_results.out examples/dipole.nec
```

This creates `dipole_results.out` containing all simulation data that would normally be displayed in the terminal.

### Example 3: Syntax Validation

Check an antenna model for errors without running the simulation:

```bash
onec -nt questionable.nec
```

This runs validation tests (`-t`) but skips simulation (`-n`), making it fast even for complex models. Output includes warnings about:
- Segment count issues (too few or too many)
- Geometry problems (overlapping wires, zero-length segments)
- Invalid card parameters
- Missing required cards

### Example 4: Format Conversion

Convert an MMANA-GAL file to standard NEC format:

```bash
onec -n -w yagi.nec yagi.maa
```

The `-n` flag prevents simulation, making the conversion faster. The `-w` flag specifies the output format based on the `.nec` extension.

### Example 5: Batch Processing

Process all antenna models in a directory:

```bash
onec examples/*.nec
```

Each `.nec` file generates a corresponding `.out` file. Progress information is displayed as each file completes.

### Example 6: Recursive Directory Processing

Process all antenna models in a directory tree:

```bash
onec -r ~/antenna_designs/
```

This finds all `.nec` and `.deck` files in `~/antenna_designs/` and all subdirectories, runs simulations, and generates `.out` files for each.

### Example 7: Parallel Processing

Speed up batch processing using multiple CPU cores:

```bash
onec -j 4 -r ~/antenna_designs/
```

Processes up to 4 files simultaneously, significantly reducing total processing time for large collections.

### Example 8: Import and Simulate MMANA Files

Process all MMANA-GAL files in a directory:

```bash
onec '~/mmana_antennas/*.maa'
```

Note the quotes around the glob pattern — this prevents the shell from expanding the pattern and allows OpenNEC to control the file matching.

### Example 9: Batch Format Conversion

Convert an entire directory of MMANA files to NEC format:

```bash
onec -n -w .nec -r ~/mmana_collection/
```

This creates a `.nec` file for every `.maa` or `.mma` file found, preserving the directory structure.

### Example 10: Pipeline Processing

Use OpenNEC in a Unix pipeline:

```bash
cat antenna.nec | onec > results.txt
```

Or combine with preprocessing:

```bash
grep -v '^CM' antenna.nec | onec > results.txt
```

This example removes all comment lines before processing.

### Example 11: Multiple Output Formats

Generate both simulation results and a converted file:

```bash
onec -w yagi.nec -o yagi_results.out yagi.maa
```

This converts the MMANA file to NEC format, runs the simulation, and saves:
- `yagi.nec` — converted deck
- `yagi_results.out` — simulation results

### Example 12: Error Logging

Collect all validation warnings when processing many files:

```bash
onec -t -e warnings.log -r ~/antenna_collection/
```

All warnings and errors from every file are written to `warnings.log` for later review.

### Example 13: Selective Processing

Process only Yagi Optimizer files in a mixed directory:

```bash
onec -r '/path/to/antennas/*.yo'
```

The glob pattern must be quoted to prevent shell expansion. Only `.yo` files are processed, and the pattern is applied recursively.

### Example 14: Testing Before Full Run

Test a large batch before committing to full simulation:

```bash
# First, validate all files
onec -nt -r ~/large_collection/

# If validation passes, run simulations
onec -r ~/large_collection/
```

### Example 15: Using with Other Tools

Combine OpenNEC with other command-line tools:

```bash
# Find all files that fail validation
for f in *.nec; do
  onec -nt "$f" 2>&1 | grep -q ERROR && echo "$f has errors"
done

# Count frequencies in all models
grep -h '^FR' *.nec | wc -l

# Extract impedance data
onec antenna.nec | grep 'IMPEDANCE'
```

Integration with Other Programs
--------------------------------

OpenNEC is designed as a drop-in replacement for traditional NEC-2 engines in graphical antenna modeling programs.

### Using OpenNEC with 4nec2

4nec2 is a popular Windows-based antenna modeling GUI. To use OpenNEC:

1. Copy `onec.exe` to the 4nec2 installation directory
2. In 4nec2, go to **Settings → NEC engine → Manual select**
3. Select `onec.exe` from the list

See [Using OpenNEC with 4nec2](Using%20OpenNEC%20with%204nec2.md) for detailed instructions.

### Using OpenNEC with cocoaNEC

cocoaNEC is a sophisticated antenna modeling application for macOS. To configure:

1. Install OpenNEC using Homebrew or copy the binary to a known location
2. In cocoaNEC, configure the external NEC-4 engine path to point to `onec`
3. Set engine options in cocoaNEC preferences

See [Using OpenNEC with cocoaNEC](Using%20OpenNEC%20with%20cocoaNEC.md) for complete setup instructions.

### Using OpenNEC in Scripts

OpenNEC's command-line interface makes it ideal for automation and batch processing:

```bash
#!/bin/bash
# Process all antennas and generate a summary

for file in *.nec; do
  echo "Processing $file..."
  onec -o "${file%.nec}.out" "$file"
  
  # Extract gain from output
  gain=$(grep "GAIN" "${file%.nec}.out" | head -1)
  echo "$file: $gain" >> summary.txt
done
```

### Return Codes

OpenNEC uses standard Unix return codes:
- `0` — Success (all files processed without fatal errors)
- `1` — Fatal error (parse failure, missing file, etc.)
- `2` — Invalid command-line arguments

In batch mode, OpenNEC continues processing remaining files even if one fails, and returns the appropriate code based on the worst error encountered.

File format support
-------------------

OpenNEC can read and write multiple antenna modeling file formats:

| Extension(s) | Format | Read | Write | Notes |
|-------------|--------|------|-------|-------|
| `.nec`, `.deck`, `.4nec` | OpenNEC/4nec2 | ✓ | ✓ | Native format, full feature support |
| `.nec2` | NEC-2 | ✓ | ✓ | Pure NEC-2, no extensions |
| `.nec4` | NEC-4 | ✓ | ✓ | Some NEC-4 cards not supported |
| `.ez`, `.ezn` | EZNEC | — | — | Recognized but not yet supported |
| `.maa`, `.mma` | MMANA-GAL | ✓ | ✓ | Full support |
| `.nc` | cocoaNEC | ✓ | ✓ | Most features supported |
| `.yo`, `.ant`, `.yag` | Yagi Optimizer | ✓ | ✓ | Full support |
| `.nwp`, `.nwz` | NEC-Win Plus/Zip | — | — | Recognized but not supported |

For detailed information on each format's capabilities and limitations, see the format-specific documents in the [docs](.) directory.

Troubleshooting
---------------

### Common Issues

**"Cannot open file"**  
Ensure the file path is correct and the file is readable. On Unix systems, check file permissions.

```bash
ls -l antenna.nec
chmod 644 antenna.nec
```

**Windows: "Can't find DLL", or simply exits with no output**  
If you are using the BLAS version, ensure OpenBLAS or MKL DLLs are in your PATH or the same directory as `onec.exe`. See [BUILD-WINDOWS](BUILD-WINDOWS.md).

**"Unexpected token" or parsing errors**  
The input file may have formatting issues. Try:
- Ensuring the file uses proper line endings for your platform
- Checking for non-ASCII characters
- Validating with `-nt` to see detailed error messages

**Simulation hangs or takes very long**  
Large models with many segments and frequencies can take hours. Use `--skip-large` when batch processing, or reduce the FR card frequency count for testing.

**Incorrect results**  
Verify your model with validation:
```bash
onec -t antenna.nec
```

Common modeling errors include:
- Too few segments (use at least 10-20 per wavelength)
- Incorrect wire radius
- Overlapping or nearly touching wires
- Missing ground plane or incorrect ground parameters

### Verbose Output

For debugging, combine validation with error logging:

```bash
onec -t -e debug.log antenna.nec
```

This captures all warnings, errors, and diagnostic information.

### Getting Help

If you encounter bugs or have questions, open an issue at https://github.com/maurymarkowitz/OpenNEC/issues

Performance tips
----------------

### Multi-Threading

Builds with OpenMP use multiple threads for matrix operations. The OpenMP runtime chooses the default thread count, which can vary by system and may not be the fastest choice for every model.

Performance can vary depending on the number of threads, so it's always a good idea to experiment a bit to see what selections work best on your machine. You can do this by setting the value of the shell variable `OMP_NUM_THREADS` before launching onec:

```bash
OMP_NUM_THREADS=4 onec antenna.nec
```

In the classic Windows shell, use `set OMP_NUM_THREADS=4` before running `onec`. In PowerShell, use `$env:OMP_NUM_THREADS = "4"`.

A good way to find the best value on your platform is to use the benchmarking script in `/tests/benchmark_tests/`. This runs the two large decks multiple times. You can modify it to change the `OMP_NUM_THREADS` and see which setting works best for you. These are large files though, it can take several minutes to run so be ready for that.

### BLAS Libraries

When compiled with BLAS support (OpenBLAS, MKL, or Apple Accelerate), OpenNEC can be significantly faster:
- Small models (< 100 segments): minimal difference
- Medium models (100-1000 segments): 2-3× faster
- Large models (> 1000 segments): 5-10× faster

Binary distributions include both standard and BLAS-enabled versions where applicable.

### Batch Processing

When processing many files:
- Use `-j N` to process multiple files in parallel
- Use `--skip-large` to avoid extremely time-consuming models
- Use `-n -t` first to validate all files before running simulations

When using the `-j` option, choose a value for N that is close to the number of physical cores in your machine, *not the number of virtual cores*. For instance, it is quite common for modern processors to have, say, four physical cores on the chip, each of which is able to run two threads. Due to the way that NEC is data-bound, the key to improving performance is to avoid communications between the cores, and threading will have little effect. So in this example, `-j 4` would likely provide the best performance.

Unfortunately there does not appear to be a standard way to find this number programmatically, you have to look it up online.

### Model Optimization

For faster simulations:
- Reduce the number of frequency points in FR cards during testing
- Use the minimum necessary segment count (but not less than ~10 per wavelength)
- Avoid unnecessarily dense wire grids

Appendix: Quick Reference
--------------------------

### Common Command Patterns

```bash
# Single file simulation
onec antenna.nec

# Save to file
onec -o results.out antenna.nec

# Validate only
onec -nt antenna.nec

# Convert format
onec -n -w output.nec input.maa

# Batch process directory
onec -r /path/to/models/

# Parallel batch processing
onec -j 4 -r /path/to/models/

# Process with error logging
onec -t -e errors.log -r /models/
```

### Options Summary

| Short | Long | Argument | Description |
|-------|------|----------|-------------|
| `-h` | `--help` | — | Display help and exit |
| `-v` | `--version` | — | Display version and exit |
| `-n` | `--no-run` | — | Skip simulation (parse only) |
| `-t` | `--test-deck` | — | Run validation tests |
| `-r` | `--recursive` | — | Process subdirectories |
| | `--skip-large` | — | Skip models with T ≥ 10¹¹ |
| `-i` | `--input-file` | FILE | Specify input file |
| `-o` | `--output-file` | FILE | Specify output file (single file mode) |
| `-e` | `--error-file` | FILE | Write errors to file |
| `-g` | `--greens` | [FILE] | Generate Green's function file |
| `-j` | `--jobs` | N | Process N files in parallel |
| `-w` | `--write-file` | FILE | Convert and write to file |
| `-f` | `--format` | FORMAT | Output format: `original` or `nec2c` |
| `-l` | `--line-ending` | ENDING | Line endings: `lf` or `crlf` |

### Exit Codes

| Code | Meaning |
|------|---------|
| 0 | Success |
| 1 | Fatal error |
| 2 | Invalid arguments |

Further Reading
---------------

- [OpenNEC modeling manual](OpenNEC%20modeling%20manual.md) — How to create antenna models
- [OpenNEC programmer manual](OpenNEC%20programmer%20manual.md) — Using OpenNEC as a library
- [OpenNEC, NEC-2 and nec2c](OpenNEC,%20NEC-2%20and%20nec2c.md) — Technical comparison
- [BUILD](BUILD.md) — Compilation instructions
- [File format documents](.) — Details on MMANA, Yagi Optimizer, cocoaNEC formats
