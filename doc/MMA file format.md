MMANA‑GAL ".maa" file format
=================================

Introduction
------------

The MMANA‑GAL software (and its derivatives) uses an ASCII format for saving antenna models. The files conventionally carry the `.maa` extension, or more rarely `.mma`. These contain an ordered sequence of text lines. Although the format is undocumented, a large number of examples on‑line allow the structure to be inferred. This document summarises the features that OpenNEC’s import/export code understands and points out the sections that are currently ignored.

The variation seen in the wild ranges from the minimal 4‑line variant used by simple exporters (frequency, counts, wires, loads, sources) up through more elaborate files which include segmentation parameters, ground and measurement options, and comment blocks. After reading this description you should be able to glance at an existing `.maa` file and understand which parts will be carried over when using OpenNEC's conversion functions.

Format overview
---------------

A typical `.maa` file is organised into the following logical sections. Two distinct structural variants exist in the wild (see *Format variants* below). They differ in their use of section headers, which are marked in the file with `***…***`. A survey of 935 real‑world files found only one without them, and that turned out to be a NEC deck saved with the wrong extension.

1. **Title line.**  Arbitrary text used as a description of the model. **Optional in Variant B** — 220 of 935 surveyed files omit it entirely and begin with a bare `*` separator instead. When present and the file is imported the converter creates a `CM` card containing this line (followed immediately by a `CE` card). During export the first comment card in the deck is written back as the title line.
2. **Frequency line.**  A single floating‑point value giving the design frequency in megahertz. Some files (Variant B) include a bare `*` on a separate line between the title and the frequency; this is ignored.
3. **Counts.**  In Variant A a single line holds three integers: wire count, load count, source count. In Variant B each count appears on its own line immediately inside the relevant `***…***` section.
4. **Wire (geometry) block.**  Exactly `N_wires` following lines, each containing eight numeric values and usually a trailing “‑1” placeholder. The fields represent the end‑point coordinates of a straight wire in metres, the radius in metres, and the number of segments. Example:
   ````
   0.0, -21.1, -3.662e-07,  0.0, 0.0, 0.0, 0.001, -1
   ````
