OpenNEC, NEC-2 and nec2c
============================================

OpenNEC is a re-implementation of the nec2c code, which is a reimplementation of the original Fortran NEC-2 code. The differences between onec and nec2c are much greater than those between nec2c and NEC-2.

This document describes the main changes, which include changes to the original nec2c, as well as the large number of additions and features not found in the original code.

Changes in the code
-------------------

- nec2c is in the form of a stand-alone command-line program. Other programs wanting to use nec2c to perform calculations by running the program with a temporary deck file, and then parsing the resulting output file.
- onec is in the form of a library that can be directly included in other programs. Those programs can modify the deck and read the results directly without using files. A simple wrapper program is also included, which emulates the nec2c command-line interface.

- nec2c parses the deck card-by-card from the input file.
- onec parses the entire deck in one pass, which allows it to perform whole-deck checks.

- nec2c can calculate only one file at a time.
- onec's command line can process multiple files, whole directories, and can recurse through directories.

- nec2c has considerable global state inherited from the original Fortran code's COMMON sections. This makes it non-reentrant and it cannot be used in a threaded fashion.
- onec has been refactored so there is no global state and is completely thread-safe. Programs can use the library to work on multiple decks, and the command shell can run multiple input files at the same time.

- nec2c uses the original Fortran matrix calculation code. A number of forks of nec2c support one matrix library or another.
- onec supports a wide variety of well-known matrix libraries across multiple platforms. These offer major performance improvements on larger files (3x on 1000 segments, 7x on 4000). You can compare the performance by running the script in the speed_tests folder. On Apple platforms, Accelerate will be linked by default as this is always available, on other platforms the library selection is manual.

- nec2c added initial support for green's files, but never implemented it.
- OpenNEC has complete green's file support, and a green's file can be written using the -g flag on the command line, bypassing the need to add a WG card to the deck.

Other basic changes
-------------------

- OpenNEC includes extensive input validations that test for errors in the deck setup, like a missing `GE` card, `FR`s lacking a frequency, or a `GN` that is not followed by `GD`. These tests can be run against any NEC-2 compatible deck. These are emitted as warnings and do not prevent calculation in cases where other engines are permissive, but they help make decks more portable across implementations.

- The tests also include a geometry and calculation sanity check system that looks for common errors like overlapping wires or wires touching ground. This is currently limited in scope, but will be expanded over time.
    - Note: Many validations are emitted as warnings (non-fatal) to preserve compatibility with existing decks while highlighting potential issues.

- OpenNEC also includes per-field validations that can be used by a GUI program to graphically indicate problems. For instance, if the user makes a new FR card, the validation functions will indicate that the base frequency value in the F1 field needs to be entered. If they enter a value in I2, which indicates steps, it will indicate that a step value has to be entered in F2. There is an extensive suite of these validations that can be tied to fields in the GUI and updated in real-time.

- OpenNEC has extensively updated error reporting that, wherever possible, reports the card and tag that caused the issue. This makes debugging stacks much easier.

- OpenNEC fixes the notorious bug that caused loops in the geometry to cause the program to go into an infinite loop. The program exits gracefully in this case, with a clear error message.

- OpenNEC has a simple timing function inspired by the original NEC-2 user manual that can be used to estimate the time it will take to run a calculation. This can be used in a GUI program to decide whether it can run these in real-time as the user edits the layout.

Additions from other systems
----------------------------

A number of features commonly found in other popular NEC-based programs have been added:

* OpenNEC supports the `SY` card type from 4nec2. This is used to define variables, or SYmbols, which can be used in place of numbers in the rest of the deck. These are useful for defining the radius of wires and similar tasks, as well as making the deck more self-documented. A common example is to use something like `SY rad=0.01` to define a 1 cm radius, and then use the variable `rad` instead of typing `0.01` everywhere. The advantage is that you can experiment with changing the radius by editing a single card.

* OpenNEC supports in-line formulas, also found in 4nec2 decks. These allow you to define a symbol and then perform basic math operations on it, like "height+5". This has many uses, especially during optimizations. OpenNEC adds the additional ability to define these formulas in the extensions instead of directly in the card fields. OpenNEC can save files in 4nec2 format with these items directly in the fields, or in OpenNEC format with them hidden in comments so that the resulting deck is NEC-2 compatible. In the latter case, the calculated value is placed in the field so it remains NEC-2 compatible during calculations.

* In-line formulas can be used to support measurement units on a per-field basis. For instance, one can use "1ft" to define the span of a wire. Internally, this is converted to the formula "1*ft", where ft is a variable with the conversion from feet to meters. This is the same basic logic used in 4nec2.

* OpenNEC supports the `XT` card type from nec2c, which stops processing at that point. OpenNEC will still read the entire deck, but will only process up to the point of the XT. This allows additional cards to be placed in the deck but ignored during processing, and those can be "turned on" by removing the XT.

* OpenNEC supports the `#` comment marker from nec2c. This was used to insert whole-line comments at any point in the deck, a system that does not appear to be widely used in other systems. Note that the `#` comment marker can only appear at the start of a line and cannot be used to insert end-of-line comments. At any other location in the line, # is used as it is in 4nec2, to indicate a field is using AWG measurements. The leading-`#` is included only for compatibility, it is not considered to be part of the OpenNEC standard and should not be used except to produce nec2c compatible files. The modern replacement is `!`.

OpenNEC additions
-----------------

OpenNEC also includes a number of additions to the basic NEC-2 system:

- OpenNEC adds a generic extension mechanism using key/value pairs that can be used by 3rd party software to add functionality without changing the underlying deck format. For instance, one could add "material:copper" to a card, and a GUI application using OpenNEC could then apply a copper color in a 3D model. These extensions are stored within an inline comment, so they have no effect on the NEC-2 code. A utility method allows these to be stripped out to produce a new deck that is compatible with generic NEC implementations.
    - Extension keys should always be compared in a case-insensitive manner. Keys should always be *written* in lower-case no matter how they were entered.

- Four extensions are known to the basic OpenNEC system: `name`, `group`, `ignored` and `invisible`.
    - `name` and `group` are free-form strings intended to allow GUI-based applications to provide richer information and/or group related sections of the deck together.
    - `ignore` is used to calculate the object's geometry but ignore it during calculations. This can be used, for instance, to add a boom that is displayed in the GUI to more closely represent a real antenna, but has no effect on the calculations.
    - `invisible` is used to suppress the element in GUI displays of the geometry, while still being used in calculations. This is useful when there are elements placed a long distance from the main antenna, which might otherwise cause the GUI to zoom out too far when rendering.
        - OpenNEC notices cards using the 4nec2 convention with tag numbers from 9800 to 9899 and sets `invisible=true` on those cards.

- 3rd party software using OpenNEC should be aware of these non-required GUI-related extensions: `material` and `shape`.
    - `material` is a free-form field but a number of common materials are defined.
    - `shape` is used to control the cross section in the display, for instance `shape=square` might be used when defining a boom on a Yagi antenna.

- The key/value entries can be entered in a variety of formats, see the "OpenNEC file format" document for details.
