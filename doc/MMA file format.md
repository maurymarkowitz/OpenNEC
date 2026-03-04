MMANA‑GAL ".maa" file format
============================

Introduction
------------

The MMANA‑GAL software (and its derivatives) use a text format for saving antenna models. The files normally use the `.maa` extension, or more rarely `.mma`. These files contain an ordered sequence of text lines, separated by header lines and line counts, somewhat similar to the YO format.

Although the format is undocumented, a large number of examples on‑line allow the structure to be inferred. The variation seen in the wild ranges from the minimal 4‑line variant used by simple exporters (frequency, counts, wires, loads, sources) up through more elaborate files which include segmentation parameters, ground and measurement options, and comment blocks. After reading this description you should be able to glance at an existing `.maa` file and understand which parts will be carried over when using OpenNEC's conversion functions.

This document summarises the features that OpenNEC’s import/export code understands and points out the sections that are currently ignored.

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

   The importer computes a concrete segment count for each auto-seg wire using the `***Segmentation***` parameters and the design frequency. See the *Auto-segmentation* section below.

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

   The importer resolves the designator to a NEC segment index and emits an `EX 0` card with the magnitude and phase converted to real/imaginary components. All designators — including those pointing at wires with MMANA auto-segmentation markers — are resolved to concrete 1-based segment numbers using the computed wire segment counts:

   | Designator | Resolved segment |
   |---|---|
   | `W3C`   | `(N+1)/2` where N = computed segs of wire 3 |
   | `W3C1`  | `(N+1)/2 + 1` |
   | `W3C-1` | `(N+1)/2 − 1` |
   | `W6E`   | N (last segment) |
   | `W6E-2` | N − 2 |
   | `W5B`   | 1 (first segment) |

   Results are clamped to `[1, N]` to stay within wire bounds.
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

   The `***Segmentation***` section contains a single line with four comma‑separated values that control how MMANA‑GAL automatically divides wires into segments when the user clicks *Auto‑segment*:

   | Field | Values seen | Meaning |
   |---|---|---|
   | 1 | 200 – 1500 | Maximum total segment count |
   | 2 | 40 – 200   | Target segments per wavelength |
   | 3 | 1.01 – 2.0 | Length taper ratio between adjacent segments |
   | 4 | 2 – 16     | Minimum segments per wire |

   Example: `800, 80, 2.0, 2` — 800 max total, 80 seg/λ, 2.0× taper, min 2/wire. Among the 935 files there are only 16 distinct combinations of these values. On import the values are used to compute concrete segment counts and are also preserved as a `! maa-segmentation:` annotation card so they can be written back correctly on export:

   `! maa-segmentation: dm1=800 dm2=80 sc=2 ec=2 mode=-1`

   The `mode=` field is appended when all wires share the same MMANA auto-segmentation type (common case). When wires differ, each `GW` card receives a per-wire `!segmentation:N` suffix instead.

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

Auto-segmentation
-----------------

A key feature of the MMANA program is auto-segmentation, or as they refer to it, "tapering" - a poor choice of terms given "tapering" means something entirely different in NEC. The concept is documented in the Segmentation section of the MMANA-GAL documentation (currently) found here:

http://gal-ana.de/basicmm/en/

In NEC, geometry like a `GW` card includes a value for the number of segments it should be divided into. For instance, one might want a 10 metre long element to be represented internally as ten 1 meter parts. This "even segmentation" approach has several problems. For one, you generally want to use more segments where there are curves, as curvature can strongly influence the results. You can simply choose large numbers of segments, and some files do this, but this leads to longer calculation times.

MMANA-GA adds a system that calculates reasonable segment counts based on the size and shape of the element. It does this based on the wavelength of the test signal, which in NEC is found on the `FR` card. It also adjusts the segment sizes by their position, adding more segments at the ends of the elements and fewer in the center. This minimizes the number of small segments, and improves calculation time. There are settings to control whether to use manual segments (the NEC solution), use smaller segments at both ends, or at one end or the other. The manual strongly suggests using the both-ends method, -1, for most designs.

During import, OpenNEC uses the `***Segmentation***` parameters together with the design frequency to compute a concrete segment count for every wire that carries an auto-segmentation marker. The mode is also recorded in the deck. If all wires share the same auto-segmentation type (the common case — nearly all real-world files use `-1` uniformly), a single `! maa-segmentation:` annotation card is appended to the deck with the parameters and a `mode=` field, for example:

   `! maa-segmentation: dm1=800 dm2=80 sc=2 ec=2 mode=-1`

If the wires use different modes, the common-mode field is omitted from the global annotation and each `GW` card instead receives a per-wire `!segmentation:N` suffix.

This approach fixes the segment counts so that all other NEC cards (EX, LD, TL, etc.) that reference segment numbers are correct. The annotation also allows the exporter to reconstruct the original MMANA `.maa` file with the correct (negative) mode values in the SEG column and a `***Segmentation***` block containing the original parameters — completing the round-trip.


Features not supported by OpenNEC
---------------------------------

