OpenNEC file format
===================

Version 1.0, January 2026

Introduction
------------

OpenNEC, or onec for short, is an implementation of the Numerical Electromagnetics Code ("NEC") system for simulating antennas. It is based on a port of the original NEC-2 code converted from Fortran to C by Neoklis Kyriazis, called nec2c.

NEC systems use text files known as "decks" to exchange design information between antenna simulator programs using the NEC-2 and NEC-4 programs, or compatible systems like nec2c, 4nec2, MININEC, and many others. The OpenNEC file format is an extended version of the *de facto* NEC file format, adding a small number of new features and more clearly specifying some formerly ill-defined features.

The onec file format also adds a feature that allows the format to be arbitrarily extended without effecting the underlying NEC data, using inline comments. Most modern NEC implementations can read onec files without modification. onec files can be easily converted to a more generic NEC-2 format, so any pure NEC-compatible program can read a converted onec file with no loss of information. 

Background
----------

The NEC code traces its history to BRACT, written in Fortran in the 1960s. As was the case for most programs of that era, BRACT was a single large program that parsed user input from a card reader, ran the calculations, and then output the results to a line printer. The code mixed these functions together, processing the data piece-by-piece in order to reduce temporary memory requirements. Because paper cards were expensive, the input format contains a number of features intended to reduce the total card count. This results in a number of variant formats for cards, where the number and meaning of fields on the cards changed depending on values in other fields, or values on earlier cards in the deck. The system is highly modal.

Starting in the 1970s the original BRACT code was updated several times, becoming NEC. As part of these modifications, the input was changed to text files by recording each 80-character card into a single line of text in a file. The resulting file was known as a deck, in keeping with it's origins as a deck of cards. The Fortran code for the NEC-2 version was released into the public domain in the early 1980s. A number of ports of this code have been made, most recently in the C or C++ languages. The format of a deck was not formally defined when these ports were being made, and as a result there are a profusion of slightly different versions. Matters were not helped when the original NEC code was updated, introducing still more variations as part of NEC-3 and NEC-4.

For example, almost all deck formats allow comments to be placed at the end of a line (or "card") to provide documentation. However, the comment indicator varies across implementations. The original NEC used `CM` only at the start of a line, but later implementations added `!`, `'` and `#`. Additionally, the individual fields, formerly delimited by their column number on the punch cards, are now what the NEC-4 documentation calls "close to free format" - tabs, commas, spaces and even fixed-format versions can be found in use.

On top of all this, new card types like `SY` have been added by 3rd party software which add useful functionality but make the decks incompatible with other software.

Impetus
-------

OpenNEC is an attempt to solve (some of) these problems. It does so by defining a single canonical format for the files. It also includes an extension mechanism that allows the addition of arbitrary data to any card, in a way that will be safely ignored by (most) modern NEC implementations. onec also supports the SY card type, as this feature is both common and very useful in practice.

The inclusion of these new features means the onec format is not fully compatible with the original NEC format. However, onec defines a simple mechanism to convert any onec deck into a fully compatible form with no loss of information. This makes onec files easy to use in the traditional role where the NEC code is in an external library or executable that communicates only through files.

Design decisions
----------------

The design of the onec format started as part of an effort to canvas the internet looking for the most popular NEC editing tools. This revealed that there were a number of common extensions to the NEC format. Some of these were identical across various applications, so a single format would be able to support many of them. The key would be to include these in a new format in such a way that older software would still be able to open and use the decks, and potentially edit them.

The simplest solution would be to use the NEC comment card, `CM`, as a prefix on all extensions. Any data on a `CM` card is simply ignored by the NEC processor. This has been used by some programs to include additional parameters which only they notice. One could extend this mechanism to add new features; for instance, have a comment that states the 3rd measurement on a particular card is in millimetres, not the deck's default of inches set on an `GS` card later in the deck.

