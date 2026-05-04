cocoaNEC `.NC` File Format
==========================

Introduction
------------

[cocoaNEC](https://www.w7ay.net/site/Applications/cocoaNEC/index.html) is a macOS antenna modeling system written starting in 2002 by Kok Chen (W7AY). It provides  a GUI interface to the nec2c engine, and allows the antenna description to be entered  in three formats:

* "spreadsheet" mode, which saves an XML document with a `.nec` extension
* "deck" mode, which saves a traditional NEC-2 file with a `.deck` extension
* "NC" mode, short for "NEC C", which saves a *program* with a `.nc` extension

This document describes the NC format and how it can be used in a traditional NEC engine like OpenNEC. OpenNEC includes both an importer and an exporter for the cocoaNEC `.nc` scripting language; the implementation covers a practical subset sufficient to round-trip the example `.nc` files included with this repository. In contrast to some other formats, like `.EZ`, NC maps 1 to 1 onto NEC constructs, and conversion is always possible with no loss of information.

The choice of `.nec` for a non-NEC file, and `.deck` for those that are NEC format, is unfortunate as it means the default behaviour when opening one of these files may result in the wrong internal code being called.

Format overview
---------------

An NC file is plain text. It is compiled by cocoaNEC into a NEC-2 card deck before being sent to the solver. The structure is:

```
[global comments]
[global variable declarations]

model ( "model name" ) {
    [local variable declarations]
    [statements]
}

[optional: control() { … }]  - ignored for basic extraction
[optional: user-defined functions] - may be called from model()
```

- Everything from `//` to the end of the line is a comment.
- Whitespace (space, tab, newline) is ignored.
- Every statement ends with a semicolon `;`.
- Braces `{ }` group compound statements.
- The model name string (e.g. `"dipole"`) becomes the NEC-2 `CM` / `CE` comment card.

## Syntax example

The simplest possible NC file that produces input...

```
model ( "dipole" )
{
    voltageFeed( wire( 0, -5, 12, 0, 5, 12, 0.01, 21 ), 1, 0 ) ;
}
```
Comments
--------

The `model()` directive includes a title. If such a name is present, it is inserted into the output deck as a leading `CM` line. NC also allows comments at any point in the deck, and the conversion treats these differently depending on where they appear:

- if the comment appears before the `model`, it will be added to the comment block at the top of the deck. If there is also a title in the `model`, an empty CM will be added to make it more readable.
- if the comment appears within `model` section, on its own line, a new `!` type comment card will be inserted at that same point in the output deck.
- if the comment is within `model` section and is at the end of a non-comment line, it will be insertedas a `!` comment at the end of that line.
- comments found in parts of the NC we do not read are not retained.

Variable Declarations
---------------------

Variables must be declared before use. A declaration names a type followed by one or
more comma-separated identifiers, ending with `;`.

| NC type     | NEC-2 equivalent | Notes |
|-------------|-----------------|-------|
| `int`       | integer constant | Boolean true = 1, false = 0 |
| `real`      | floating-point constant | |
| `element`   | wire tag reference | Returned by `wire()` etc.; holds a tag number internally |
| `coaxtype`  | transmission-line descriptor | Used with coax functions |
| `vector`    | 3-D coordinate | Used with `wirev()` etc. |
| `transform` | affine transform | Used with `wirev()` etc. |

**Scope:** Variables declared outside all functions are global and visible everywhere. Variables declared inside a function body are local to that function.

Variables declared at global scope map directly to `SY` (symbol) cards when their values are numeric scalars (`int` or `real`). `element` variables hold internally-assigned wire tag numbers and do not appear in `SY` cards.

### Unit-conversion suffixes

cocoaNEC uses unit suffixes in a fashion similar to OpenNEC and 4nec2, that is, one can add a unit to a measurement like "10in" in any field or formula.

cocoaNEC differs from OpenNEC in that it also supports the alternative way of writing feet and inches, using the `'` and `"` characters. These are both used for other purposes in NEC decks, and cannot be easily converter in-place. When these are encountered the input file, they will be converted to `ft` and `in`. No examples of "mixed measurements" like `10'6"` were found, and this format is not supported.

NC also adds `u`, `n` and `p` units for entering small values. These will be converted to `uH`, `nH` and `pH`, respectively.

| Suffix | Emitted form | Runtime value |
|--------|-------------|---------------|
| `"` (double-quote, no space) | appends `in` unit symbol | `× 0.0254` (inches → metres) |
| `'` (single-quote, no space) | appends `ft` unit symbol | `× 0.3048` (feet → metres) |
| `#N` (hash before integer) | replaced by `awgN` symbol (e.g. `#14` → `awg14`) | AWG-N wire radius in metres |
| `u` | replaced by `*1e-6` multiplier | `× 1e-6` (micro) |
| `n` | replaced by `*1e-9` multiplier | `× 1e-9` (nano) |
| `p` | replaced by `*1e-12` multiplier | `× 1e-12` (pico) |

The `u`/`n`/`p` suffixes are recognised when immediately after a digit and not
followed by an alphanumeric character (so compound unit symbols like `uH`, `nF`,
`pF` that the evaluator already knows are left untouched).

Examples: `1.35"` becomes `1.35in`; `12'` becomes `12ft`; `#14` becomes
`awg14`; `100p` becomes `100*1e-12`; `47n` becomes `47*1e-9`.

Geometry Functions
------------------

NEC-2 requires coordinates in **metres**. Expressions using unit suffixes (e.g. `12ft`) evaluate to metres at runtime. Returned values are of type `element`.

### `wire( x0, y0, z0, x1, y1, z1, radius, segments )`
Defines a straight wire from `(x0,y0,z0)` to `(x1,y1,z1)` with the given radius and
segment count. NC automatically makes `segments` odd (adds 1 if even) so that a
feed can sit at the exact centre segment. Generates a NEC-2 **GW** card.

```nc
element fedWire ;
fedWire = wire( 0, -5, 12, 0, 5, 12, 0.01, 21 ) ;
```

### `line( x0, y0, z0, x1, y1, z1, radius, segments )`
Identical to `wire` but does **not** force an odd segment count. Generates a **GW** card.

### `taperedWire( x0, y0, z0, x1, y1, z1, radius, segmentLength0, segmentLength1 )`
Similar to `wire` but segment lengths (in wavelengths) follow a geometric progression
from `segmentLength0` at end 0 to `segmentLength1` at end 1. Generates one or more **GW** cards.

### Vector variants
For each of the above, a `v`-suffixed version (`wirev`, `linev`, `taperedWirev`) accepts
`vector` coordinates and an optional `transform` for rotation/translation:

```nc
wirev( t, v0, v1, radius, segments )
```

Use `nil` as the transform argument when no transformation is needed.

### Low-level card functions (deprecated but valid)
These emit NEC-2 cards directly and bypass the element reference system:

| NC call | NEC-2 card |
|---------|-----------|
| `gwCard( i1, i2, f1…f7 )` | GW |
| `gaCard( i1, i2, f1…f4 )` | GA (arc) |
| `ghCard( i1, i2, f1…f7 )` | GH (helix) |
| `gmCard( i1, i2, f1…f7 )` | GM (coordinate transform) |
| `grCard( i1, i2 )` | GR (cylindrical structure) |
| `gsCard( f1 )` | GS (scale structure) |
| `gxCard( i1, i2 )` | GX (reflection) |
| `spCard(…)` / `scCard(…)` | SP / SC (surface patch) |
| `smCard(…)` | SM (multiple patch) |

Wires created by card functions cannot be referenced by NC excitation/loading
functions (e.g. `voltageFeed`); use the native `wire()` / `line()` family instead, or
splice a short native wire between them.

Excitations (Feeds)
-------------------

All excitation functions target an `element` variable. They generate NEC-2 **EX** cards.

### Voltage feed
```nc
voltageFeed( element, real, imag )
voltageFeedAtSegment( element, real, imag, segment )
```
Places a voltage source `real + j·imag` at the centre of the wire (or at a specific
1-based segment for `AtSegment`).

### Current feed
```nc
currentFeed( element, real, imag )
currentFeedAtSegment( element, real, imag, segment )
```
Same shape as voltage feed but drives current rather than voltage.

### Plane-wave excitation
```nc
incidentPlaneWave( element, theta, phi, eta )
rightPolarizedIncidentPlaneWave( element, theta, phi, eta )
leftPolarizedIncidentPlaneWave( element, theta, phi, eta )
```

Loading
-------

Loading functions apply impedance or distributed R/L/C values to wire elements and
generate NEC-2 **LD** cards.

### Impedance load
```nc
impedanceLoad( element, R, X )
impedanceAtSegments( element, R, X, fromSegment, toSegment )
```
Attaches `R + jX` (ohms) to the centre segment, or to a range of segments.

### Series RLC
```nc
lumpedSeriesLoad( element, R, L, C )
distributedSeriesLoad( element, R, L, C )       // R, L, C per metre
seriesLoadAtSegments( element, R, L, C, perLength, fromSeg, toSeg )
```

### Parallel RLC
```nc
lumpedParallelLoad( element, R, L, C )
distributedParallelLoad( element, R, L, C )
parallelLoadAtSegments( element, R, L, C, perLength, fromSeg, toSeg )
```

### Single-component shortcuts
```nc
resistiveLoad( element, R )
inductiveLoad( element, L )
capacitiveLoad( element, C )
```

### Wire conductivity
```nc
conductivity( element, sigma )                  // sigma in S/m
conductivityAtSegments( element, sigma, fromSeg, toSeg )
```

### Insulated wire (sheath)
```nc
insulate( element, permittivity, conductivity, radius )
```
Uses NEC-4 IS card when available; falls back to the Yurkov approximation on NEC-2.

Frequency
---------

Frequency functions generate NEC-2 **FR** cards. All frequencies are in **MHz**.

```nc
setFrequency( f )           // single frequency; clears any previously set frequencies
addFrequency( f )           // add a second (or further) frequency point
frequencySweep( f0, f1, n ) // n equally spaced frequencies from f0 to f1 inclusive
```

**Default frequency:** if no frequency function is called the importer automatically
inserts `FR 0,1,0,0,14.0,0` (14 MHz) so the deck is always runnable.

Ground
------

Ground functions generate NEC-2 **GN** cards. Only one ground definition is active.

```nc
freespace()
poorGround()
averageGround()
goodGround()
perfectGround()
saltWaterGround()
freshWaterGround()
ground( epsilon_r, sigma )  // sigma in S/m
useSommerfeldGround( 1 )    // enable Sommerfeld-Norton approximation (1=yes, 0=no)
```

### Radials
```nc
necRadials( length, wireRadius, numRadials )
radials( x, y, z, length, wireRadius, numRadials )  // centred at (x,y,z)
```

Radiation Pattern Requests
--------------------------

These generate NEC-2 **RP** cards.

```nc
azimuthPlotForElevationAngle( angle )   // elevation angle in degrees
elevationPlotForAzimuthAngle( angle )   // azimuth angle in degrees
```

**Automatic full-sphere pattern:** regardless of whether the NC file calls any
radiation-pattern function, the importer always appends
`RP 0, 37, 73, 1000, 0, 0, 5, 5` (full sphere at 5° resolution) just before
`EN`. This mirrors what cocoaNEC itself produces and ensures output is always
generated.

Networks and Transmission Lines
-------------------------------

NC network functions generate NEC-2 **NT** and **TL** cards.

```nc
network( element1, element2, y11r, y11i, y12r, y12i, y22r, y22i )
networkAtSegments( element1, seg1, element2, seg2, y11r, y11i, y12r, y12i, y22r, y22i )

transmissionLine( element1, element2, z0 )
crossedTransmissionLine( element1, element2, z0 )
transmissionLineAtSegments( element1, seg1, element2, seg2, z0 )
longTransmissionLine( element1, element2, z0, electricalLength )
terminatedTransmissionLine( element1, element2, z0, electricalLength, y1r, y1i, y2r, y2i )
```

Physical coax and twin-lead helpers (see the Extensions page) expand into NT cards
computed from cable parameters (Zo, velocity factor, loss constants). Because the
frequency is required to compute the NT admittance matrix, one NT card is emitted per
FR card.

Miscellaneous Control Functions
-------------------------------

```nc
useExtendedKernel( 1 )      // enable NEC extended thin-wire kernel (EK card)
```

Mapping NC to NEC-2 Cards
-------------------------

| NC construct | NEC-2 card(s) |
|---|---|
| `model( "name" )` | `CM name` / `CE` comment block |
| `wire()` / `line()` / `taperedWire()` | `GW` |
| `arcCard` / `gaCard` | `GA` |
| `helixCard` / `ghCard` | `GH` |
| `gmCard` | `GM` |
| `grCard` | `GR` |
| `gsCard` | `GS` |
| `gxCard` | `GX` |
| `spCard` / `scCard` | `SP` / `SC` |
| `smCard` | `SM` |
| _[end of geometry]_ | `GE` |
| `voltageFeed` / `currentFeed` | `EX` |
| `incidentPlaneWave` | `EX` |
| `impedanceLoad` / `lumpedSeriesLoad` / etc. | `LD` |
| `conductivity` | `LD` |
| `insulate` | `IS` (NEC-4) or Yurkov `LD` approximation |
| `network` | `NT` |
| `transmissionLine` | `TL` |
| Physical coax / twin-lead helpers | `NT` (one per frequency) |
| `setFrequency` / `addFrequency` / `frequencySweep` | `FR` |
| `freespace` / `ground` / … | `GN` |
| `necRadials` / `radials` | `GN` + `GR` |
| `useSommerfeldGround` | `GN` flag |
| `azimuthPlotForElevationAngle` | `RP` |
| `elevationPlotForAzimuthAngle` | `RP` |
| `useExtendedKernel` | `EK` |
| global `int` / `real` variables | `SY` |

### Variables to SY cards

Global `int` and `real` scalar variables whose values are known at compile time map to
NEC-2 `SY` (symbol) cards. For example:

```nc
real length, height ;
int  segments ;
segments = 21 ;
height   = 12 ;
length   = 5 ;
```

produces:

```
SY segments=21
SY height=12
SY length=5
```

`element`, `coaxtype`, `vector`, and `transform` variables are internal compiler
bookkeeping and do not appear as `SY` cards.

Annotated Minimal Example
-------------------------

```nc
// simple dipole — minimal NC program
model ( "dipole" )
{
    element fedWire ;
    real length, height ;
    int  segments ;

    segments = 21 ;
    height   = 12 ;
    length   = 5 ;

    fedWire = wire( 0, -length, height, 0, length, height, 0.01, segments ) ;
    voltageFeed( fedWire, 1, 0 ) ;
    freespace() ;
    setFrequency( 14.15 ) ;
    azimuthPlotForElevationAngle( 0 ) ;
}
```

Resulting OpenNEC deck:

```
CM dipole
CE
SY segments=21
SY height=12
SY length=5
GW  1  segments  0  -length  height  0  length  height  0.01
GE  0
EX  0  1  (segments+1)/2  0  1  0
GN  -1
FR  0  1  0  0  14.15  0
RP  0  1  360  1000  0  0  0  1  0
EN
```

What to ignore for basic extraction
-----------------------------------

The following NC language features do **not** need to be interpreted to produce a
runnable NEC deck:

- `control()` function and all its contents
- `runModel()`, `vswr()`, `feedpointImpedance*()`, `maxGain`, etc.
- `while` / `repeat` / `break` loops
- `if` / `else` conditionals (unless they gate geometry — treat conservatively)
- User-defined recursive functions beyond simple substitution
- `keepDataBetweenModelRuns()`, `pause()`, `printf()`
- Vector and transform arithmetic beyond direct coordinate assignment

### Known unsupported pattern: geometry driven by `control()` variables

Some NC files declare global variables that are never assigned in the `model()` body;
their values are only set inside the `control()` block (which the importer skips).
When such variables are then used as wire coordinates or radii, the geometry is
undefined and conversion will fail with formula evaluation errors.

Example (`Gull.nc`):
```nc
real theta0, theta1, rho ;   // global — declared but never assigned in model()

model ( "Gull" ) {
    real y, dt, dy, dx ;
    …
    dt = rho*y ;             // rho is 0 (never assigned here)
    dy = dt*sind( theta0 ) ; // theta0 is 0 (never assigned here)
    …
}

control() {
    theta0 = 42 ;   // only set here — too late for geometry
    rho    = 0.57 ;
    runModel() ;
}
```

This pattern is used by parametric / optimization models where the `control()` block
sweeps parameter values and calls `runModel()` multiple times. Supporting it would
require executing the control block, including potential loops, conditionals, and
user‑defined function calls — well beyond a static importer. Such files **cannot be
imported** by `nc2nec` and should be converted manually or redesigned so that
parameter values are assigned in the `model()` body.

---

*References:*
- https://www.w7ay.net/site/Manuals/cocoaNEC/Manual/RefManual2/language.html
- https://www.w7ay.net/site/Manuals/cocoaNEC/Manual/RefManual2/NCFunctions.html
- https://www.w7ay.net/site/Manuals/cocoaNEC/Manual/RefManual2/Cards.html
- https://www.w7ay.net/site/Manuals/cocoaNEC/Manual/RefManual2/Extensions.html
