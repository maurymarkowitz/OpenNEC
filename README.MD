OpenNEC
=======


**Copyright © 2026 Maury Markowitz**

[![MIT license](https://img.shields.io/badge/license-MIT-blue.svg)](https://opensource.org/licenses/MIT)

This software is released under the MIT license. See [LICENSE](LICENSE) for details.

Introduction
------------

OpenNEC is an implementation of the NEC-2 code written in the C programming language. It is used to simulate the reception and transmission patterns of radio frequency antennas.

The antenna's physical design is stored on a series of "cards", and a collection of cards is known as a "deck". Cards are physically represented by lines of text in a text file representing a deck. The deck files are normally saved with a `.nec`, `.deck` or `.onec` extension.

OpenNEC will process the design in the deck and run a number of calculations to determine the antenna's performance. The results are written back out to a file with the same name but with the extension `.out`.

Running OpenNEC
---------------

Although OpenNEC is primarily intended to be used embedded in other programs, it also includes a command-line interace that can be used to parse decks from the shell, and will write the calculated results (if desired) to an output text file. To run it, use a command similar to this example, replacing the "FILE" with the names of the one or more text files containing the antenna designs you wish to process:

```./onec FILE...```

Command-line options include:

`-h`, `--help`: print usage notes  
`-v`, `--version`: print version info  
`-n`, `--no-run`: parse the deck but skip calculation/simulation  
`-t`, `--test-deck`: run deck sanity checks (often used with -n)  
`-j` <N>, `--jobs` <N>: if multiple files are passed in, run them in parallel up to <N> at a time  
`-o`, `--output-file` <path>: write output to a specific file (single input only)  
`-e`, `--error-file` <path>: write errors/logs to a file instead of stderr  
`-g`, `--greens` <file>: write Greens function output to the given file (or default .ngf)  

If you wish to use onec to simply check syntax or reformat a file, use the `-n` and `-t` switches.

Short options with no parameters can be ganged, for instance, `-nt`.

Build & Setup
-------------
OpenNEC is a makefile project that does not require `make config` or other steps. A simple `make` should produce a working executable on practically any unix-like system.

For platform-specific build instructions and backend selection (Accelerate/OpenBLAS/MKL/BLAS), see [BUILD](BUILD).

OpenNEC concept
---------------
OpenNEC's code ultimately traces its history to the NEC-2 Fortran code released by Lawrence Livermore in the early 1980s. A port to C was made by Neoklis Kyriazis in the nec2c program which was first released on 20 August 2003. OpenNEC was forked from the 1.3 version of nec2c, released in 2014.

The primary difference between OpenNEC and nec2c is its form; nec2c is a command-line program, while most of the OpenNEC code is in the form of a static library. This allows OpenNEC to be embedded in any program written in a language that provides access to C libraries, like Rust, Java, or Swift. The optional main.c wraps the library to provide a command line tool similar to the original nec2c, but adds a number of new command line switches to provide finer control over execution.

Programs that wish to use OpenNEC as their calculation engine can directly access the various structures like the segment list and radiation patterns. They can also change the deck by modifying cards and re-running the calculations, or by directly changing the simulation parameters and re-running. Using the segment list is particularly useful during the antenna layout and design stage; programs can use the code to turn their geometry cards into segments which can then be sent to a 3D library.

OpenNEC also improves many parts of the internal functionality, including multi-threading, parsing multiple files at once, adding support for 3rd party extensions to NEC, and supporting a number of common math libraries that dramatically increase performance. See [Differences between OpenNEC, NEC-2 and nec2c](/doc/Differences%20between%20OpenNEC,%20NEC-2%20and%20nec2c.md) for details.

Known limitations
-----------------

* OpenNEC does not currently implement the IT and OP cards from 4nec2, although this is high on the list for implementation.
* OpenNEC implements measurements differently than 4nec2, but this may be change to use their solution in the future.
* NEC-4 cards like CW are not currently supported.

Acknowledgements
----------------
* OpenNEC is a fork of nec2c by Neoklis Kyriazis
* OpenNEC parses formulas using tinyexpr by Lewis Van Winkle, which is released under the zlib licence
