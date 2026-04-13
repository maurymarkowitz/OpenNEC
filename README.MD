OpenNEC
=======

**Copyright © 2026 Maury Markowitz**

[![MIT license](https://img.shields.io/badge/license-MIT-blue.svg)](https://opensource.org/licenses/MIT)
![NEC-2](https://img.shields.io/badge/NEC2-engine-orange?style=flat-square)
![PRs welcome](https://img.shields.io/badge/PRs-welcome-brightgreen?style=flat-square)

This software is released under the MIT license. See [LICENSE](LICENSE) for details.

Introduction
------------

OpenNEC is an implementation of the NEC-2 code written in the C programming language. It is used to simulate the reception and transmission patterns of radio frequency antennas. The antenna's physical design is stored on a series of "cards", and a collection of cards is known as a "deck". Decks are stored in text files normally with a `.nec` extension, or more rarely, `.deck`.

OpenNEC will read the design in the deck and can run a number of calculations to determine the antenna's performance. The output can be read directly, or exported to a text file, normally with an `.out` extension. It can also provide warnings about common design problems, and import and export decks to and from other common antenna file formats. OpenNEC uses the latest features of modern computers and runs much faster than older implementations.

Installing OpenNEC
------------------

The latest source code and binaries can be found at:

https://github.com/maurymarkowitz/OpenNEC/releases

Binaries are built for macOS, Linux and Windows. They can be installed by downloading the appropriate .zip file and unzipping into any convenient location.

On macOS and Linux, you can use homebrew:

```
brew tap maurymarkowitz/tap https://github.com/maurymarkowitz/homebrew-tap
brew install maurymarkowitz/tap/onec
```

On Apple Silicon, the executable will be installed to `/opt/homebrew` instead of `/usr/local` in keeping with Homebrew standards.

On Windows, you can use scoop:

```
scoop bucket add maurymarkowitz https://github.com/maurymarkowitz/scoop-bucket
scoop install onec
```

Building OpenNEC
----------------

You can also easily build the project from source. It is a makefile project that does not require `autoconfig` or other steps. A simple `make` in the root directory should produce a working executable on practically any unix-like system. For platform-specific build instructions and backend selection (Accelerate/OpenBLAS/MKL/BLAS), see [BUILD](docs/BUILD.md).

Windows compatibility is provided using MinGW, which needs to be installed manually. The same makefile is used for both Unix and Windows, and the `make` utility supplied with MinGW works perfectly. See [BUILD-WINDOWS](docs/BUILD-WINDOWS.md) for details.

Both platforms support `make install` which adds the manuals to the proper locations, and `make uninstall` to cleanly remove all the parts.

Running OpenNEC
---------------

OpenNEC includes a command-line interace, `onec`, that can be used to parse decks from the shell, and will write the calculated results to `stdout` or to a specific file using the `-o` flag.

It is normally invoked from your unix shell, `Terminal` on macOS, or `CMD` on Windows. To run it, use a command similar to this example, replacing the "<FILE/PATH>" with the names of the one or more text files containing the antenna designs you wish to process, or with one or more directories containing such files:

```./onec [options] <FILE/PATH>...```

Command-line options include:

`-h`, `--help`: print usage notes (this list)  
`-v`, `--version`: print version info  
`-r`, `--recursive`: process directories recursively, parse every NEC file found  
`-n`, `--no-run`: parse the deck but skip calculation/simulation  
`-t`, `--test-deck`: run deck sanity checks (often used with -n)  
`-j` <N>, `--jobs` <N>: if multiple files are passed in, run them in parallel up to <N> at a time  
`-o`, `--output-file` <file>: write output to a specific file (single input only)  
`-e`, `--error-file` <path>: write errors/logs to a file instead of `stderr`  
`-g`, `--greens` <file>: write Greens function output to the given file (or default .ngf)  
`-w`, `--write-file` <file>: writes the deck out to the named file type (see below)  
`-f`, `--format`: select the format for the generated `.out` file. Valid values are `nec2c` (modern default on macOS/Linux) or `original` (classic Fortran-style output, default on Windows).  
`--skip-large`: when running multiple decks, skip those with estimated complexity T >= 1e11  

Short options with no parameters can be ganged, for instance, `-nt`.

### On the PC

There are two changes in the command-line interface on windows, to better support existing NEC-2 workflows.

The first is to add the `/?` command line switch, the same as `-h`.

The second is that running the `onec.exe` with no command line switches or positional arguments will prompt the user for input and output filenames, rather than printing the usage notes. This matches the behavior of the original Fortran code, which is normally fed input from `CONIN$` (the windows `stdin`). The input is a file that contains two lines of text, the first having the input filename, and the second the output filename. If the filename is empty, the program will exit with an error.

Examples
--------

To run against a single file and send the results to stdout:

```./onec example.nec```

To run against the same file and send the results to a file with the default name `example.out`:

```./onec -o example.nec```

Or you can change the output name:

```./onec -o example_output.out example.nec```

To select the output layout format for the generated `.out` file:

```./onec -f original example.nec```

To run against a folder full of files, even in subdirectories:

```./onec -r /example_files/```

When using the `-r` switch, the output will always be sent to `<file>.out`, and stdout is used for reporting the filenames as they are processed, along with end-of-batch information.

If you wish to use onec to simply check syntax or reformat a file, use the `-n` and `-t` switches:

```./onec -nt example.nec```

This will produce a series of warnings or errors detailing issues (if any) in the deck and then exit, without attempting to run the simulations.

File conversion
---------------

OpenNEC has built-in readers and writers for various 3rd party file formats. For instance:

```./onec example.maa```

This will read the file `example.maa`, convert it to NEC format, and then run the simulations. When started with a path, or with the `-r` option, normally only NEC files will be processed. You can force it to read other files using a globbing expression:

```./onec -r /example_files/*.maa```

In this example it will ignore anything that is *not* an `.mma` file.

onec can also write files in different formats using the `-w` option. This is normally used with a "bare" extension like `.nec`, but you can also provide a complete filename if you wish to rename the converted deck. In most cases, you use something like:

```./onec -w .nec example.maa```

This will create a new NEC file, `example.nec`. This also works along with `-r` and globbing, so one can convert a folder of files to NEC format with something like:

```./onec -r -n -w .nec /example_files/*.maa```

In this case the `-n` was added so it does not try to calculate any results, it simply reads the file and writes the deck.

There are a number of limitations in these importers as the various formats support different features and options. See the various "file format" documents in the [docs folder](docs/) for more details. The currently recognized formats and extensions are:

| Extension(s) | Format | Read | Write |
|---|---|---|---|
| `.nec` `.deck` `.4nec` | OpenNEC / 4nec2 deck | ✓ | ✓ |
| `.nec2` | NEC-2 | ✓ | ✓ |
| `.nec4` | NEC-4 | ✓ | ✓ |
| `.maa` `.mma` | MMANA-GAL | ✓ | ✓ |
| `.nc` | cocoaNEC | ✓ | ✓ |
| `.yo` `.ant` `.yag` | Yagi Optimizer | ✓ | ✓ |
| `.ez` `.ezn` | EZNEC | recognised | — |
| `.nwp` `.nwz` | NEC-Win Plus / Zip | recognised | — |
| `.aci` | AntSolver | recognised | — |
| `.mmae` | MMANA-EZ | recognised | — |

"Recognised" means the extension is detected and a helpful error is shown, but no import/exporter exists yet.

OpenNEC concept
---------------
OpenNEC's code ultimately traces its history to the NEC-2 Fortran code released by Lawrence Livermore in the early 1980s. A port from Fortran to C using an automated conversion tool was made by Neoklis Kyriazis in the nec2c program which was first released on 20 August 2003. OpenNEC was forked from the 1.3 version of nec2c, released in 2014. Although initially conceived a simple update to nec2c, OpenNEC was eventually completely re-written, but an effort was made to keep the overall layout of the nec2c project.

The primary difference between OpenNEC and nec2c is its form; nec2c is a command-line program, while most of the OpenNEC code is in the form of a static library. This allows OpenNEC to be embedded in any program written in a language that provides access to C libraries, like Rust, Java, or Swift. The optional `main.c` included in the project wraps the library to provide a command line tool similar to the original nec2c, but adds a number of new command line switches to provide finer control over execution and many new options.

Programs that wish to use OpenNEC as their calculation engine can directly access the various structures like the segment list and radiation patterns. They can also change the deck by adding or modifying cards, or by changing the simulation parameters and re-running the calculations. Using the segment list is particularly useful during the antenna layout and design stage; programs can use the code to turn their geometry cards into a segments structure which can then be sent to a 3D library.

OpenNEC also improves many parts of the internal functionality, including multi-threading, parsing multiple files at once, adding support for 3rd party extensions to NEC, and supporting a number of common math libraries that dramatically increase performance. On large files, OpenNEC can be 7 to 10 times as fast as the original code. See [OpenNEC, NEC-2 and nec2c](docs/OpenNEC,%20NEC-2%20and%20nec2c.md) for details.

OpenNEC also adds many of the features found in the popular 4nec2 program on Windows. In particular, it supports the symbols (variables), formulas and calculations that make it easier to design complex antennas. These features are widely used and found in many of the real-world `.nec` decks found on the 'net. It also supports new variants of the `LD` and `EX` cards and other 4nec2 additions.

**NOTE:** One curiousity in the nec2c code is that the value for the speed of light, the constant `CVEL` is set to 299.8. This is not very accurate, and changing this to a more accurate value, 299.792458, results in 1 to 2% changes in most files. The original value has been retained in onec for compatibility reasons.

Known limitations
-----------------

* OpenNEC does not currently implement the IT and OP cards from 4nec2.
* NEC-4 cards like CW and IS are not currently supported.

Acknowledgements
----------------
* OpenNEC is a fork of nec2c by Neoklis Kyriazis
* OpenNEC parses formulas using tinyexpr by Lewis Van Winkle, which is released under the zlib licence
