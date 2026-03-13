OpenNEC User Manual
===================

Introduction
------------
OpenNEC is an open-source implementation of the NEC (Numerical Electromagnetics Code) family of antenna simulation engines. It aims to be a lightweight, portable, and modern re‑implementation of the classic NEC‑2/NEC‑4 workflow, with support for recent extensions and improved deck validation.

This manual focuses on using OpenNEC as a library in other programs, and the internal structures and functions that you call from the library in your programs. It also includes guidance on generating and validating NEC decks, controlling simulations, import and export of other formats, and interpreting output.

Library API Overview
--------------------
### Initialization and cleanup
### Core data structures (context, decks, errors)
### Running a simulation (API calls and workflow)

Using OpenNEC as a Library
--------------------------
- High-level architecture (core library + CLI)
- Header files and public API
- Initialization and cleanup
- Performing a simulation from C/C++

Bindings and Integration Examples
--------------------------------
- Calling OpenNEC from Python
- Calling OpenNEC from Rust
- Calling OpenNEC from Java (JNI)
- Calling OpenNEC from other languages (e.g., Julia, Go)

Deck File Format (NEC Input)
---------------------------
- Card types overview (GW, EX, LD, GN, GE, FR, etc.)
- Card syntax conventions
- Symbolic expressions and unit suffixes
- Examples of common antenna geometries

OpenNEC Extensions
------------------
- Extensions beyond standard NEC-2 (additional LD, GN types, etc.)
- OpenNEC-only cards (if any)
- How extensions are documented and versioned

Output Formats
--------------
- Text output summary (impedance/loading/patterns)
- Field files and plotting support
- Debug output and intermediate files

Testing and Validation
----------------------
- Built-in deck validation logic
- Running the test suite
- Adding new regression tests

Import/Export
-------------
- short introduction to the import/export functionality and how to use it in your program
- short entry for each supported type which links to the related documents in the docs folder

Troubleshooting
---------------
- Common errors and their meaning
- Debugging deck parsing problems
- Working around numerical issues

Appendices
----------
- Glossary of terms
- NEC-2 card reference
-- comments
-- geometry
-- control
- NEC-4 card reference (new cards only)
- 4nec2 card reference
- onec extension reference
- AWG conversion table
- Wire conductivity
- Ground types
- References and further reading
- Change log / version history
