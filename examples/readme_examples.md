Example antenna models
======================

This folder contains a number of example antenna model files found on the 'net, which have been used to drive development of the OpenNEC parser.

You can run the script `examples/check_all_nec_files.sh` which will run onec with `-r --skip-large` on all of the sub-folders, which is useful for regression testing.

## DL5SAY collection

An eclectic collection of NEC files gathered by Hartmut Hans Kreh (DL5SAY). Found on github, posted by PA3KJ here:

https://github.com/Kees-PA3KJ/DL5SAY-Collection

These files are particularily useful for parser validation as they come in almost every format you can imagine. There's files with space-delimited columns, tabs, commas, and even examples with the cards split into multiple lines due to the insertion of line feeds (apparently) due to edits in a 3rd party text editor. If you can parse these files, you can parse just about anything you'll find.

## nec2++ examples

A selection of files used to validate the nec2++ engine. These mostly overlap the DL5SAY collection. One oddity is the `hang.nec` file with demonstrates the infinite loop in older NEC engines that occurs when the geometry is connected in a loop. This file was useful for finding and fixing this bug in OpenNEC. These files can currently be found at:

https://github.com/tmolteno/necpp/tree/master/testharness/data

## 4nec2 examples

A collection of several hundred NEC files, mostly in 4nec2 format, as well as a number of EZnec and MiniNEC files that have been converted to NEC format. This collection contains practically every possible combination of 4nec2 extensions. The collection is part of the 4nec2 ZIP download, currently found here:

https://qsl.net/4nec2/4nec2zip.zip

## ARRL examples

A small collection containing a number of NEC files that overlap the DL5SAY collection, along with a number of files for the Yagi Optimizer program in .YAG format. The files can currently be found here:

https://www.arrl.org/antenna-modeling-files

## cocoaNEC examples

A small collection of files from the macOS program cocoaNEC. These are in .nc and .nec format, the latter of which is not actually NEC but the program's own internal "spreadsheet format". The .nc files were used to create the cocoaNEC importer in OpenNEC. The spreadsheet format is in XML and is not currently supported. The collection (in Mac DMG format) can currently be found here:

http://www.w7ay.net/site/Downloads/cocoaNEC/Examples.dmg

Although it is not currently part of this collection, another set of NC example files can be found here:

https://github.com/rnistuk/Antennas

## DF9CY EZNEC files

A collection of EZnec format .EZ files. Most of these are just variations on a small number of designs, but still useful for comparison with the Cebik collection. The files can be found here:

https://www.df9cy.de/tech-mat/cy-ez-files/DF9CY-EZNEC-files.zip

## Not included

The amazing Cebik collection can be found here:

https://github.com/antenna2/cebik/tree/main/content

The repo contains thousands of model files in .NEC and .EZ format. The repo also includes many documents and PDFs explaining these files. It is extremely useful for validation, but too large to post in this repo. 