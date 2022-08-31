Yagi Optimizer YO file format
=============================

Introduction
------------

YO is an antenna design file format introduced in the 1994 Yagi Optimizer application by Brian Beezley. Files in YO format have no consistant extension, but sometimes ".ANT" or ".YO" may be seen. They were common during the 1990s and 2000s, and examples are still found around the 'net today.

The application is solely for use with Yagi antennas, and uses the MININEC code to run its calculations. A key feature of MININEC is its ability to define tapering dimensions, in contrast to NEC which uses a formula to calculate a series of steps. YO makes extensive use of this MININEC feature and its files are structured to include this data in a way that requires conversion to use with NEC.

This document outlines the features of the YO format and describes how to convert the file to a canonical NEC or OpenNEC file.

Format definition
-----------------

  <YO file> = 
  <title line>,
  { <option line> },
  <frequency line>,
  <geometry line>,
  <taper line>,
  <length line>,
  { <taper line> }, { <length line> }, 
  { <freeform text line> } 
  ?EOF? ;

 <title line> = [ <freeform text> ], ?EOL? ;

(* The Yagi Optimizer application truncates any text beyond the 52nd character, but this is not a limitation of the file format itself and does not need to be adhered to. *)

 <option line> = <material line> | <height line> | <stacking line> | <dual line>, ?EOL? ;

 <height line> = "Height ", <float value>, { <unit abbr> }, ?EOL? ;

 <material line> = <material def> | <resistivity def> | <conductivity def>, ?EOL? ;
                
 <material def> = "Silver" | "Copper" | "Aluminum" | "6063-T832"
               | "Brass" | "Phosphor Bronze" | "Steel"
                      
 <resistivity def> = "Resistivity ", <float value>, { <float value> }

 <conductivity def> = "Conductivity ", <float value>, { <float value> }

(* Yagi Optimizer has several built-in values for conductor losses and assumes 6061-T6 aluminum with a value of 4.01E-08 ohm-m if no other material is defined. The documentation does not indicate if "6061-T6" is a valid input on its own.

Alternately, the material can be defined directly using Resistivity and supplying the value in ohm-m or Conductivity using IACS (International Annealed Copper Standard) units. Both Resistivity and Conductivity have an optional relative permeability, which defaults to 1 if it is not supplied. *)

<dual line> = "Dual" | "KLM", { <float value> }, { <float value> }

(* Adding Dual or KLM assumes a 230 ohm phasing line impedance and a 200 ohm feedline impedance. The first of the two optional inputs changes the phasing line impedance, the second changes the feedline impedance. *)

<frequency line> = <float value> 
                 | <float value> <float value> <float value>, { <float value> }
                 , { "KHz" | "MHz" | "GHz" }
                 , ?EOL? ;
                 
(* Frequencies can be given as a single number or three. If the three-number format is used, an optional transformer matching frequency can be supplied as a fourth number. The frequencies can be followed with the optional measurement units. If no unit type is supplied, "MHz" is assumed. *)

<geometry description line> = <integer value>, " elements", { "," ), " ", <unit name> | <unit abbr>, ?EOL? ;

<unit name> = "feet" | "inches" | "meters" | "centimeters" | "millimeters" | "wavelengths"
<unit abbr> =  "'" | '"' | "ft" | "in" | "m" | "cm" | "mm" | "w"

(* Only the first two letters of the unit specifiers are read, so "feet" and "fe" are equivalent. *)

<taper line> = { "Spacing" } <float value>{ <unit abbr> }, { <float value>{ <unit abbr> } }, ?EOL? ;

(* Every YO file has to have at least one taper definition with at least one value. If a single value is supplied it describes the diameter of the antenna elements, if more than one value is supplied is describes the tapering of the diameter sampled at N points starting at the boom. Each entry in a taper line defines a "taper section". Some form of whitespace is normally inserted before the first value so that values on the following length lines will line up with the taper values. Up to 20 taper values are allowed.

The optional "Spacing" keyword is described below. *)

<length line> = <float value>{ <unit abbr> }, <float value>{ <unit abbr> }, { <float value>{ <unit abbr> } }?EOL? ;

