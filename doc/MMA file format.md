MMANA‑GAL ".maa" file format
============================

Introduction
------------

The MMANA‑GAL software (and its derivatives) uses an ASCII format for saving antenna models. The files conventionally carry the `.maa` extension, or more rarely `.mma`. These files contain an ordered sequence of text lines. Although the format is undocumented, a large number of examples on‑line allow the structure to be inferred. This document summarises the features that OpenNEC’s import/export code understands and points out the sections that are currently ignored.

The variation seen in the wild ranges from the minimal 4‑line variant used by simple exporters (frequency, counts, wires, loads, sources) up through more elaborate files which include segmentation parameters, ground and measurement options, and comment blocks. After reading this description you should be able to glance at an existing `.maa` file and understand which parts will be carried over when using OpenNEC's conversion functions.

Format overview
---------------

A typical `.maa` file is organised into the following logical sections. Two distinct structural variants exist in the wild (see *Format variants* below). They differ in their use of section headers, which are marked in the file with `***…***`. 

1. **Title line.**  Arbitrary text used as a description of the model. The title is optional in Variant B — 220 of 935 files omit it and begin with a bare `*` separator instead. When present and the file is imported the converter creates a `CM` card containing this line (followed immediately by a `CE` card). During export the first comment card in the deck is written back as the title line.
2. **Frequency line.**  A single floating‑point value giving the design frequency in megahertz. Some files (Variant B) include a bare `*` on a separate line between the title and the frequency; this is ignored.
3. **Counts.**  In Variant A a single line holds three integers: wire count, load count, source count. In Variant B each count appears on its own line immediately inside the relevant `***…***` section.
4. **Wire (geometry) block.**  Exactly `N_wires` following lines, each containing eight numeric values. The fields represent the end‑point coordinates of a straight wire in metres, the radius, and the segment count. Example:
   ````
   0.0, -21.1, -3.662e-07,  0.0, 0.0, 0.0, 0.001, -1
   ````

   **Radius units.**  The radius field is ambiguous: values appear to be in millimetres or in wavelengths unless there is a per‑file annotation that overrides this. The mechanism for indicating the unit is not yet fully understood; OpenNEC's importer currently treats the field value as metres (the NEC native unit) without conversion.  Users should verify or scale values manually if the source file uses mm.

   **Segment count special values.**  A positive integer gives the exact NEC segment count for that wire.  The following negative and zero values are MMANA auto‑segmentation directives:

   | Value | MMANA meaning |
   |---|---|
   | `0`  | Automatic uniform segmentation |
   | `‑1` | Automatic tapering segmentation (denser near junctions) |
   | `‑2` | Tapering applied only at the start end of the wire |
   | `‑3` | Tapering applied only at the finish end of the wire |

   OpenNEC does not implement these auto‑segmentation modes (see *Features not supported* below).

5. **Source (EX) block.**  A line giving the count, followed by that many source definitions.  Each definition uses a three‑field format:

   ````
   <source-designator>, <phase°>, <magnitude>
   ````

   The source designator encodes the source type, wire number, attachment point and optional offset in a compact alphanumeric token.  The leading letter selects the source type:

   | Prefix | Source type |
   |---|---|
   | `W` | Wire |
   | `V` | Voltage source |

   After the type letter comes the wire number as a decimal integer, which is the ordinal position of the wire in the Wires list. After that is a single letter that identifies the attachment point on the wire:

   | Letter | Position |
   |---|---|
   | `C` | Centre segment |
   | `B` | Beginning (first) segment |
   | `E` | End (last) segment |

   An optional signed integer offset may follow the attachment letter immediately (no separator).  Positive values step towards the end; negative values step towards the beginning:

   | Designator | Meaning |
   |---|---|
   | `W1C`   | Centre segment of wire 1 |
   | `W3C1`  | One segment past the centre of wire 3 |
   | `W4C-1` | One segment before the centre of wire 4 |
   | `W5B`   | First segment of wire 5 |
   | `W6E3`  | Third segment from the end of wire 6 |

   The importer resolves the designator to a NEC segment index and emits an `EX 0` card with the magnitude and phase converted to real/imaginary components. When the wire uses an MMANA auto-segmentation marker (`0`, `‑1`, `‑2`, or `‑3`) the true segment count is not yet known, so instead of a concrete integer the segment field is written as a tinyexpr expression that references the `segs` symbol from the `SY` card inserted for that purpose:

   | Designator | Emitted EX segment field |
   |---|---|
   | `W3C`   | `(segs+1)/2`    |
   | `W3C1`  | `(segs+1)/2+1`  |
   | `W3C-1` | `(segs+1)/2-1`  |
   | `W6E`   | `segs`          |
   | `W6E-2` | `segs-2`        |
   | `W5B`   | `1` (concrete)  |

   Once the user edits the `SY segs=…` line to a concrete positive value, OpenNEC's expression evaluator resolves these automatically.