The basic idea is sound, but the original NEC-2 allows comment cards to be placed only at the start of a file. Using this mechanism would separate the extensions from the cards they applied to, which would make it more difficult to implement and harder to read. NEC-4 allows `CM` cards to be placed anywhere, but 3rd party implementations written before the release of NEC-4 generally don't support this. NEC-4 also introduced the `!` character to allow comments to be added to the end of any line, but other implementations had already selected other characters for this, and very few of them, if any, support the `!` marker. Among the popular alternatives are nec2c and 4nec2, which use the `#` and `'` symbols in the same way NEC-4 uses `!`.

None of these inline comment systems are compatible with software supporting the original NEC format, which would see them either as extraneous data, or bad data in the last field on the card. Such a deck can be converted to NEC compatible format by simply deleting any characters on a given line from the position of the comment character on. Hiding data behind an end-of-line comment means things like measurements can be defined on the same card, but stripping them off would require the system to convert the numeric values in the NEC fields.

Another decision was whether or not to directly support the `SY` card type. This card was introduced in the 4nec2 program, allowing the user to define a SYmbol, a variable. The name of the symbol can then be used in places where numbers would normally appear, the height of the antenna over the ground for instance. It would be possible to include the `SY` cards using the commented-out format, starting the line with `!SY` for example, but this alone would not make the resulting deck compatible with NEC because the *references* to the symbols in the card fields would still cause errors. One could place those references in a trailing comment, but they would still have to be calculated and copied into the appropriate field before sending it to NEC. For that reason, the decision was made to also treat `SY` as a formally supported card type in onec. 

NEC was designed to measure everything in meters, but this is not convenient for many antenna designs, like those in a cell phone that might be only a few millimeters long. Most GUI-based NEC programs allow values to be input using other units, and then convert them on entry to meters. Some add the ability to choose a default measurement type, and convert that to meters using NEC's scaling factor card (`GS`). All of these have the disadvantage that the original measurement type is lost on input. For instance, if the user selects `#14` for the wire radius in a GUI program, this would be converted to 0.000814 meters in the NEC deck. While this is technically sufficient, it loses the original intent of the user and reduces the readability of the deck. The inclusion of measurement markers, like `mm`, is another 4nec2 feature that seems too useful to ignore.

NOTE: The short-form symbols for feet, ', and AWG, #, are used for comment markers in common NEC software like nec2c. For this reason, onec stores these as `ft`, `in` and `awg` in the file. User-facing software should convert this *only in the display*, if desired by the user, and should *never save these symbols to the file*. 4nec2 files do allow `#` as the AWG marker in files, so onec treats `#` as the comment marker only at the start of the line.

Finally, onec was being built with the assumption it would not normally be used as a stand-alone program, but as part of a GUI application. These often add additional data to aid with the graphical display of the antenna design. onec includes a new feature that allows key/value pairs to be inserted on any line as a trailing comment. These will be parsed out separately and presented to the client software without being visible to the underlying NEC implementation. Examples include `!material=copper` which an application might use to appropriately color a particular element on the display.

As this mechanism uses the inline comment system, and users will likely mix comments and key/value pairs on a single card, the system allows multiple entries to be separated by commas or semicolons. Inline comments are first separated by these delimiters, and then each result is examined for an `=` or `:`. Those entires containing such a value are assumed to be key/values, everything else is concatenated into a single comment.

Defined extensions
------------------
The onec extension system is intended to be largely free-form, edited by the user or applications calling the onec library. There are a small number of defined extensions all 3rd party software should support:

- `name` allows a single element to be given a name. This is typically used in the geometry section and might have values like `reflector` or `boom`.

- `group` is used to collect multiple elements together. In a GUI application, this might be used with a disclosure widget to allow sections to be collapsed down to the group name, like `upper reflector`.

- `ignore` indicates whether or not the card should take part in the calculations. Setting this to `true` causes that card to be ignored during processing. This is useful during the development or testing of a deck, as a card can be ignored in the calculations without having to physically remove it or mark it as a comment card. 4nec2 has a similar feature that ignores all cards with tag values between 9800 and 9899, and onec also follows this rule this as well, but the explicit `ignore` tag is more obvious and doesn't require you to change the original tag number.

 - `comment` marks everything after that point on the line (and the following separator) to be a comment. This may seem redundant as it would be placed within an in-line comment, but it included to allow key/value pairs and comments to be placed on the same card, although the `comment` has to be the last value on the line.
 
 Additionally, a number of additional extensions are expected to be supported by programs that provide a graphical display.
 
 - `visible` indicates whether the geometry on the card should be visible onscreen. The default is `true`. Changing this to `false` indicates it should not be drawn on-screen. This does not remove it from the calculations, it is a visual effect only. This is generally used to limit the bounding box of the resulting design, so the GUI can calculate a useful camera position in the case when there are elements placed a long distance from the "main" antenna.