(* Length lines (normally) describe one-half of an element, measured out from the boom. The first length line in the file is always the reflector.

At least two values are required on every length line. The first value is the location of the element along the boom, normally the Y-axis in NEC terms. The first entry, for the reflector, is normally at position 0, but it can be moved if desired. The location entry is followed by one entry for each taper section defined in the preceding taper line, which describes the distance from the boom, normally the X-axis in NEC, where it reaches that taper dimension. This is why whitespace is often added to the front of taper lines, so that the taper definitions line up with the distances and skip over the location along the boom at the front of the line.

If one particular element does not taper, or tapers in a different way, taper diameters that are not used can be entered as zeros, or left empty if they are at the end of the line. This is common as many Yagis have a reflector that is slightly longer than the rest of the elements. In YO, this would be entered by having the reflector's taper fully defined out to the tip, and then simply leaving off the last entry on the rest of the elements. Less common uses might have a single boom with a fixed diameter, in which case only the last entry in the taper is used and preceding entries are zeros.

Element locations along the boom can be described in two ways, as absolute values in a given measurement unit, or as "spacings". The later mode is triggered by the addition of the optional "spacing" keyword to the proceeding taper line. When this mode is turned on, the Y locations specified in following length lines are offsets from the previous entry, not absolute values.

If the line is part of a taper definition with a single entry, that is, no taper, the user can optionally enter the length measurement as a tip-to-tip value rather than the normal out-from-the-boom values used otherwise. Yagi Optimizer assumes any measurement > 0.3 wavelengths is a whole-element-length, as Yagi antennas are normally about half a wavelength long, or a quarter wavelength per side, so anything larger than 0.25 is likely to be a whole-element-length.

YO files may have more than one taper line, and the geometry following each one must follow the last one's format.

As there is nothing in the taper or length lines that clearly define whether it is a taper or length line, Yagi Optimizer uses a formula to determine the type: if the sum of all the numbers on a given line are greater than .15 wavelengths, the line is a taper line, otherwise it is a length line. *)

  <freeform text> = ? any printable ASCII character ?, { ? any printable ASCII character ? }, ?EOL? ;

  <integer value> = [ "-" ], <digit>, { <digit> } ;

  <float value> = [ "-" ] { <digit> }, [ ".", { <digit> } ] [ "E" | "e" , [ "-" ] <digit>, { <digit> } ] ;

  <letter> = "A" .. "Z" | "a" .. "z" ;

  <digit> = "0" .. "9" ;

(* All keywords are case insensitive, "Copper", "copper" and "CoPPer" are equivalent. Whitespace between fields can consist of spaces or tabs.

Any text after the end of the deck is considered to be a free-form comment and does not require a comment marker. This is similar to NEC. *)

Examples
--------

Here is an example of a simple YO file from the original Yagi Optimizer documentation:

	K7HYR's Max Gain Yagi
	24.890  24.940  24.990  24.960  MHz
	4 elements, inches
	            1.617     1.250     1.125     0.875
	  0.000     2.938    15.062    66.000    33.305
	124.000     2.938    15.062    66.000    28.248
	248.000     2.938    15.062    66.000    26.815
	372.000     2.938    15.062    66.000    28.313

This file is optimizing a multi-frequency system with a transformer matching frequency of 24.960  MHz. There are four elements and the basic measurement unit is inches. There is a single taper line describing elements that taper from 1.617 inches near the boom to 0.875 at the tip. The three elements are spaced at 10' 4", 20' 8", and 31' from the reflector, and taper more strongly in the tip area.

Here is a version of the same design using spacing instead of absolute measurements:

	K7HYR's Max Gain Yagi
	24.890  24.940  24.990  24.960  MHz
	4 elements, inches
	spacing     1.617     1.250     1.125     0.875
	  0.000     2.938    15.062    66.000    33.305
	124.000     2.938    15.062    66.000    28.248
	124.000     2.938    15.062    66.000    26.815
	124.000     2.938    15.062    66.000    28.313
	
