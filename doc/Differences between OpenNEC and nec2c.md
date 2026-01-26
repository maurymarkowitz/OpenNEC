Differences between OpenNEC, NEC-2 and nec2c
============================================

OpenNEC is a re-implementation of the nec2c code, which is a reimplementation of the original Fortran NEC-2 code. The differences between onec and nec2c are much greater than those between nec2c and NEC-2.

This document describes these changes, which include changes to the original nec2c, as well as the large number of additions and features not found in the original code.

Changes in the code
-------------------

- nec2c is in the form of a stand-alone command-line program. Other programs wanting to use nec2c to perform calculations by running the program with a temporary deck file, and then parsing the resulting output file.
- onec is in the form of a library that can be directly included in other programs. Those programs can modify the deck and read the results directly without using files. A simple wrapper program is also included, which emulates the nec2c command-line interface.

- nec2c parses the deck card-by-card from the input file
- onec parses the entire deck in one pass, which allows it to perform whole-deck checks, as well as allowing other programs to modify the deck by adding or removing cards without using a file.

- nec2c can calculate only one file at a time.
- onec's command line can process multiple files.

- nec2c has considerable global state inherited from the original Fortran code's COMMON sections. This makes it non-reentrant and it cannot be used in a threaded fashion.
- onec has been refactored so there is no global state and is completely thread-safe. Programs can use the library to work on multiple decks, and the command shell can run multiple input files at the same time.

- nec2c uses the original Fortran matrix calculation code. A number of forks of nec2c support one matrix library or another.
- onec supports a wide variety of well-known matrix libraries across multiple platforms. These offer major performance improvements on large files (3x on 1000 segments, 7x on 4000). You can compare the performance by running the script in the speed_tests folder. On Apple platforms, Accelerate will be linked by default as this is always available.

Other basic changes
-------------------

- OpenNEC includes extensive input validations that test for errors in the deck setup, like `FR`s lacking a frequency, or that `GN` must be immediately followed by `GD`. These can be run against any NEC-2 compatible deck. These are emitted as warnings and do not prevent calculation in cases where other engines are permissive, but they help make decks more portable across implementations.

- OpenNEC provides a `-g` option to export Green's function data (NGF). When enabled, segment centers and the interaction matrix are written per frequency step.

OpenNEC additions
-----------------

OpenNEC also includes a number of significant additions to the basic NEC-2 system:

- OpenNEC allows measurement units to be defined on a per-field basis. This is especially useful when defining wire radii; in an OpenNEC file a wire can be defined as "3awg" instead of having to replace that with the measurement in meters, "0.0058268". Different measurements can be used on different fields on the same card, or different cards. A `GS` card, if present, will *not* override those fields that have explicit measurements.

- OpenNEC adds a generic extension mechanism using key/value pairs that can be used by 3rd party software to add functionality without changing the underlying deck format. For instance, one could add "material:copper" to a card, and a GUI application using OpenNEC could then apply a copper color in a 3D model. These extensions are stored within an inline comment, so they have no effect on the NEC-2 code. A utility method allows these to be stripped out to produce a new deck that is compatible with generic NEC implementations.

  - Extension keys should always be compared in a case-insensitive manner. Keys should always be *written* in lower-case no matter how they were entered.

  - Three extensions are known to the basic OpenNEC system: `name`, `group`, and `invisible`. `name` and `group` are free-form strings intended to allow GUI-based applications to provide richer information and/or group related sections of the deck together.  `invisible` is used to suppress the element in GUI displays of the geometry, while still being used in calculations.

  - Additionally, 3rd party software using OpenNEC should be aware of these non-required GUI-related extensions: `material` and `shape`. `material` is a free-form field but a number of common materials are defined. `shape` is used to control the cross section in the display, for instance `shape=square` might be used when defining a boom on a Yagi antenna.

- The key/value entries can be entered in a variety of formats, see the "OpenNEC file format" document for details.

Additions from other systems
----------------------------

A number of features commonly found in other popular NEC-based programs have been added:

* OpenNEC includes a whole-deck sanity check system that looks for common errors, like overlapping wires or wires touching ground. It also reports on more minor issues like missing CE or EN cards that can cause problems with some NEC-2 implementations. This allows OpenNEC to be used as a stand-alone syntax checker. The list of issues is in an exposed structure, Errors, which allows further tests to be implemented in external software and then added to existing errors lists to keep everything in one place.

  - Note: Many validations are emitted as warnings (non-fatal) to preserve compatibility with existing decks while highlighting potential issues.

* OpenNEC notices cards using the 4nec2 convention with tag numbers >= 9800 < 9900 and sets `invisible=true` on those cards. 

* OpenNEC supports the `SY` card type from 4nec2. This is used to define variables, or SYmbols, which are parsed out in the deck. These are useful for defining the radius of wires and similar tasks, as well as making the deck more self-documented.

* OpenNEC supports in-line formulas, also found in 4nec2 decks. These allow you to define a symbol and then perform basic math operations on it, like "height+5". This has many uses, especially during optimizations. OpenNEC adds the ability to define these formulas in the extensions instead of directly in the card fields. OpenNEC can save files in 4nec2 format with these items "exposed", or in OpenNEC format with them hidden in comments so that the resulting deck is NEC-2 compatible.

* OpenNEC supports the `XT` card type from nec2c, which stops processing at that point. In contrast to nec2c, which simply exits the program when an `XT` is encountered, OpenNEC will still read the remaining cards but will not process them, effectively treating them as comments.

* OpenNEC supports the `#` comment marker from nec2c. This was used to insert whole-line comments at any point in the deck, a system that does not appear to be widely used. Note that the `#` comment marker can only appear at the start of a line and cannot be used to insert end-of-line comments. At any other location in the line, # is used as it is in 4nec2, to indicate a field is using AWG measurements. The leading-`#` is included only for compatibility, it is not considered to be part of the OpenNEC standard and should not be used except to produce nec2c compatible files. The modern replacement is `!`.

Defined constants
-----------------

OpenNEC defines a number of constants used in the extensions that 3rd party software should be aware of.

- `ignore` is an example of a boolean value which is indicated using `true` and `false`. During reading, the values `yes` and `no` or `1` and `0` may also be used, but these will be converted to `true` and `false` on write. As always, these are all case insensitive.

- the `material` may be any value, but the following values should be expected; `silver`, `copper`, `aluminum`, `6061-T6`, `6063-T832`, `brass`, `phosphor bronze` and `steel`. This list was based on the materials from Yagi Optimizer.

- `shape` may also be any value, but `circle` and `square` should be expected
