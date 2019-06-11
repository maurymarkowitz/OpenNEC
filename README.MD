OpenNEC
======

OpenNEC is an implementation of the NEC2 code written in the C programming language. It is used to simulate the reception and transmission patterns of radio frequency antennas. The antenna's physical design is stored on a series of "cards", and a collection of cards is known as a "deck". These are physically represented by lines of text in a text file.

OpenNEC history
--------------------
OpenNEC's code ultimately traces its history to the NEC2 release from Lawrence Livermore in the early 1980s. There have been many implementations of this code, both in the original Fortran and many other languages. One such port to C was made by Neoklis Kyriazis in the nec2c program which was first released on 20 August 2003. OpenNEC was originally forked from the 1.3 version of nec2c, released in 2014.

The primary difference between OpenNEC and nec2c is its form; nec2c is a command-line program, while most of the OpenNEC code is in the form of a static library. This allows OpenNEC to be easily embedded in any program written in a language that provides access to C shared libraries, like Apple's Swift. A simple main.c is also included with this code to show how to use the library.

As part of this conversion, many of the shared variables used to pass information around nec2c were moved into malloc'ed structures. These can be used by client software, instead of being hidden within the calculation systems. This is particularily useful during the design stage, as one can access the segment data directly and create a 3D representation, without having to parse the data from a file. It also means that most of the code is thread safe, due to the elimination of global variables.

OpenNEC extensions
-------------------------
In addition to converting the system for use in a library, OpenNEC also includes a number of extensions on the original NEC2 format. Most of these come from popular NEC-based programs.

* OpenNEC includes a whole-deck sanity check system that looks for common errors, like overlapping wires. It also reports on more minor issues like missing CE or EN cards which can cause problems with some NEC2 implementations. This code can be used as a stand-alone syntax checker.
* OpenNEC supports the `SY` card type from 4nec2. This is used to define variables (or SYmbols), which are then cut-and-pasted into the deck as parsing continues. These are useful for defining the radius of wires and similar tasks.
* OpenNEC supports in-line formulas, also found in 4nec2 decks. These allow one to define a symbol and then perform basic math operations on it, like "height+5". This has many uses, especially during optimizations.
* OpenNEC allows measurement units to be defined on a per-field basis. This is especially useful when defining wire radii; in an OpenNEC file this can be defined as "3awg" instead of having to replace that with the measurement in meters, "0.0058268". Different measurements can be used on different fields on the same card, or different cards.
* OpenNEC adds a generic extension mechanism using key/value pairs that can be used by 3rd party software to add functionality without changing the underlying deck format. For instance, one could add "material:copper" to a card, and the application could then apply a copper color in a 3d model. These extensions are stored within an inline-comment, so they have no effect on the NEC2 code, and can be stripped out entirely.

Acknowledgements
-----------------------
OpenNEC is a fork of nec2c by Neoklis Kyriazis
OpenNEC's parser is based on tinyexpr, written by Lewis Van Winkle.
OpenNEC would not be possible without stackoverflow