6. **Load (LD) block.**  A count line then load definitions. Each load is expressed as wire number, segment number, R, X, L, C. The code maps series RLC or fixed impedance constructs; parallel loads are ignored.
7. **Optional extra sections.**  Many real‑world `.maa` files then contain further labelled blocks such as:
   `***Segmentation***` (MMANA auto‑segmentation tuning),
   `***G/H/M/R/AzEl/X***` (ground type/parameters and measurement options),
   and final `###Comment###` lines with free‑form text. The marker is
   sometimes followed by the comment on the same line, but more commonly the
   text appears on the next line; our importer handles both forms and keeps
   the comment at its original position relative to the geometry and load
   blocks. During import each comment is turned into a CM/CE card, and when
   exporting any comment cards present in the deck are emitted back as
   `###Comment### …` lines.

   The `***Segmentation***` section is imported as described below.

   The `***G/H/M/R/AzEl/X***` section contains a single comma‑separated line with seven fields:

   | Field | Meaning |
   |---|---|
   | 1 | Ground type: `0`=free‑space, `1`=perfect, `2`=real/MININEC, `‑1`=Sommerfeld‑Norton |
   | 2 | Conductivity in mS/m (e.g. `5.0` = 5 mS/m = 0.005 S/m); `0.0` = unspecified |
   | 3 | Radials count (informational; a full `GD` card requires radius and length not present in the file) |
   | 4 | Reference impedance for SWR/reflection display (50 or 112 Ω); ignored by importer |
   | 5–7 | Pattern display angles and height offset; ignored by importer |

   The importer emits a `GN` card (placed before the `FR` card so the deck is in the correct NEC order):
   - Type `0` → no `GN` card (free‑space simulation)
   - Type `1` → `GN 1` (perfect ground)
   - Type `2` → `GN 0` (MININEC real ground)
   - Type `‑1` → `GN 2` (Sommerfeld‑Norton real ground)

   Because the file stores only conductivity, the relative dielectric constant (`epsr`) is derived from standard NEC soil‑type correlations:

   | σ (mS/m) | epsr used | Soil description |
   |---|---|---|
   | 0 (unspecified) | 13 | Average (NEC default) |
   | ≤ 1  | 5  | Poor/rocky/dry sand |
   | 1–8  | 13 | Average soil |
   | 8–30 | 17 | Good/agricultural |
   | > 30 | 25 | Very good/wet |

   If the radials count (field 3) is non‑zero a `!` comment records the count, e.g.:
   `! maa-ground-radials: 4 (GD card not emitted: radius/length unknown)`
   Users who need a ground‑radial screen should add a `GD` card manually.

   The `***Segmentation***` section contains a single line with four
   comma‑separated values that control how MMANA‑GAL automatically divides
   wires into segments when the user clicks *Auto‑segment*:

   | Field | Values seen | Meaning |
   |---|---|---|
   | 1 | 200 – 1500 | Maximum total segment count |
   | 2 | 40 – 200   | Target segments per wavelength |
   | 3 | 1.01 – 2.0 | Length taper ratio between adjacent segments |
   | 4 | 2 – 16     | Minimum segments per wire |

   Example: `800, 80, 2.0, 2` — 800 max total, 80 seg/λ, 2.0× taper, min 2/wire. Among the 935 files there are only 16 distinct combinations of these values. OpenNEC does not perform auto‑segmentation (each wire's segment count is fixed on the wire line itself), so on import the four values are captured and written as a single `!` comment card for reference, e.g.:

   `! maa-segmentation: max-segs=800 segs-per-wl=80 taper=2 min-segs=4`

Whitespace is permissive: commas or any combination of spaces and tabs may separate the numeric fields. The lines may also contain leading/trailing spaces. The format is case‑insensitive.

Format variants
---------------

The 935 real‑world `.maa` files revealed two structural variants (see [MMA format survey.md](MMA%20format%20survey.md) for the full per‑file table).

**Variant A — 729 files (78 %)** — combined counts line, `***Wires***` optional

    Broadband antenna 80m 3.5 - 3.8MHz
    3.650000
    7 1 1              ← nw nl ns together
    0.0, -21.1, ..., 0.001, -1
    ...
    ***Source***
    1, 1
    w7c, 0.0, 1.0
    ***Load***
    0, 1
    ...

**Variant B — 205 files (22 %)** — per‑section headers, count inside each section

The title line is **optional** in Variant B — 220 of 935 files start immediately with a bare `*` separator and have no title. With title:

    144CQlomba
    *                  ← optional bare asterisk line
    144.28

Without title:

    *
    144.28
    ***Wires***
    16                 ← wire count alone
    0.37853, -0.26672, ..., 8.000e-04, -1
    ...
    ***Source***
    1, 0
    w1c, 0.0, 1.0
    ***Load***
    0, 0
    ...

Minimal grammar
---------------

(BNF for reference; both variants shown.)

```
<MMA-A>  ::= <title> NEWLINE
             <frequency> NEWLINE
             <nw> SEP <nl> SEP <ns> NEWLINE    // all three counts
             <wire>^nw
             ["***Source***" NEWLINE <ns> SEP <0> NEWLINE <src-a>^ns]
             ["***Load***"   NEWLINE <nl> SEP <0> NEWLINE <load>^nl]
             <extra>*

<MMA-B>  ::= [<title> NEWLINE]              // title is OPTIONAL in Variant B
             ["*" NEWLINE]                     // optional separator
             <frequency> NEWLINE
             "***Wires***" NEWLINE
             <nw> NEWLINE                      // wire count alone
             <wire>^nw
             ["***Source***" NEWLINE <ns> SEP <0> NEWLINE <src-b>^ns]
             ["***Load***"   NEWLINE <nl> SEP <0> NEWLINE <load>^nl]
             <extra>*

<wire>   ::= <float> SEP <float> SEP <float> SEP   // x1 y1 z1
             <float> SEP <float> SEP <float> SEP   // x2 y2 z2
             <float> SEP <seg-count>               // radius (mm or λ, see notes), segments

<seg-count> ::= <positive-int>  // explicit NEC segment count
              | "0"             // auto uniform segmentation
              | "-1"            // auto tapering segmentation
              | "-2"            // taper at start end only
              | "-3"            // taper at finish end only

<src-a>  ::= <int> SEP <int> SEP <float> SEP <float>   // wire, seg, mag, phase°
<src-b>  ::= <src-designator> SEP <float> SEP <float>  // designator, phase°, mag

<src-designator> ::= <src-type> <int> <attach-point> [ <signed-int> ]
<src-type>       ::= "W" | "V"     // voltage source (both forms equivalent)
<attach-point>   ::= "C"           // centre segment
                   | "B"           // beginning (first) segment
                   | "E"           // end (last) segment
<signed-int>     ::= ["-"] <digit> { <digit> }  // offset from attach-point

<load>   ::= <int> SEP <int> SEP <float> SEP <float> SEP <float> SEP <float>
                                                       // wire, seg, R, X, L, C

<extra>  ::= "***Segmentation***" NEWLINE <seg-params>   // MMANA auto-seg settings
           | "***G/H/M/R/AzEl/X***" NEWLINE <ground-params>
           | "###Comment###" [SP <text> | NEWLINE <text>]
           | other line  // ignored

<seg-params> ::= <max-segs> SEP <segs-per-wl> SEP <taper-ratio> SEP <min-segs> NEWLINE
                 // max-segs:    200–1500  total segment limit
                 // segs-per-wl: 40–200   segments per wavelength target
                 // taper-ratio: 1.01–2.0 adjacent-segment length ratio
                 // min-segs:    2–16     minimum segments per individual wire

SEP      ::= "," | whitespace+
```

The `***…***` headers and `###Comment###` markers may appear anywhere between sections and are interpreted or skipped as described above.

Features not supported by OpenNEC
---------------------------------

* The `***Segmentation***` section is imported as a `!` comment line for reference (e.g. `! maa-segmentation: max-segs=800 segs-per-wl=80 taper=2 min-segs=4`) but the values are not used to drive re‑segmentation.
* The `***G/H/M/R/AzEl/X***` ground section is imported and a `GN` card is emitted (see *Format overview* section 7).  The radials count from field 3 is noted in a `!` comment but a `GD` card is not emitted because the radial wire radius and length are not stored in the file.  All other extra sections (measurement settings, stacking information, etc.) are still ignored.
* When importing files where any wire uses one of the MMANA auto‑segmentation markers (`0`, `‑1`, `‑2`, or `‑3`) as the segment count, the importer inserts a small helper card immediately after the `CE` card with the form:

   `SY segs=10 !default segment count, change to realistic value`

   and replaces the textual placeholder token in the `GW` card strings with the literal token `segs`.  All four marker values are treated the same way at import time — the specific tapering mode is not preserved.  This makes the auto‑segmentation intent explicit in the NEC deck so a user can edit the `SY` line (change `segs=10` to a sensible positive value) and then replace `segs` with that number before running a simulation.  Note: this change is textual (it updates `card->card_str`/`orig_str`) and does not automatically convert parsed numeric fields elsewhere in OpenNEC.
* The full source designator syntax (`W`/`V` prefix, `C`/`B`/`E` attachment point, signed offset) is recognised by the importer.  For wires with a concrete segment count the attachment and offset are resolved to an absolute 1-based segment number (clamped to the wire bounds).  For wires that use any MMANA auto-segmentation marker (`0`, `‑1`, `‑2`, `‑3`) the segment count is not yet known, so `C` and `E` designators emit a tinyexpr expression (`(segs+1)/2`, `segs`, etc.) in the `EX` card's segment field, referencing the `segs` SY symbol; `B` (beginning) designators always resolve to a concrete integer since they have no segment-count dependency.  The `V` source type is treated identically to `W` (both map to `EX 0`).
* The exporter produces Variant B: a `***Wires***` header with the wire count on its own line, followed by the wire data, then `***Source***` (before `***Load***`) with sources in `w<N>c, phase°, mag` form.  It does not emit `***Segmentation***` or ground/measurement headers.
* Filenames in the collection sometimes contain other units (feet, inches) or spatial rotations. Only coordinates in metres are handled by the importer; unit conversions are **not** performed.
* Parallel loading networks, voltage sources with special types, and certain non‑linear loads present in some `.maa` examples are not mapped.

Because of these omissions, running `read_deck_maa` followed by `write_deck_maa` on a real‑world `.maa` file will generally produce a much smaller file containing only the structural geometry, loads, and a single frequency. Use of the converter is appropriate when you merely need the wire geometry and excitations, but it should not be relied on for preserving MMANA‑specific tuning, ground options, or comments.

Examples
--------

### Importer

The following shows `Broadband 80m.5.maa` converted to NEC‑2 format by `read_deck_maa`. The title becomes the `CM` card, each wire line becomes a `GW` card, the source designator `w7c` resolves to wire 7 centre with a tinyexpr expression for the segment (because all wires use auto‑segmentation), the `***Segmentation***` parameters are preserved as a `!` comment, and the `###Comment###` block likewise becomes a `!` line.  This particular file specifies `gtype=0` (free‑space), so no `GN` card is emitted:

```
CM Broadband antenna 80m 3.5 - 3.8MHz (SWR<1,2)
CE
SY segs=10 !default segment count, change to realistic value
GW 1, segs, 0.000000, -21.100000, -0.000000, 0.000000, 0.000000, 0.000000, 0.001000
GW 2, segs, 0.500000, 0.000000, 0.000000, 0.500000, 21.100000, 0.000000, 0.001000
GW 3, segs, 0.500000, -17.550000, -0.000000, 0.500000, 0.000000, 0.000000, 0.001000
GW 4, segs, 0.000000, 0.000000, 0.000000, 0.200000, 0.000000, -1.030000, 0.001000
GW 5, segs, 0.000000, 0.000000, 0.000000, 0.000000, 17.550000, 0.000000, 0.001000
GW 6, segs, 0.300000, 0.000000, -1.030000, 0.500000, 0.000000, 0.000000, 0.001000
GW 7, segs, 0.200000, 0.000000, -1.030000, 0.300000, 0.000000, -1.030000, 0.001000
GE
EX 0, 7, (segs+1)/2, 1.000000, 0.000000, 0,0,0
! maa-segmentation: max-segs=800 segs-per-wl=80 taper=2 min-segs=2
FR 0,0,3.650000,0,0,0,0,0
RP 0, 37, 73, 1000, 0, 0, 5, 5
EN
! Mod by UR0GT, 02.04.2008 0:06:04
```

The importer replaces MMANA's `-1` placeholder with the token `segs` in the textual GW lines and inserts the `SY` helper so the user can set a concrete segment count. The default value inserted is `segs=10`; change this value to a realistic per‑wire segment count and then replace `segs` with that integer before running a simulation. The textual `segs` token is not automatically propagated into numeric fields used by other parts of OpenNEC.

### Exporter

A minimal file produced by the current exporter looks like this (using a deck whose first `CM` card reads `Half-wave dipole at 14 MHz`):
```
Half-wave dipole at 14 MHz
14.000000
***Wires***
1
0.000000, 0.000000, 0.000000, 1.000000, 0.000000, 0.000000, 0.001000, 5
***Source***
0, 0
***Load***
0, 0
```
If no `CM` card is present the title line is left blank.  The exporter produces Variant B (`***Wires***` header, source lines in `w<N>c, phase°, mag` form) with `***Source***` before `***Load***`, matching the structure of the most common real‑world files.  The example corresponds to a single wire from (0,0,0) to (1,0,0) with radius field 0.001 (interpret according to the radius‑unit note above) and five segments.

The earlier sample (`Broadband antenna 80m 3.5 - 3.8MHz …`) shown above is an example of a richer file.  `***Segmentation***` and `***G/H/M/R/AzEl/X***` are handled as described; other extra headers (measurement settings, stacking, etc.) are still ignored.