This modification describes an antenna where the reflector is slightly longer than the elements:

	K7HYR's Max Gain Yagi
	24.890  24.940  24.990  24.960  MHz
	4 elements, inches
	spacing     1.617     1.250     1.125     0.875     0.75
	  0.000     2.938    15.062    66.000    28.248    33.305
	124.000     2.938    15.062    66.000    28.248
	124.000     2.938    15.062    66.000    26.815
	124.000     2.938    15.062    66.000    28.313

And this one where the first element is not tapered but a constant 1.125 inches diameter:

	K7HYR's Max Gain Yagi
	24.890  24.940  24.990  24.960  MHz
	4 elements, inches
	spacing     1.617     1.250     1.125     0.875     0.75
	  0.000     2.938    15.062    66.000    28.248    33.305
	124.000     0    	0    	0	28.248
	124.000     2.938    15.062    66.000    26.815
	124.000     2.938    15.062    66.000    28.313

Note that using 0 or leaving an entry blank may result in the columns not lining up properly, as in this case. One may correct this by entering additional decimals on the zeros. Finally, we will describe the material, antenna height and pair of two antennas:

	Stacked yagis 50 feet apart
	Height 70'
	Copper
	Stacked 600
	24.890  24.940  24.990  24.960  MHz
	4 elements, inches
	spacing     1.617     1.250     1.125     0.875     0.75
	  0.000     2.938    15.062    66.000    28.248    33.305
	124.000     0.000     0.000     0.000	 28.248
	124.000     2.938    15.062    66.000    26.815
	124.000     2.938    15.062    66.000    28.313

The stacking is 50 feet, or 600 inches, the default measurement type for this file. The original documentation does not state whether a measurement type can be defined here, authors should assume it can.

Converting to NEC format
------------------------

A YO file can be easily converted to NEC format, although some information will be lost on translation. Some of this information can be retained in the OpenNEC format via the use of the extensions system.

The conversion of the initial data at the top of the file generally follows this pattern:

1. the first line is a comment, and can be copied directly to a GC line with a following GE, or a single GE with the comment on that line
2. a Height line, if present, sets the Z value for the following geometry lines
3. a material, if defined, can be converted to values on an LD card in NEC for calculation, and may be defined as an OpenNEC extension on a deck-wide basis to display proper coloration in a GUI display
4. a stacked antenna can be implemented in NEC with a GM card at the bottom of the geometry section of the NEC file with a NRPT of 1 and the separation value in the Z input. The ITS should be zero, and a value should be provided by the ITGI although it will not be required.
5. the frequency inputs can be put into one or more FR cards with RP/EX following
6. the "elements" number can be ignored, but the dimension units, if supplied, should be used in a GS card so that the original measurement numbers are retained in the GW elements

Conversion of the geometry may be simple or complex due to the modal nature of the values. Generally there are going to be several steps applied for every line:

1. determine if the line is a taper or length line based on the formula above
2. if it is a taper line, copy down the "spacing" flag and taper values so they can be applied to following lines
3. if it is a length line, use the other formula above to determine if it is whole-length or half-length
4. if it is a length line, and there are any unit abbreviations, and they are not the same as the base units, convert that number to the base units

NEC attempts to preserve symmetry as a way of improving performance - if the antenna is symmetrical across the X axis it only has to calculate values for one side of the antenna and then mirror those values for the other side and thereby perform far fewer complex calculations. Yagis are normally symmetrical, so this makes YO files a perfect fit for this system. The key is to ensure that NEC realizes there is symmetry by defining it using a GM card. As the YO format *normally* defines the antenna as lengths measured out from the boom, these will work perfectly with the GM system.

For the simple case with a single value in the taper, meaning simple cylindrical elements, any following length lines can be converted into a single GW card for each element. This can be accomplished by setting the YW1 to the second value on the line and YW2 to the negative of that number, or 1/2 of those values if the line is defined using whole-length. The ZW1 and ZW2 are set to the Height input, or zero if it's not present. Note that the Height is likely expressed in different units and will have to be converted. The YW1 and YW2 are set to the first value on the length line if the "spacing" flag is not present; if it is, the value has to be calculated as the sum of all measurements before it.

Things become more complex if the tapering includes more than one number, which is almost always the case in YO files. Although NEC includes the ability to define tapers using the GC card, these are defined in a calculated fashion, not the explicit values seen in YO. It is highly likely that using a GC with the starting and ending diameters from the taper line and the RDEL set to the number of entries in the taper will result in almost identical results, as most antennas taper smoothly and the original values in the YO file are likely measurements based on the actual gradual taper.

