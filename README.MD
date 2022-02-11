OpenNEC
=======

OpenNEC is an implementation of the NEC2 code written in the C programming language. It is used to simulate the reception and transmission patterns of radio frequency antennas. The antenna's physical design is stored on a series of "cards", and a collection of cards is known as a "deck". Cards are physically represented by lines of text in a text file representing a deck. The files are normally saved with a ".nec", ".deck" or ".onec" extension.

OpenNEC concept
---------------
OpenNEC's code ultimately traces its history to the NEC-2 release from Lawrence Livermore in the early 1980s. There have been many implementations of this code, both in the original Fortran and many other languages. One such port to C was made by Neoklis Kyriazis in the nec2c program which was first released on 20 August 2003. OpenNEC was originally forked from the 1.3 version of nec2c, released in 2014.

The primary difference between OpenNEC and nec2c is its form; nec2c is a command-line program, while most of the OpenNEC code is in the form of a static library. This allows OpenNEC to be easily embedded in any program written in a language that provides access to C libraries, like Java or Swift. Additionally, the optional main.c wraps the .a to provide a command line tool like the original in nec2c.

As part of this conversion, many of the shared variables used to pass information around nec2c were moved into malloc'ed structures. These are exposed and can be used by client software, instead of being hidden within the code as it was in the past. This is particularly useful during the antenna layout and design stage; programs can access the segment data directly and create a 3D representation without having to parse the data from a file. It also means that most of the code is thread safe, due to the elimination of global variables.

OpenNEC additions
-----------------

OpenNEC includes two significant additions to the basic NEC-2 system:

* OpenNEC allows measurement units to be defined on a per-field basis. This is especially useful when defining wire radii; in an OpenNEC file this can be defined as "3awg" instead of having to replace that with the measurement in meters, "0.0058268". Different measurements can be used on different fields on the same card, or different cards. Note that `GS` and `GM` will still modify these, so even if you specify 3mm, if there is a `GS 0 0 0.5` below it, the resulting measurment will be 1.5mm.

* OpenNEC adds a generic extension mechanism using key/value pairs that can be used by 3rd party software to add functionality without changing the underlying deck format. For instance, one could add "material:copper" to a card, and the application could then apply a copper color in a 3d model. These extensions are stored within an inline comment, so they have no effect on the NEC-2 code. A utility method allows these to be stripped out to produce a new deck that is compatibile with generic NEC implementations.

* Five "built in" extensions are part of the OpenNEC system: name, group, material, ignore, and invisible. Name and group are free-form strings intended to allow GUI-based applications to provide richer information and/or group related bit of the deck together. material is a string that is up to the GUI to interpret. ignore tells the system to eliminate this card's data from the deck when being used for calculations, this is useful during testing as cards can be "turned off" without having to remove them or comment them out. invisible is used to surpress the element in GUI displays of the geometry.

Additionally, a number of other features commonly found in popular NEC-based programs have been added:

* OpenNEC includes a whole-deck sanity check system that looks for common errors, like overlapping wires or wires touching ground. It also reports on more minor issues like missing CE or EN cards that can cause problems with some NEC2 implementations. This allows OpenNEC to be used as a stand-alone syntax checker. The list of issues is in an exposed malloc'ed structure, Errors, which allows further tests to be implemented in external software and then added to existing errors lists to keep everything in one place.

* OpenNEC supports the `SY` card type from 4nec2. This is used to define variables (SYmbols), which are parsed out in the deck. These are useful for defining the radius of wires and similar tasks, as well as making the deck more self-documented.

* OpenNEC supports in-line formulas, also found in 4nec2 decks. These allow one to define a symbol and then perform basic math operations on it, like "height+5". This has many uses, especially during optimizations. OpenNEC also allows these to be defined in the extensions, which produces a more NEC2-compatible deck.

Acknowledgements
----------------
* OpenNEC is a fork of nec2c by Neoklis Kyriazis
* OpenNEC's function parser is based on tinyexpr, written by Lewis Van Winkle
* OpenNEC would not be possible without stackoverflow
