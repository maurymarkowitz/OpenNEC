OpenNEC User Manual
===================

Introduction
------------
OpenNEC is an open-source implementation of the NEC (Numerical Electromagnetics Code) family of antenna simulation engines. It aims to be a lightweight, portable, and modern re‑implementation of the classic NEC‑2/NEC‑4 workflow, with support for recent extensions and improved deck validation.

This manual focuses on using OpenNEC as a library in other programs, and the internal structures and functions that you call from the library in your programs. It also includes guidance on generating and validating NEC decks, controlling simulations, import and export of other formats, and interpreting output.

Using OpenNEC as a Library
--------------------------
- High-level architecture (core library + CLI)
- Header files and public API
- Core data structures (context, decks, errors)
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
- typical insulation

Aluminium-oxide 10
Bakelite 3.5 - 4.5
Copper-oxide 18.1
Glass 5.0 - 9.0
Glass (window) 7.6
Mica 4.0 - 8.0
Neoprene 4.0 - 6.7
Oil 1.5 - 4.7
Paper 1.6 - 2.6
Parrafin 2.0 - 3.0
Pertinax 4.3 - 5.5
Plexiglas 2.6 - 3.5
Polycarbonate 2.9 - 3.2
Polyethylene 2.4
Polyamide (nylon) 3.4 - 3.5
Polystyrene 2.4 - 3.0
Porcelain 5.0 - 6.5
PVC (hard) 3.0 - 4.0
PVC (soft) 4.0 - 5.0
Rubber 2.7 - 3.2
Shellac (Nat.) 2.9 - 3.9
Styrofoam 1.03
Teflon 2.1
Water (destil) 34 - 78
Wood (dry) 1.4 - 2.9
Most plastics appear to have a dielectric constant (permittivity) between 2.0 and 3.5. The dielectric
constant of air is around 1.0, so if we would specify a value of 1.0, no matter what radius we would get
the performance of bare wire.

- Ground types
- References and further reading
- Change log / version history