* The `***G/H/M/R/AzEl/X***` ground section is imported and a `GN` card is emitted (see *Format overview* section 7).  The radials count from field 3 is noted in a `!` comment but a `GD` card is not emitted because the radial wire radius and length are not stored in the file.  All other extra sections (measurement settings, stacking information, etc.) are still ignored.
* The full source designator syntax (`W`/`V` prefix, `C`/`B`/`E` attachment point, signed offset) is recognised by the importer. All designators are resolved to concrete 1-based segment numbers using the computed wire segment counts; the `V` source type is treated identically to `W` (both map to `EX 0`).
* The exporter produces Variant B: a `***Wires***` header with the wire count on its own line, followed by the wire data, then `***Source***` (before `***Load***`) with sources in `w<N>c, phase°, mag` form. It restores the original MMANA SEG mode values (the negative auto-segmentation markers) in the wire SEG column and emits a `***Segmentation***` block when the deck contains a `! maa-segmentation:` annotation. Ground and measurement headers (`***G/H/M/R/AzEl/X***`) are not emitted.
* Parallel loading networks, voltage sources with special types, and certain non‑linear loads present in some `.maa` examples are not mapped.

Because of these omissions, running `read_deck_maa` followed by `write_deck_maa` on a real‑world `.maa` file will produce a file that preserves the geometry (with original SEG mode markers), sources, loads, frequency, segmentation parameters, and comments, but omits the ground type entry. The round-trip is sufficient for re-opening the file in MMANA-GAL and re-running auto-segmentation.

Examples
--------

### Importer

The following shows `Broadband 80m.5.maa` converted to OpenNEC deck format by `read_deck_maa`. The title becomes the `CM` card, each wire becomes a `GW` card with a computed concrete segment count, the source designator `w7c` resolves to concrete segment 1 (wire 7 has 1 segment at this frequency), the `***Segmentation***` parameters are preserved as a `! maa-segmentation:` annotation, and the `###Comment###` block likewise becomes a `!` line. This file specifies free-space (`gtype=0`), so no `GN` card is emitted:

```
CM Broadband antenna 80m 3.5 - 3.8MHz (SWR<1,2)
CE
GW 1,28,0,-21.1,-0,0,0,0,0.001
GW 2,28,0.5,0,0,0.5,21.1,0,0.001
GW 3,24,0.5,-17.55,-0,0.5,0,0,0.001
GW 4,11,0,0,0,0.2,0,-1.03,0.001
GW 5,24,0,0,0,0,17.55,0,0.001
GW 6,11,0.3,0,-1.03,0.5,0,0,0.001
GW 7,1,0.2,0,-1.03,0.3,0,-1.03,0.001
! maa-segmentation: dm1=800 dm2=80 sc=2 ec=2 mode=-1
GE
EX 0,7,1,1.000000,0,0,0
FR 0,0,3.650000,0,0,0
RP 0,37,73,1000,0,0,5,5
EN
! Mod by UR0GT, 02.04.2008 0:06:04
```

All seven wires carry mode `-1` (taper both ends), so a single `mode=-1` is appended to the global annotation rather than annotating each `GW` card individually. Segment counts are computed from the `dm1=800 dm2=80 sc=2 ec=2` parameters at 3.65 MHz: the six longer wires receive 11–28 segments each; wire 7 is only 0.1 m (≈ 0.0012 λ) and receives 1 segment (below the minimum effective length for meaningful subdivision at this frequency). The source `w7c` (centre of wire 7) therefore resolves to segment 1.

### Exporter

The exporter produces Variant B output. Re-exporting the `Broadband 80m.5.maa` deck shown above back to `.maa` format gives:

```
Broadband antenna 80m 3.5 - 3.8MHz (SWR<1,2)
*
3.650000
***Wires***
7
0.000000, -21.100000, -0.000000, 0.000000, 0.000000, 0.000000, 0.001000, -1
0.500000, 0.000000, 0.000000, 0.500000, 21.100000, 0.000000, 0.001000, -1
0.500000, -17.550000, -0.000000, 0.500000, 0.000000, 0.000000, 0.001000, -1
0.000000, 0.000000, 0.000000, 0.200000, 0.000000, -1.030000, 0.001000, -1
0.000000, 0.000000, 0.000000, 0.000000, 17.550000, 0.000000, 0.001000, -1
0.300000, 0.000000, -1.030000, 0.500000, 0.000000, 0.000000, 0.001000, -1
0.200000, 0.000000, -1.030000, 0.300000, 0.000000, -1.030000, 0.001000, -1
***Source***
1, 0
w7c, 0.00, 1.000000
***Load***
0, 0
***Segmentation***
800, 80, 2, 2
###Comment###
Mod by UR0GT, 02.04.2008 0:06:04
```

The SEG column is restored to `-1` (the original MMANA mode marker) for all seven wires rather than the computed counts. The `***Segmentation***` block is written from the `! maa-segmentation:` annotation. The `sc=2` (taper ratio) value is written as `2` rather than `2.0` because trailing `.0` is suppressed by the `%.4g` format descriptor. The original Cyrillic characters in the comment are reproduced faithfully if the terminal encoding matches; they may appear garbled in ASCII-only environments.

The `*` separator between the title and frequency is always emitted. If no `CM` card is present the title line is left blank (the `*` is still written). If the deck has no `! maa-segmentation:` annotation (e.g. it was not imported from a `.maa` file), the `***Segmentation***` block is omitted and wire SEG columns contain the computed positive segment counts.
