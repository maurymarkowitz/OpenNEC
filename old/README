
  README File for nec2c

  0. IMPORTANT!: As I am getting on in age (I am 1948 vintage), I have decided
  to spend more time on easier and possibly more pleasant activities, like 
  gardening and photography. Another pressing reason for moving away from
  active work on computers is the increasing complexity of FOSS operating
  systems, which have recently made it more or less impossible for me to
  install a distribution that would work well with all my peripherals, notably
  my printer, sound card and WIFI adaptor.

  Therefore I will only undertake to fix minor bugs in the applications I have
  written and provide some help with setting up and configuring, if needed. I
  can no longer undertake to implement new large scale and difficult feature
  requests and modifications to my applications, however anyone interested in
  taking over maintenance and development is welcome to peruse freely any
  source code I have written, respecting the GPL license where applicable.

  1. INTRODUCTION:
  nec2c is a translation of the NEC2 FORTRAN source code to the C language.
  The translation was performed mostly "by hand" and a lot of modifications
  to the original program were introduced in order to modernize the NEC2
  and to remove as many built-in limitations as possible. The attendant
  SOMNEC program was also translated to C and incorporated in nec2c as a
  function so that Sommerfeld ground solutions are a part of the program.

  2. CHANGES:
  The following is a list of the more significant changes incorporated into
  nec2c during translation from FORTRAN to C:

  * All GO TO constructs have been removed (all 961 of them!) and "spaghetti"
  code sections untangled as far as was possible to the author. Still, a lot
  of the code is not as clean and straightforward as might have been.

  * Obsolete memory-saving practices (such as "equivalences" of different
  variables) have been eliminated and memory-sharing variables have been
  separated so that they are independent.

  * All fixed-size arrays used in calculations have been replaced with
  buffer pointers which are allocated memory dynamically according to the
  needs of the program and the complexity of each structure's geometry.
  There is a two-fold advantage in this - there is virtually no limit to
  the complexity of a structure (number of segments/patches etc), and there
  is no wasted memory in fixed arrays. Additionally, there is no need for
  data storage/swapping between memory and files and therefore functions
  relating to this activity and also the NGF form of solution have been
  removed from the program.

  * When a Sommerfeld finite ground solution is requested, since the
  SOMNEC program has been incorporated in nec2c there is no need to store
  the ground grid data in a file and read it when running nec2c. Instead,
  ground grid data are calculated as needed and for each new frequency if
  frequency stepping is specified.

  * The factr() and solve() functions have been modified to handle the
  main matrix (cm) in untransposed form so that calculations are faster.

  * The parser that reads the input file allows the two characters of the
  mnemonic to be in lower case if preferred. It also allows comments to be
  inserted anywhere in the input file in Unix style, e.g. all lines
  beginning with a '#' are ignored.

  * Operationally, nec2c differs from NEC2 in not being an interactive
  application. Instead, nec2c is a non-interactive command-line application
  which accepts an input file name and optionally an output file name.
  If this is not specified, a name for the output file is made by stripping
  any extensions from the input file name and adding a ".out" extension.
  Furthermore, nec2c has the potential of being incorporated in another
  application (like a GUI) after suitable modifications, allowing the
  creation of a stand-alone program without the need for reading files
  produced separately.

  * My original motive for translating NEC2 into C was to make it easier
  to modify and modernize and to change obsolete functions and usage. As
  a result I have edited to some extend the format of the output file to
  make it more "human readable" (e.g. provided a single space between
  adjacent numbers to avoid a hard-to-read "chain" of numbers joined by
  - signs) etc. In my humble opinion these changes make the output file
  easier to read and possibly somewhat more presentable, although this is
  likely to be a problem with applications that read the output file in a
  rigid manner, based on the exact output format. I apologize for this
  change if it causes such problems but my intention is to eventually
  modify nec2c to be used as part of a graphical application, providing
  results for graphical plots directly in its buffers.

  3. COMPILATION:
  Please note that I use Arch Linux AMD64 which is a "bleeding edge"
  type distribution, so there may be compilation and/or run time
  difficulties if you are using a relatively old distro.
  
  To compile the package, it may be preferable to first run the
  included "autogen.sh" script in the package's top directory, to
  produce a fresh build environment. Then the "configure" script can
  be run with optional parameters to override the default settings
  and compiler flags, e.g: ./configure --prefix=/usr CFLAGS="-g -O2"
  will override the default /usr/local installation prefix and the
  "-Wall -O2" compiler flags.
 
  Running "make" in the package's top directory should produce the
  executable binary in src/. Running "make install" will install the
  binary into /usr/local/bin by default or under the specified
  prefix. There is also this hypertext documentation file which
  you can copy to a location of your choice.

  4. USAGE: nec2c is run as a non-interactive command-line application
  and is invoked in the following manner:
  nec2c -i<input-file-name> [-o<output-file-name>][-hv]
  -h: print this usage information and exit.
         -v: print nec2c version number and exit.

  The -i option is always needed and it specifies the name of the input
  file. The -o switch is optional and it specifies the output file name.
  If not used, a name for the output file is made by stripping any
  extensions from the input file name and adding a ".out" extension, e.g.
  nec2c -i yagi.nec will cause nec2c to read yagi.nec as the input file
  and produce yagi.out as the output file.

  5. BUGS!!
  Translating such a complex and large program from FORTRAN to C and making
  so many changes along the way is very prone to bugs in the new program.
  I have fixed a lot of these by using various input files that hopefully
  invoke most if not all of NEC2's functions but there must still be bugs
  in nec2c that will surface with some specific combinations of "cards" in
  some input file. The best way to check nec2c's results is to run nec2dx
  with the same input file and compare results - there should be very close
  agreement between them as nec2dx is also double-precision.

  6. Version history:
  Version 0.1 beta: First release of the translated NEC2

  Version 0.2: I used the "valgrind" (http://valgrind.kde.org)
  tool to check nec2c and found two significant bugs in intrp() and
  subph()   which I (hopefully!) have fixed. I also fixed another bug
  that was found by Tim Molteno in the netwk() routine.

  If you intend to use valgrind (recommended!) to test nec2c for bugs
  (mainly memory allocation/access errors) then do not use performance
  enhancing C flags (e.g. do not use -Ox flags etc) otherwise you will get
  false error reports.

  Version 0.3: I have split nec2c.c into a number of smaller files to
  make it easier to work on during bug-fixing or development.

  Version 0.4: Fixed a bug in conect that caused segmentation faults
  when only one wire segment exists in the structure. this is a case that
  will probably never exist in practice but the seg fault had to be
  fixed.

  Version 0.5: Replaced the cmplx() function with a macro to speed up
  calculations. Changed the fbar() and zint() functions from complex
  long double to void type and returned the calculated values via a
  pointer in the argument list. This was done to work around a bug I
  could never trace, possibly due to gcc itself, were functions of the
  complex long double type produce a NAN result on return.

  Version 0.6: Fixed a bug inherited from the original NEC2 FORTRAN
  code. Please see NEC2-bug.txt for details.

  Version 0.7: After a bug report from Juha Vierinen regarding seg
  faulting of xnec2c, my graphical adaptation of NEC2, I changed
  all "sprintf" commands to "snprintf" to avoid buffer overruns.
  Following on the above changes, I revised all similar situations
  in nec2c source code and changed all "sprintf" commands to
  "snprintf" just in case. While going through the nec2c source
  code, I also fixed some minor bugs like typos and tidied error
  messages.

  Version 0.8: After a segmentation fault bug report, I fixed the
  netwk() function in network.c to allow allocation of the ipnt buffer
  when maximum admittance matrix asymmetry printing is requested in the
  Ex card.

  Version 0.9: After a bug report by David J. Singer, I changed all
  declarations of variables that are used in memory allocations, from int
  to size_t. This error was in the nec2c code from the beginning but
  apparently it only showed up when extremely large memory allocations
  are requested in nec2c.

  Version 1.0: Changed timing calculations to be in sec rather than msec,
  as there are indications that for very long execution time, (working on
  a huge number of wire segments and frequencies etc) negative times are
  reporeted, possibly due to numerical overflows.

  Version 1.1: I made extensive modifications to the source code to
  silence a large number of warnings generated by the  LLVM clang
  compiler when used with the -Weverything option. These  were mostly
  cases of implicit conversions between variable types, like int to
  char or uint to int etc. Also introduced an Autotools build system
  for compiling and installation.

  Version 1.2: Fixed four issues (defects) reported by Coverity Scan
  (https://scan.coverity.com/). Two of those were in the somnec
  functions and I suspect that they did effect the calculations in
  the rom1() and gshank() functions, at least under some circumstances.
  Unfortunately the original Fortran source (somnec.for), as the rest
  of the NEC2 Fortran source, is extremely difficult to read so I am
  not able to confirm the impact of those reported defects.

  Version 1.3: Revised the fix of one issue reported by Coverity Scan,
  as on more detailed comparison with the original Fortran code, it
  seems the original changes (in the ROM1 function in SOMNEC) were
  not correct. Unfortunately I cannot verify the correctness of the
  above fixes, as nec2c does not print out SOMNEC output files.

  7. License:
  nec2c is Public Domain, same as the original FORTRAN source.
  Please keep any software you write incorporating nec2c in Public
  Domain or at least use an open license like GPL or BSD.

  8. AUTHOR:

  Neoklis Kyriazis

  February 12 2013