- `shape` is used to change the shape of the geometry for GUI programs. The calculation engine does not care about the shape of the elements, but the user of a GUI program might. By adding something like `name=boom, ignore=true, shape=square`, the boom on a Yagi antenna can be added to the file to make the display of the antenna more accurate without effecting the output. At a minimum, `circle` and `square` should be supported, along with any other shapes the GUI software might wish to add.

- `material`  is a similar display-only extension and may be any value, but the following values should be expected; `silver`, `copper`, `aluminum`, `6061-T6`, `6063-T832`, `brass`, `phosphor bronze` and `steel`. This list was based on the materials from Yagi Optimizer.

Converting from onec to NEC
---------------------------

The overriding design goal for the onec format is to have a simple way to convert the stack into a format that is fully compatible with NEC-2. This can be accomplished by removing certain bits of text from the deck, although individual programs may wish to apply additional logic to improve this process. There are three basic steps:

### parse and replace SY assignments
SY's are a simple "replacement" system in which any entry of an SY name in a data card is replaced with the string defined on the SY card. This is a two-step process:

1) scan the deck and extract all SY assignments, and remove those cards from the deck
2) find any use of those SY variable names, and replace them with the calculated value from (1)

### parse and replace measurement units
onec adds the ability to define units on a field-by-field basis, so you can have elements that are 10ft long and 1cm in radius. These need to be converted to a common unit, and the unit markers removed:

1) scan the geometry cards for any unit indicators, like `cm` or `in`, replace those values with ones converted to meters

### remove inline comments and unused cards
NEC does not allow empty cards in the deck, but other formats allow this or some variation on the theme. These should all be removed during the conversion to NEC-compatible format. There are several cases:

1) remove all leading whitespace on lines, before the card type marker
2) remove any card that is empty, consisting solely of a CR, LF or CR/LF
3) remove any card that starts with a non-NEC comment marker like `'`, `!` or `#`
4) scan the deck for leading comments, any card at the top of the deck marked with `CM` or `CE`. this section ends with the first card that starts with a `G`, or the `CE` card if present.
5) remove any cards that start with `CM` *after* that point. The original NEC format only supports comments at the top.
6) scan the geometry section and remove any cards containing upper or lower case versions of strings `ignore=true`, `ignore=yes`, etc. (see below for details). Also remove any cards with a tag value >=9700.
7) remove any remaining cards that are not part of the NEC standard. This would include any cards like `IT`.

The resulting deck is now compatible with any known NEC parser.

Other notes
-----------

There are other file formats used in the antenna design world that may be of interest as they can easily be converted to onec format. Most of these are historical and no longer used, but examples are still found on the 'net. Most notable among these was Brian Beezley's Yagi Optimizer application. These files do not have a consistent extension, although ".ANT" and ".YO" is sometimes seen. These files look like modified NEC-2 decks lacking card codes, but are quite different in format.

onec format definition
----------------------

### deck
```
<deck> = 
  (* comment area *)
  { <nec comment card> } | { <onec comment card> }, <nec comment end card>,
  (* variable area *)
  { <onec variable card> } | { <onec comment card> },
  (* geometry area *)
  <geometry card> | { <onec comment card> }, { <geometry card> } | { <onec comment card> }, <geometry end card>,
  (* control area *)
  <control card> | { <onec comment card> }, { <control card> } | { <onec comment card> },
  (* end of deck area *)
  { <onec comment card> }, <deck end card>,
  { <freeform text card> } ;
```
A deck is the ultimate product of the onec file format. A deck must have at least a comment end card, at least one geometry card and a geometry end card, one or more command cards, a deck end card, and may be followed by one or more lines of freeform text. onec comment cards can be inserted at any point in the deck.

### comments area
```
<nec comment card> = "CM", [ <freeform text> ], <EOL> ;
<nec comment end card> = "CE", [ <freeform text> ], <EOL> ;
```
Comment cards do not require comment text, and are often found with no text as a way to insert vertical whitespace and make the deck easier to read.
```
<onec comment card> = <onec comment marker>, [ <freeform text> ], <EOL> ;
<onec comment marker> = "CM" | "!" | "'" | "#" ;
```
onec comment cards can be placed anywhere in the deck. They can be marked with the NEC-style `CM` or any of the newer comment markers - `!`, `'` and `#`. All text after the marker should be preserved, including any whitespace between the marker and the comment, if present. *Generally*, onec comments are not placed in the normal NEC comment area at the top of the file, but may be found immediately following it or anywhere else in the deck.