If you wish to use NEC's tapering system to emulate YO's tapers, start with a GW card with the element made in the same fashion as the non-tapered case but with the radius set to zero, then add a GC card following it with the starting and ending taper values. The RDEL input on the GC card is a bit more difficult; this can be set to 1 to produce a set of evenly spaced taper points, but can be adjusted to produce less taper as you travel away from the boom, with typical values being between 1/50 and 1/100.

It is also possible to exactly recreate the YO geometry in NEC by creating a GW for each entry in the taper sections. For instance, if the taper definition has three entries, this can be modelled using three GW's, one for each section of the YO taper and then mirroring that on the other side of the boom. Each one is modelled with a radius equal to that selection in the YO.

Note that any particular length line in a YO file may have fewer items than the taper line for that section, so it is possible that some of the entries can be reduced back to a single GW. In any case where a single GW is used, the number of segments should not be one, but a value chosen according to the NEC rules to create a reasonable calculation. For Yagis in amateur radio, a value between 5 and 7 is likely useful, and doubling the number of items in the associated taper section is likely to work well if it falls in those limits.

###Example conversion

Consider the following YO file:

	Stacked yagis 50 feet apart
	Height 70'
	Copper
	Stacked 600
	24.890  24.940  24.990  24.960  MHz
	4 elements, inches
	spacing     1.617     1.250     1.125     0.875     0.75
	  0.000     2.938    15.062    66.000    28.248    33.305
	124.000     0    	0    	28.248
	124.000     2.938    15.062    66.000    26.815
	124.000     2.938    15.062    66.000    28.313

There are a number of items to consider:

1. the Height is in feet, and has to be converted to inches
2. an LD card is needed to convert the material to copper
3. a second antenna is stacked 50 feet above the first
4. measurements are given in "spacing" format
5. tapering is complex

The resulting NEC file would be:

  CM Stacked yagis 50 feet apart
  CE
  !
  ! the Z value is 840, the Height of 70' converted to inches
  ! the Y value is 0, meaning "the reflector end of the antenna"
  ! there are five entries in the taper line, so the reflector
  ! has five GW's, one for each taper
  !
  GW 1 1 0.000 840 0 2.938 840 0 1.617
  GW 2 1 2.938 840 0 15.062 840 0 1.250
  GW 3 1 15.062 840 0 66.000 840 0 1.250
  GW 4 1 66.000 840 0 66.000 840 0 0.875
  GW 5 1 28.248 840 0 33.305 840 0 0.75
  !
  ! the next element has only a single GW because the
  ! other taper values are zero. note the segment count
  ! matching the number above as parallel conductors should
  ! (generally) have the same count in NEC. also note the Z
  ! measure as an offset because this is located along the boom
  !
  GW 6 5 -28.248 840 124.000 28.248 840 124.000 0.875
  !
  ! and remaining two passive elements are one segment shorter than 
  ! the reflector so they have four entries each. They are located 
  ! using "spacing" so we have to calculate the Z values
  !
  GW 7 1 0 840 248 2.938 840 248 1.617
  GW 8 1 2.938 840 248 15.062 840 248 1.250
  GW 9 1 15.062 840 124 66.000 840 124 1.250
  GW 10 1 66.000 840 124 28.313 840 124 0.875
  !
  GW 11 1 0 840 248 2.938 840 248 1.617
  GW 12 1 2.938 840 248 15.062 840 248 1.250
  GW 13 1 15.062 840 124 66.000 840 124 1.250
  GW 14 1 66.000 840 124 28.313 840 124 0.875
  !
  ! now we use a GM card to reflect that through the X axis
  ! by rotating it around the Z axis
  !
  GM 20 1 0 0 180 0 0 0
  !
  ! now another GM to create the second antenna above the
  ! first by translating it on the Y axis
  !
  GM 30 1 0 0 0 0 600 0
  !
  ! and then a GS to convert everything to inches
  !
  GS 0 0 39.3701
  !
  ! and we're done with the geometry
  GE

