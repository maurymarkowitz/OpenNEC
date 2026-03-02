MMANA‑GAL ".maa" file format
=================================

Introduction
------------

The MMANA‑GAL software (and its derivatives) uses a lightweight ASCII format for saving antenna models.  The files conventionally carry the `.maa` extension, or more rarely `.mma`, which contain an ordere sequence of text lines.  Although the format is undocumented, a large number of examples on‑line allow the structure to be inferred.  This document summarises the features that OpenNEC’s import/export code understands and points out the sections that are currently ignored.

The variation seen in the wild ranges from the minimal 4‑line variant used by simple exporters (frequency, counts, wires, loads, sources) up through more elaborate files which include segmentation parameters, ground and measurement options, and comment blocks.  The import routine in `mma-support.c` handles the minimal subset; the exporter produces only the basic sections.

After reading this description you should be able to glance at an existing `.maa` file and understand which parts will be carried over when using OpenNEC's conversion functions.

Format overview
---------------

A typical `.maa` file is organised into the following logical sections.  Two
distinct structural variants exist in the wild (see *Format variants* below).
`***…***` section headers are effectively required — a survey of 935 real‑world
files found only one without them, and that turned out to be a NEC deck saved
with the wrong extension.

1. **Title line.**  Arbitrary text used as a description of the model.  When the file is imported the converter creates a `CM` card containing this line (followed immediately by a `CE` card).  During export the first comment card in the deck is written back as the title line.  This preserves the user‑supplied description across conversions.
2. **Frequency line.**  A single floating‑point value giving the design
   frequency in megahertz.  Some files (Variant B) include a bare `*` on a
   separate line between the title and the frequency; this is ignored.
3. **Counts.**  In Variant A a single line holds three integers: wire count,
   load count, source count.  In Variant B each count appears on its own line
   immediately inside the relevant `***…***` section.
4. **Wire (geometry) block.**  Exactly `N_wires` following lines, each
   containing eight numeric values and usually a trailing “‑1” placeholder.
   The fields represent the end‑point coordinates of a straight wire in
   metres, the radius in metres, and the number of segments.  Example:
   ````
   0.0, -21.1, -3.662e-07,  0.0, 0.0, 0.0, 0.001, -1
   ````
5. **Source (EX) block.**  A line giving the count (usually same as
   number of excitations), followed by that many lines with four values:
   wire number, segment number, voltage magnitude and phase (degrees).
   The importer converts magnitude/phase into real/imaginary parts.
6. **Load (LD) block.**  A count line then load definitions.  Each load
   is expressed as wire number, segment number, R, X, L, C.  The code
   maps series RLC or fixed impedance constructs; parallel loads are
   ignored.
7. **Optional extra sections.**  Many real‑world `.maa` files then contain
   further labelled blocks such as
   `***Segmentation***` (integer segmentation parameters),
   `***G/H/M/R/AzEl/X***` (ground type/parameters and measurement options),
   and final `###Comment###` lines with free‑form text.  The marker is
   sometimes followed by the comment on the same line, but more commonly the
   text appears on the next line; our importer handles both forms and keeps
   the comment at its original position relative to the geometry and load
   blocks.  During import each comment is turned into a CM/CE card, and when
   exporting any comment cards present in the deck are emitted back as
   `###Comment### …` lines.  The remaining extra sections are still **ignored**.

Whitespace is permissive: commas or any combination of spaces and tabs may separate the numeric fields.  The lines may also contain leading/trailing spaces.  The format is case‑insensitive.

Format variants
---------------

A survey of 935 real‑world `.maa` files revealed two structural variants (see
[MMA format survey.md](MMA%20format%20survey.md) for the full per‑file table).

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

    144CQlomba
    *                  ← optional bare asterisk line
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

<MMA-B>  ::= <title> NEWLINE
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

<extra>  ::= "***Segmentation***" NEWLINE <seg-params>
           | "***G/H/M/R/AzEl/X***" NEWLINE <ground-params>
           | "###Comment###" [SP <text> | NEWLINE <text>]
           | other line  // ignored

SEP      ::= "," | whitespace+
```

The `***…***` headers and `###Comment###` markers may appear anywhere between
sections and are interpreted or skipped as described above.

Features not supported by OpenNEC
---------------------------------

* Any section other than the basic wires/loads/sources is ignored.  This
  includes: segmentation parameters, ground definitions, measurement
  settings, stacking information, etc.
* The exporter does not emit `***…***` headers or comments; it produces the
  minimal subset suitable for round‑tripping through the Python converters.
* Filenames in the collection sometimes contain other units (feet, inches)
  or spatial rotations.  Only coordinates in metres are handled by the
  importer; unit conversions are **not** performed.
* Parallel loading networks, voltage sources with special types, and
  certain non‑linear loads present in some `.maa` examples are not mapped.

Because of these omissions, running `read_deck_maa` followed by `write_deck_maa` on a real‑world `.maa` file will generally produce a much smaller file containing only the structural geometry, loads, and a single frequency.  Use of the converter is appropriate when you merely need the wire geometry and excitations, but it should not be relied on for preserving MMANA‑specific tuning, ground options, or comments.

Examples
--------

A minimal file produced by the current exporter looks like this:
```
OpenNEC export
14.000000
1 0 0
0.000000, 0.000000, 0.000000, 1.000000, 0.000000, 0.000000, 0.001000, 5
```
Which corresponds to a single wire from (0,0,0) to (1,0,0) with radius 1 mm and five segments.

The earlier sample (`Broadband antenna 80m 3.5 - 3.8MHz …`) shown above is
an example of a richer file; the additional headers and fields would be
ignored when imported by OpenNEC.

Contact
-------

For improvements to the converter or support for additional MMA features,
please open an issue on the OpenNEC GitHub repository.