### variable area
```
<onec variable card> = { <onec comment marker> }, "SY", 
                       <variable name>, "=", <formula>,
                       { ",", <variable name>, "=", <formula> },
                       <EOL> ;
 
<variable name> = <letter>, { <letter> | <digit> | "_" } ;

<formula> = <factor>, { <operator> <factor> } ;

<factor> = <float value> | <variable name> | <function> "(" <factor> ")" ;

<operator> = "+" | "-" | "*" | "/" | "^" ;

<function> = "sin" | "cos" | "tan" | "atn"
           | "sqr" | "exp" | "log" | "log10"
           | "abs" | "sgn" | "int" | "mod" ;
```
Note: function names are case-insensitive. Trigonometric functions (`sin`, `cos`, `tan`) take angles in **degrees**; `atn` (arc tangent) also returns **degrees**. `log` is the natural logarithm (base e); use `log10` for base-10. `int` rounds to the nearest integer. `mod(val, div)` returns the remainder after division. `sgn` returns -1, 0, or +1 depending on the sign of its argument. `sqr` is an alias for square root, the `sqrt` variation is also available.

An onec variable card contains one or more variable definitions consisting of name=formula pairs. Variable names and formulas may not contain spaces or other whitespace characters. Variable names must start with a leading letter character followed by zero or more characters including digits and underscores. Formulas consist of one or more visible characters including numbers and mathematical symbols.

NOTE: formula names are case insensitive, p and P refer to the same variable, as does r1 and R1.

If more than one definition is supplied on a single line, they are comma separated. onec variables may be "hidden" by adding an onec comment marker to the front. Hiding of this sort, if present, should be preserved when the file is written.

onec variable cards can be placed anywhere in the file as long as they appear before the cards that reference them. *Generally* they will be placed between the comment and geometry cards, but this is not a requirement. 

### geometry area

<geometry card> = <GA card> | <GF card> | <GH card> | <GM card> | <GR card>
                | <GS card> | <GW card> | <GX card> | <SP card> | <SM card> ;
                
<geometry end card> = "GE" | "GE", <integer value>, <EOL> ;

At least one geometry card is required as well as a single geometry end card. All geometry cards must appear between the comment end card and the first control card.

The format of the geometry card inputs can be found in the NEC2 documentation. 

### control area
```
<control card> = <group 1 control card> | <group 2 control card> | <group 3 control card> ;

<group 1 control card> = <EK card> | <FR card> | <GN card> | <KH card> | <LD card> ;
<group 2 control card> = <EX card> | <NT card> | <TL card> ;
<group 3 control card> = <CP card> | <GD card> | <NE card> | <NH card>
                       | <NX card> | <PQ card> | <PT card> | <RP card> | <WG card> | <XQ card>;
```
The format of the control card inputs can be found in the NEC2 documentation. 

### end of deck area
```
<deck end card> = "EN", <EOL> ;

<freeform text card> = { <freeform text> }, <EOL> ;
```
Any text after the end of the deck is considered to be a free-form comment and does not require a comment marker.

### onec extensions
```
<onec extension> = <name>, "=" | ":", <formula>, { <onec separator>, <name>, "=" | ":", <formula> } ;

<inline comment marker> = "!" | "'" | "#" ;

<onec separator> = "," | " " ;
```
onec extensions consist of one or more key/value pairs following one of the accepted inline comment markers. Keys and values are separated by equals signs or colons. *Generally* formulas will use equals, while other onec tags will use colons. This is simply to make them more easily distinguished in the deck text.

### basic types
```
<freeform text> = ? any ASCII character ?, { ? any ASCII character ? }, <EOL> ;

<integer value> = [ "-" ], <digit>, { <digit> } ;

<float value> = [ "-" ] { <digit> }, [ ".", { <digit> } ] [ "E" | "e" , [ "-" ] <digit>, { <digit> } ] ;

<boolean value> = <true value> | <false value> ;

<true value> = "yes" | "y" | "YES" | "Y" | "true" | "t" | "TRUE" | "T" | "1" ;
<false value> = "no" | "n" | "NO" | "N" | "false" | "f" | "FALSE" | "F" | "0" ;

<letter> = "A" .. "Z" | "a" .. "z" ;

<digit> = "0" .. "9" ;

<EOL> = "LF" | "CR" | "CRLF" ;
```
