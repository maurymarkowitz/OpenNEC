OpenNEC
=======

**Copyright © 2026 Maury Markowitz**

Introduction
------------

OpenNEC is an implementation of the NEC-2 code written in the C programming language. It is used to simulate the reception and transmission patterns of radio frequency antennas.

The antenna's physical design is stored on a series of "cards", and a collection of cards is known as a "deck". Cards are physically represented by lines of text in a text file representing a deck.

The deck files are normally saved with a `.nec`, `.deck` or `.onec` extension.

Running OpenNEC
---------------

Although OpenNEC is generally intended to be used embedded in other programs, it also includes a command-line interace that can be used to parse decks from the shell, and will write the calculated results (if run) to an output text file. To run it, use a command similar to this example, replacing the "FILE" with the names of the text files containing the antenna designs you wish to read:

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

Positional source_file...: one or more input deck files; each produces a .out.

If you wish to use onec to simply check syntax or reformat a file, use the `-n` and `-t` switches.

Short options with no parameters can be ganged, for instance, `-nt`.

OpenNEC concept
---------------
OpenNEC's code ultimately traces its history to the NEC-2 Fortran code released by Lawrence Livermore in the early 1980s. A port to C was made by Neoklis Kyriazis in the nec2c program which was first released on 20 August 2003. OpenNEC was forked from the 1.3 version of nec2c, released in 2014.

The primary difference between OpenNEC and nec2c is its form; nec2c is a command-line program, while most of the OpenNEC code is in the form of a static library. This allows OpenNEC to be embedded in any program written in a language that provides access to C libraries, like Rust, Java, or Swift. The optional main.c wraps the library to provide a command line tool similar to the original nec2c, but adds a number of new command line switches to provide finer control over execution.

Programs that wish to use OpenNEC as their calculation engine can directly access the various structures like the segment list and radiation patterns. They can also change the deck by modifying cards and re-running the calculations, or by directly changing the simulation parameters and re-running. Using the segment list is particularly useful during the antenna layout and design stage; programs can use the code to turn their geometry cards into segments which can then be sent to a 3D library.

OpenNEC additions
-----------------

OpenNEC includes a number of significant additions to the basic NEC-2 system:

- OpenNEC allows measurement units to be defined on a per-field basis. This is especially useful when defining wire radii; in an OpenNEC file a wire can be defined as "3awg" instead of having to replace that with the measurement in meters, "0.0058268". Different measurements can be used on different fields on the same card, or different cards. A `GS` card, if present, will *not* override those fields that have explicit measurements.

- OpenNEC adds a generic extension mechanism using key/value pairs that can be used by 3rd party software to add functionality without changing the underlying deck format. For instance, one could add "material:copper" to a card, and a GUI application using OpenNEC could then apply a copper color in a 3D model. These extensions are stored within an inline comment, so they have no effect on the NEC-2 code. A utility method allows these to be stripped out to produce a new deck that is compatible with generic NEC implementations.

  - Extension keys should always be compared in a case-insensitive manner. Keys should always be *written* in lower-case no matter how they were entered.

  - Three extensions are known to the basic OpenNEC system: `name`, `group`, and `ignore`. `name` and `group` are free-form strings intended to allow GUI-based applications to provide richer information and/or group related sections of the deck together. `ignore` tells the system to skip over that card when producing geometry or running calculations. This is useful during testing, as cards can be turned off without having to remove them from the deck or comment them out.

  - Additionally, 3rd party software using OpenNEC should be aware of these non-required GUI-related extensions: `invisible`, `material` and `shape`. `invisible` is used to suppress the element in GUI displays of the geometry, while still using in calculations. `material` is a free-form field but a number of common materials are defined. `shape` is used to control the cross section in the display, for instance `shape=square` might be used when defining a boom on a Yagi antenna.

- The key/value entries can be entered in a variety of formats, see the "OpenNEC file format" document for details.

Additions from other systems
----------------------------

A number of features commonly found in other popular NEC-based programs have been added:

* OpenNEC includes a whole-deck sanity check system that looks for common errors, like overlapping wires or wires touching ground. It also reports on more minor issues like missing CE or EN cards that can cause problems with some NEC-2 implementations. This allows OpenNEC to be used as a stand-alone syntax checker. The list of issues is in an exposed structure, Errors, which allows further tests to be implemented in external software and then added to existing errors lists to keep everything in one place.

* OpenNEC notices cards using the 4nec2 convention with tag numbers >= 9000 and sets `invisible=true` on those cards. 

* OpenNEC supports the `SY` card type from 4nec2. This is used to define variables, or SYmbols, which are parsed out in the deck. These are useful for defining the radius of wires and similar tasks, as well as making the deck more self-documented.

* OpenNEC supports in-line formulas, also found in 4nec2 decks. These allow you to define a symbol and then perform basic math operations on it, like "height+5". This has many uses, especially during optimizations. OpenNEC adds the ability to define these formulas in the extensions instead of directly in the card fields. OpenNEC can save files in 4nec2 format with these items "exposed", or in OpenNEC format with them hidden in comments so that the resulting deck is NEC-2 compatible.

* OpenNEC supports the `XT` card type from nec2c, which stops processing at that point. In contrast to nec2c, which simply exits the program when an `XT` is encountered, OpenNEC will still read the remaining cards but will not process them, effectively treating them as comments.

* OpenNEC supports the `#` comment marker from nec2c. This was used to insert whole-line comments at any point in the deck, a system that does not appear to be widely used. Note that the `#` comment marker can only appear at the start of a line and cannot be used to insert end-of-line comments. At any other location in the line, # is used as it is in 4nec2, to indicate a field is using AWG measurements. The leading-`#` is included only for compatibility, it is not considered to be part of the OpenNEC standard and should not be used except to produce nec2c compatible files. The modern replacement is `!`.

Defined constants
-----------------

OpenNEC defines a number of constants used in the extensions that 3rd party software should be aware of.

- `ignore` is an example of a boolean value which is indicated using `true` and `false`. During reading, the values `yes` and `no` or `1` and `0` may also be used, but these will be converted to `true` and `false` on write. As always, these are all case insensitive.

- the `material` may be any value, but the following values should be expected; `silver`, `copper`, `aluminum`, `6061-T6`, `6063-T832`, `brass`, `phosphor bronze` and `steel`. This list was based on the materials from Yagi Optimizer.

- `shape` may also be any value, but `circle` and `square` should be expected.

Acknowledgements
----------------
* OpenNEC is a fork of nec2c by Neoklis Kyriazis
* OpenNEC's function parser is based on tinyexpr, written by Lewis Van Winkle
* OpenNEC would not be possible without stackoverflow
