Differences between OpenNEC and nec2c
=====================================

OpenNEC is a re-implementation of the nec2c code, which is a reimplementation of the original Fortran NEC-2 code. The differences between onec and nec2c are much greater than those between nec2c and NEC-2. This document describes these changes.

This list does not include the OpenNEC-unique features like the key/value pairs, this is a comparison of just the parts of onec that are also in nec2c. The differences are presented here as pairs of points.

- nec2c is in the form of a stand-alone command-line program. Other programs wanting to use nec2c to perform calculations do so by running the program with a temporary deck file, and then parsing the resulting output file.
* onec is in the form of a library that can be directly included in other programs. Those programs can modify the deck and read the results directly without using files. A simple wrapper program is also included, which emulates the nec2c command-line interface.

- nec2c parses the deck card-by-card from the input file
* onec parses the entire deck in one pass, which allows it to perform whole-deck checks, as well as allowing other programs to modify the deck by adding or removing cards

- nec2c can calculate only one file at a time.
* onec's command line can process multiple files, and the library can work on any number of decks at the same time.

- nec2c has considerable global state inherited from the original Fortran code's COMMON sections. This makes it non-reentrant and it cannot be used in a threaded program.
* onec has been refactored so there is no global state and is completely thread-safe. Programs can use the library to work on multiple decks at the same time.

- nec2c uses the original Fortran matrix calculation code. a number of forks of nec2c support one matrix library or another.
* onec supports a variety of well-known matrix libraries across multiple platforms. These offer major performance improvements on large files (3x on 1000 segments, 7x on 4000). You can compare the performance by running the script in the speed_tests folder.