5. **Source (EX) block.**  A line giving the count (usually same as number of excitations), followed by that many lines with four values: wire number, segment number, voltage magnitude and phase (degrees). The importer converts magnitude/phase into real/imaginary parts.
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
   `###Comment### …` lines. The remaining extra sections are still **ignored**.

   The `***Segmentation***` section contains a single line with four
   comma‑separated values that control how MMANA‑GAL automatically divides
   wires into segments when the user clicks *Auto‑segment*:

   | Field | Values seen | Meaning |
   |---|---|---|
   | 1 | 200 – 1500 | Maximum total segment count |
   | 2 | 40 – 200   | Target segments per wavelength |
   | 3 | 1.01 – 2.0 | Length taper ratio between adjacent segments |
   | 4 | 2 – 16     | Minimum segments per wire |

   Example: `800, 80, 2.0, 2` — 800 max total, 80 seg/λ, 2.0× taper, min 2/wire. A survey of 935 files found only 16 distinct combinations of these values. OpenNEC does not perform auto‑segmentation (each wire's segment count is fixed on the wire line itself), so on import the four values are captured and written as a single `!` comment card for reference, e.g.:
   
   `! maa-segmentation: max-segs=800 segs-per-wl=80 taper=2 min-segs=4`

Whitespace is permissive: commas or any combination of spaces and tabs may separate the numeric fields. The lines may also contain leading/trailing spaces. The format is case‑insensitive.

Format variants
---------------

A survey of 935 real‑world `.maa` files revealed two structural variants (see [MMA format survey.md](MMA%20format%20survey.md) for the full per‑file table).

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
             <float> SEP <int>                     // radius, segments

<src-a>  ::= <int> SEP <int> SEP <float> SEP <float>   // wire, seg, mag, phase°
<src-b>  ::= "w" <int> "c" SEP <float> SEP <float>    // wire-label, phase°, mag

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

* The `***Segmentation***` section is imported as a `!` comment line for reference (e.g. `! maa-segmentation: max-segs=800 segs-per-wl=80 taper=2 min-segs=4`) but the values are not used to drive re‑segmentation.  All other extra sections (ground definitions, measurement settings, stacking information, etc.) are still ignored.
* When importing files where any wire uses `-1` as the segment placeholder (MMANA's "auto‑segment" marker), the importer inserts a small helper card immediately after the `CE` card with the form:

   `SY segs=10 'default segment count, change to realistic value'`

   and replaces the textual `-1` token in the `GW` card strings with the literal token `segs`. This makes the auto‑segmentation intent explicit in the NEC deck so a user can edit the `SY` line (change `segs=10` to a sensible positive value) and then replace `segs` with that number before running a simulation. Note: this change is textual (it updates `card->card_str`/`orig_str`) and does not automatically convert parsed numeric fields elsewhere in OpenNEC.
* The exporter produces Variant A: a combined counts line followed by bare
  wire data, then `***Source***` and `***Load***` headers with their blocks.
  It does not emit `***Wires***`, `***Segmentation***`, or ground/measurement
  headers.
* Filenames in the collection sometimes contain other units (feet, inches) or spatial rotations. Only coordinates in metres are handled by the importer; unit conversions are **not** performed.
* Parallel loading networks, voltage sources with special types, and certain non‑linear loads present in some `.maa` examples are not mapped.

Because of these omissions, running `read_deck_maa` followed by `write_deck_maa` on a real‑world `.maa` file will generally produce a much smaller file containing only the structural geometry, loads, and a single frequency. Use of the converter is appropriate when you merely need the wire geometry and excitations, but it should not be relied on for preserving MMANA‑specific tuning, ground options, or comments.

Examples
--------

A minimal file produced by the current exporter looks like this (using a deck whose first `CM` card reads `Half-wave dipole at 14 MHz`):
```
Half-wave dipole at 14 MHz
14.000000
1 0 0
0.000000, 0.000000, 0.000000, 1.000000, 0.000000, 0.000000, 0.001000, 5
***Source***
0, 0
***Load***
0, 0
```
If no `CM` card is present the title line is left blank.  The exporter produces Variant A (combined counts line, no `***Wires***` header) and includes `***Source***` and `***Load***` section headers, matching the structure of the 934 real‑world files that carry them.  Which corresponds to a single wire from (0,0,0) to (1,0,0) with radius 1 mm and five segments.

The earlier sample (`Broadband antenna 80m 3.5 - 3.8MHz …`) shown above is an example of a richer file; the additional headers and fields would be ignored when imported by OpenNEC.

The following shows `Broadband 80m.5.maa` converted to NEC‑2 format by `read_deck_maa`. The title becomes the `CM` card, each wire line becomes a `GW` card (segment count `−1` is the MMANA placeholder — see below), the `***Segmentation***` parameters are preserved as a `!` comment, and the
`###Comment###` block likewise becomes a `!` line:

```
CM Broadband antenna 80m 3.5 - 3.8MHz (SWR<1,2)
CE
SY segs=10 'default segment count, change to realistic value'
GW 1, segs, 0.000000, -21.100000, -0.000000, 0.000000, 0.000000, 0.000000, 0.001000
GW 2, segs, 0.500000, 0.000000, 0.000000, 0.500000, 21.100000, 0.000000, 0.001000
GW 3, segs, 0.500000, -17.550000, -0.000000, 0.500000, 0.000000, 0.000000, 0.001000
GW 4, segs, 0.000000, 0.000000, 0.000000, 0.200000, 0.000000, -1.030000, 0.001000
GW 5, segs, 0.000000, 0.000000, 0.000000, 0.000000, 17.550000, 0.000000, 0.001000
GW 6, segs, 0.300000, 0.000000, -1.030000, 0.500000, 0.000000, 0.000000, 0.001000
GW 7, segs, 0.200000, 0.000000, -1.030000, 0.300000, 0.000000, -1.030000, 0.001000
GE
FR 0,0,3.650000,0,0,0,0,0
EX 0, 1, 1, 1.000000, 0.000000, 0,0,0
LD 0, 0, 1, 0, 0, 0, 0
! maa-segmentation: max-segs=800 segs-per-wl=80 taper=2 min-segs=2
! Mod by UR0GT, 02.04.2008 0:06:04
EN
```

The importer replaces MMANA's `-1` placeholder with the token `segs` in the textual GW lines and inserts the `SY` helper so the user can set a concrete segment count. The default value inserted is `segs=10`; change this value to a realistic per‑wire segment count and then replace `segs` with that integer before running a simulation. The textual `segs` token is not automatically propagated into numeric fields used by other parts of OpenNEC.
