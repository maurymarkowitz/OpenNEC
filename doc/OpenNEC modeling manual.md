OpenNEC Modeling Manual
=======================

Introduction
------------

This manual describes how to build and analyze antenna models with OpenNEC. The manual covers the full input language, geometry, control cards, and output interpretation, and includes practical guidelines for constructing accurate models. Most of the material here applies to any NEC-2 compatable engine, and the manual broadly follows the structure of the NEC-2 User's Manual (Part III).

The manual also covers the modeling-related extensions found in OpenNEC: symbolic formulas, unit suffixes, wire-gauge notation, current source excitation, and additional loading types. The intent is that a user familiar with any NEC-2 simulator should find the card descriptions and modeling rules directly applicable, while OpenNEC-specific additions are clearly marked.

For a description of the OpenNEC program itself, refer to the [OpenNEC programmer manual](OpenNEC programmer manual.md).

I. Key concepts
---------------

### Why model antennas?

A computer model cannot replace building and measuring an antenna. It can, however, reveal important information that would otherwise be difficult to determine without extensive testing. This includes:

- **Feed-point impedance** across a frequency range, showing resonance and bandwidth
- **Radiation pattern** in the horizontal and vertical planes, showing gain, directivity, and front-to-back ratio
- **Effect of nearby objects:** the ground, a tower, a roof, other antenna elements
- **Sensitivity analysis:** what happens to performance when element length or spacing changes slightly

Modeling systems answer all of these from a description of the wire geometry and a few material parameters. While the results may not exactly match the performance of the real-world version, the *relative* performance is often very accurate. A typical model takes seconds to compute on modern hardware and can be iterated rapidly, giving the designer important notes on how to tune their real-world design.

A model is only as good as its description. A wire diameter that differs from the real antenna, or an incorrect segment count, can shift results substantially. The modeling guidelines in Section III describe how to choose segment counts and sizes to obtain reliable results.

### What is NEC?

The Numerical Electromagnetics Code (NEC) is a computational modeler developed at Lawrence Livermore National Laboratory (LLNL) in the 1970s. The Fortran source code to the NEC-2 version was released in the 1980s and now forms the basis for many permissive adaptations, including OpenNEC. Later versions, NEC-3 through NEC-5, are available from LLNL but only under a restrictive license. This makes NEC-2 the most widely used version, by far.

NEC describes the antenna as a collection of thin wire segments and optional surface patches. These basic objects are known as *elements*. During calculations, the wire elements are broken up into smaller pieces known as *segements*. Using the segments and patches, NEC assembles an impedance matrix **Z** whose entries encode the electromagnetic interaction between every pair of segments, and then solves the matrix equation:

$$\mathbf{Z}\,\mathbf{I} = \mathbf{V}$$

where **V** is the excitation vector (voltage sources) and **I** is the unknown current distribution on the antenna. From **I** the program computes feed-point impedance, near electric and magnetic fields, and far-field radiation patterns.

The integral-equation approach avoids many simplifying assumptions required by simpler methods such as transmission-line models or ray-tracing and provides highly accurate results for structures up to a few wavelengths in size.

### What is OpenNEC?

OpenNEC is a modern C re-implementation of NEC-2, designed to run on macOS, Linux, and Windows without a Fortran toolchain. It accepts the same input deck format as NEC-2, as well as the extensions found in nec2c and 4nec2, and produces the same `.out` output format, so existing models work directly. In addition to the standard card set, OpenNEC supports:

- **inline comments** allow you to better document the deck and any special features
- **SY (symbol) cards:** define named variables and formulas that are substituted into any subsequent numeric field
- **Unit suffixes:** lengths may be given in `ft`, `in`, `mm`, `cm`; frequencies in `kHz`, `MHz`, `GHz`; wire sizes as AWG (`#14`, `22awg`); inductances and capacitances in standard SI prefixes
- **EX 6 (current source),** a 4nec2 extension that treats the excitation as a specified current in Amperes rather than a voltage in Volts
- **LD 6 (LC-trap)** and **LD 7 (insulated wire),** additional loading types from 4nec2

### How a simulation works

A simulation runs in three phases:

1. **Geometry calculation:** the wire segment endpoints, midpoints, half-lengths, and connection data are computed from the GW (and GA, GH, GM, GR, GX, GS) cards.
2. **Matrix fill and factor:** the impedance matrix **Z** is assembled from contributions between all pairs of segments, then factored (LU decomposition). This is the most time-consuming step and scales as O(N³) in the number of segments N.
3. **Frequency loop:** for each requested frequency, the excitation vector **V** is assembled from EX cards, **Z**·**I** = **V** is solved for **I**, and the requested output (currents, near fields, radiation pattern) is computed from **I**.

II. Units, symbols, and formulas
--------------------------------

### Coordinate system

NEC uses a right-handed Cartesian coordinate system with the z-axis pointing upward. The ground plane, when present, lies at z = 0. Antenna elements are placed at positive z values; segments below z = 0 are illegal when a ground plane is active.

Radiation pattern angles follow the standard spherical convention:

- **θ (theta):** elevation angle measured downward from the +z (zenith) axis. θ = 0° is straight up; θ = 90° is horizontal; θ = 180° is nadir.
- **φ (phi):** azimuth angle measured counter-clockwise from the +x axis in the xy-plane. φ = 0° is along +x; φ = 90° is along +y.

### Base units

All numeric parameters in standard NEC-2 are in SI base units:

| Quantity | Unit |
|----------|------|
| Length, position | Metres (m) |
| Frequency | Megahertz (MHz) for FR card |
| Resistance / impedance | Ohms (Ω) |
| Conductivity | Siemens per metre (S/m) |
| Inductance | Henries (H) |
| Capacitance | Farads (F) |
| Current | Amperes (A) |
| Voltage | Volts (V) |
| Power | Watts (W) |

III. Structure modeling guidelines
--------------------------------

The basic elements for modeling structures with NEC are short, straight wire *segments* and flat surface *patches*. An antenna, and any nearby conducting object that affects its performance, must be described with segments following its wire paths and patches covering its surfaces. Curved wires and wire arrays are included, which are turned into individual segments during calculation.

Choosing segments and patches correctly is the most important step in producing accurate results. The number of segments should be the minimum necessary for accuracy; computation time increases roughly as the cube of the number of segments, so unnecessary segments cost considerably more than they gain.

### 1. Wire modeling

A wire segment is defined by the Cartesian coordinates of its two endpoints and a radius. Modeling a wire structure means following the paths of conductors with a piecewise-linear chain of segments.

#### Segment length

The segment length Δ relative to wavelength λ is the primary accuracy control. General rules:

- Δ ≤ λ/10 is a good default for most structures.
- Δ ≤ λ/20 is preferred in critical regions: near sources, loads, bends, or antenna tips where the current varies rapidly.
- Δ < 10⁻³ λ should be avoided. When segments are extremely short, the constant and cosine components of the current expansion become nearly identical, producing numerical inaccuracy.

A longer segment count does not automatically improve accuracy. Once segments are short enough to resolve the current distribution, adding more segments increases computation time with diminishing benefit.

#### Wire radius and the Δ/a ratio

The ratio of segment length to wire radius, Δ/a, affects the accuracy of the thin-wire kernel approximation:

- **Thin-wire kernel (default):** Δ/a > 8 for error < 1%. Values down to about Δ/a = 2 typically give useful results, but numerical oscillations near wire ends and sources become more apparent as Δ/a decreases.
- **Extended thin-wire kernel (EK card):** extends the reliable range to Δ/a ≥ 2, and useful results down to Δ/a ≈ 0.5. Use the EK card whenever the model includes thin segments relative to radius.

The wire radius relative to wavelength is limited by the thin-wire approximation. Only axial currents are modelled; circumferential current variation is ignored. This is valid when 2πa/λ ≪ 1. Above this limit the results should be treated with caution.

#### Wire junctions

Segments that are electrically connected must have exactly coincident endpoints. The program treats segments as connected when the distance between their ends is less than 10⁻³ times the shortest segment length; use identical coordinates where possible.

The program does not restrict the angle at which wires join. However, if the acute angle at a junction is so small that the centre of one segment falls within the radius of the other (overlapping segments), the results will be meaningless. Segments should be several wire radii apart when routed in parallel.

No more than 30 segments may meet at a single junction.

#### Radius changes at junctions

Abrupt radius changes between connected segments reduce accuracy, especially when Δ/a is small. When a radius step is needed, taper it over several segments. The GW card's GC continuation card may be used to taper radius along a single wire group automatically.

#### Sources and loads

Each feed point or lumped load requires a complete segment at that location. Do not insert a gap; the wire must be continuous across the source location so that the voltage drop can be applied as a boundary condition on an existing segment.

A charge density discontinuity voltage source (EX type 5) requires the two segments on either side of its connection point to be parallel, equal in length, and equal in radius.

#### Parallel close-spaced wires

When two parallel wires are very close together, misaligned segment junctions produce spurious current perturbations. Align segment boundaries along parallel wires to the extent possible, and maintain a separation of at least several wire radii.

#### Wire-grid modeling of surfaces

A single wire grid can represent both sides of a conducting plate. The grid currents represent the sum of currents on both surfaces; individual surface currents cannot be recovered, but the radiated far field is correct. Because broad guidelines for near-field accuracy from wire grids have not been established, wire-grid near-field results should be used with caution.

### 2. Surface modeling

Surface patches model closed conducting surfaces using the magnetic-field integral equation. A patch is characterised by its centre coordinates, outward unit-normal vector, and area.

#### Patch size

At least 25 patches per square wavelength of surface area should be used, with no individual patch larger than about 0.04 λ². Smaller patches are needed near edges, wire connections, and regions of high surface curvature.

#### Patch aspect ratio

Avoid very long, narrow patches. Aim for patches that are as nearly square as practicable, particularly in areas connected to wires. The supplied SP and SM cards generate rectangular patches; for complex surfaces, choose subdivision strategies that maintain acceptable aspect ratios near the poles and edges.

#### Wire-to-surface connections

A wire may connect to a surface only at a patch center, with the wire endpoint and patch center sharing identical coordinates. The program subdivides the patch at that connection into four equal patches. Connected patches should be approximately square and have sides parallel to the surface tangent vectors.

#### Closed surfaces only

Surface patches correctly model only the outer side of the surface (the side from which normals point outward). The modeled surface must be closed. Thin planar sheets are not correctly modeled with patches alone; a wire-grid approximation is more appropriate in that case.

### Key formulas

**Free-space wavelength:**

$$\lambda = \frac{c}{f} = \frac{300}{f_\text{MHz}} \text{ metres}$$

**Half-wave dipole length (approximate):**

$$L_{1/2} \approx \frac{0.95 \lambda}{2} = \frac{142.5}{f_\text{MHz}} \text{ metres}$$

*(The 0.95 shortening factor accounts for end effects; exact value depends on wire radius.)*

**Segment length guideline:**

$$\Delta \approx \frac{\lambda}{10} \text{ to } \frac{\lambda}{20}$$

**Segment length to radius ratio:**

$$\frac{\Delta}{a} > 8 \quad \text{(thin-wire kernel)}$$
$$\frac{\Delta}{a} > 2 \quad \text{(extended thin-wire kernel, EK card)}$$

**Skin depth (for LD 0 surface impedance loading):**

$$\delta = \sqrt{\frac{2}{\omega \mu \sigma}} = \frac{1}{\sqrt{\pi f \mu \sigma}}$$

where σ is conductivity (S/m), μ ≈ 4π×10⁻⁷ H/m (non-ferromagnetic wire), f is frequency in Hz.

### 3. Modeling structures over ground

Several ground models are available. See [Section III, GN card](#gn--ground-parameters) and [Section III, GD card](#gd--additional-ground-parameters) for full card descriptions, and [Appendix C](#appendix-c-typical-ground-values) for typical earth parameter values.

#### Perfect ground (GN 1)

The ground is modeled as a mirror, generating image currents exactly equivalent to a perfectly conducting plane. This is the most accurate and fastest option for ground modeling; it is exact for perfectly conducting surfaces. Structures may be very close to the ground in this mode.

For a horizontal wire of radius a at height h, the constraint h² + a² > 10⁻¹² λ² (approximately h ≥ 1 pm for λ = 1 m) is effectively always satisfied.

#### Reflection coefficient approximation (GN 2 or GN 3, remote structures)

For finitely conducting ground, image currents are modified by the Fresnel plane-wave reflection coefficient computed at each specular reflection point. This method is fast but of limited accuracy:

- The antenna should be at least 0.1-0.2 λ above the ground.
- Not suitable for large horizontal antennas (traveling-wave structures) because the reflection-coefficient approximation does not account for the ground wave.

#### Sommerfeld-Norton method (GN 2 or GN 3)

The exact Sommerfeld-Norton formulation integrates the fields in the presence of a lossy half-space. This method is accurate for antennas close to the ground and is the standard choice for terrestrial antenna design. It is available only for wire structures; surface-to-surface interactions use the reflection coefficient approximation even when Sommerfeld-Norton is selected for the rest of the model.

Sommerfeld-Norton computation takes approximately four times longer than free-space computation. The calculations are correct to high accuracy for heights down to those applicable to a perfect ground (see above).

#### Radial wire ground screen (GN 2, nradl > 0)

When a number of radial wires are specified in the GN card, their effect is approximated via a surface impedance formula. This models ground-plane antennas and monopoles with radial screens without explicitly placing segment-by-segment wire geometry. The approximation yields correct results for a vertical monopole (the current at the base is the same as over a perfect ground) but does not include diffraction from the screen edge.

#### Two-medium ground / cliff edge (GD card)

A linear or circular cliff may be defined using the GD card, specifying different ground parameters and height on opposite sides. The cliff is applied during far-field computation only; it does not affect the near-field interaction matrix. This option is suitable for coastal or terrain-discontinuity studies.

IV. Program input
-----------------

An NEC input file (deck) consists of cards arranged in three sections:

1. **Comment section:** normally contains at least one or more `CM` ccomment ards and ends at a `CE`. May consist of a single `GE`.
1. **Geometry section:** begins at the first non-comment card and ends with a `GE` card.
2. **Control section:** follows `GE` and normally ends with `EN`, or `XT` or `NX` in some decks.

Each card occupies one line. The first two characters are a letter-pair mnemonic. A deck may contain multiple sections separated by `NX` (Next Structure) cards, allowing several antenna configurations to be computed in sequence.

Cards generally come in two forms, with and without required parameters. Those cards that do require parameters, like geometry and most control cards, generally have up to four integer values following the mnemonic, and then up to seven floating point values.

### Comment cards

#### CM: Comment

```
CM [text]
```

An arbitrary comment line. In NEC-2 these can only appear in the comment section at the top of the deck. In later engines, including OpenNEC, they may appear anywhere. The text following `CM` is echoed to the output but otherwise ignored.

#### CE: Comment end / geometry begin

```
CE [text]
```

Marks the boundary between the comment block and the geometry section. An optional trailing comment is echoed to output. The geometry section begins on the next card. Classic NEC decks lacking a `CE` are considered ill-formed in OpenNEC.

#### !, ' and #: alternative comment formats

As NEC-2 became more common, various authors added new cards to allow comments to be inserted anywhere in the deck. These were simply dropped from the list of cards as they were fed into the various pre-existing NEC functions. There was no standard for these, so different authors added whatever was most familiar to them; the original LLNL team used `!` because that was what is normally used in Fortran, PC authors generally used `'`, the short-form marker for `REM` in MS BASIC, and nec2c used `#`. OpenNEC supports all of these markers.

It is also common to allow these extended markers to be used at the end of a card, to document anything interesting on that card. However, nec2c's `#` is more commonly used in other engines to indicate an AWG wire gauge. For that reason, OpenNEC only treats a `#` as a comment marker if it is at the start of the line. `'` or `!` should generally be used instead, and judging by the decks found on the internet, `'` seems to be most common.

OpenNEC does not treat these types of comment cards as part of the comment section, so at least a `CE` is still required for the deck to be considered valid.

### Structure geometry cards

The geometry section describes the physical structure. Cards may appear in any order except that `GC` must immediately follow a corresponding `GW`, and `SC` cards (if needed) must follow a `SM`. The geometry section ends with a `GE`.

#### GW: Wire specification

```
GW  tag  segs  x1  y1  z1  x2  y2  z2  radius
```

Defines one straight wire from point (x1,y1,z1) to (x2,y2,z2), divided into `segs` equal segments, each of radius `radius`. The wire is assigned tag number `tag`.

| Field | Type | Description |
|-------|------|-------------|
| tag | int | Tag number for this wire (1-9999) |
| segs | int | Number of segments |
| x1,y1,z1 | float | Endpoint 1 coordinates (metres) |
| x2,y2,z2 | float | Endpoint 2 coordinates (metres) |
| radius | float | Wire radius (metres) |

**Example: half-wave dipole at 14.2 MHz:**
```
SY f=14.2
SY hl=142.5/f/2
GW  1  21  0  0  0    0  0  hl  0.001
```

**Note:** Segment count should be odd for a centre-fed element (so the feed-point segment falls exactly at the wire centre).

#### GC: Wire tapering (follows GW)

```
GC  0  0  rdel  rrad
```

When placed immediately after a `GW` card, `GC` specifies tapering of segment length and wire radius along the wire. `rdel` is the ratio of the length of consecutive segments (uniform = 1.0); `rrad` is the ratio of wire radius between consecutive segments.

#### GA: Wire arc

```
GA  tag  segs  arc_radius  ang1  ang2  wire_radius
```

Generates a circular arc in the xz-plane. `ang1` and `ang2` are the start and end angles in degrees. A full circle may be made with ang1=0, ang2=360.

#### GH: Helix / spiral

```
GH  tag  segs  spacing  length  a1  b1  a2  b2  wire_radius
```

Generates a helical wire. `spacing` is the axial distance between turns (negative for left-hand helix), `length` is total axial length, `a1/b1` are the semi-axes at the start, `a2/b2` at the end (for uniform helix, a1=a2, b1=b2).

#### GM: Coordinate transformation (move/rotate/copy)

```
GM  tag_inc  num_new  rx  ry  rz  x  y  z
```

Translates and/or rotates all currently defined segments. `rx/ry/rz` are rotation angles (degrees) about each axis. `x/y/z` is the translation vector. If `num_new` > 0, creates `num_new` copies of the current structure at successive displacements/rotations. `tag_inc` is added to the tag number of each new copy.

#### GR: Generate cylindrical structure

```
GR  tag  ncopies
```

Replicates the current tagged structure `ncopies` times by rotation about the z-axis, creating a cylindrical (azimuthally symmetric) array.

#### GX: Reflection in coordinate planes

```
GX  tag  flagcode
```

Reflects the tagged structure across one or more coordinate planes. `flagcode` is a three-digit integer XYZ where each non-zero digit requests a reflection across the corresponding plane (e.g., 100 = reflect across yz-plane, 011 = reflect across both xz and xy planes).

#### GS: Scale structure dimensions

```
GS  0  0  scale_factor
```

Multiplies all coordinates and radii of previously defined segments by `scale_factor`. Useful for rescaling a model built in one set of units to another; also accepts unit suffix tokens (`in`, `ft`, `cm`, `mm`) as the scale factor.

**Example: scale from inches to metres:**
```
GS 0 0 in
```

#### SP: Single surface patch

```
SP  0  shape  x  y  z  elev  azim  area
```

Defines one flat surface patch. `shape` selects the patch geometry (0=arbitrary, 1=rectangular, 2=triangular, 3=quadrilateral). `x/y/z` is the patch centre; `elev/azim` define the outward normal direction; `area` is the patch area (m²). For shaped patches, one or more `SC` continuation cards follow.

#### SM: Multiple rectangular patches

```
SM  n1  n2  x  y  z  ...
SC  x  y  z
```

Generates a rectangular patch surface divided into n1 × n2 patches. The geometry is defined by three corner points across `SM` and `SC` cards.

#### SC: Surface patch continuation

```
SC  ...
```

Provides additional corner coordinates for `SP` or `SM` cards. Must immediately follow its parent `SP` or `SM`.

#### GF: Read Numerical Green's function file

```
GF  [filename]
```

Loads a pre-computed Numerical Green's Function (NGF) from a binary file. If no filename is given, the NGF file is inferred from the deck filename with a `.ngf` extension. The NGF contains the interaction matrix for a previously computed geometry; this geometry is merged with the current deck's geometry, avoiding redundant matrix computations for large fixed structures.

#### GE: End geometry

```
GE  [ground_flag]
```

Marks the end of the geometry section. `ground_flag`:
- `0`: no ground plane.
- `1`: ground plane present; wire endpoints at or near z=0 are connected to the image in the ground.
- `-1`: ground plane present; wire endpoints near z=0 are not connected to the image (for wires insulated at the base).

### Program control cards

Control cards follow the `GE` card and configure the simulation parameters. They may appear in any order, subject to the dependency rules described in the Introduction. Default values apply when a card is omitted.

Group I (affect the impedance matrix, must be set before the first matrix fill):

> EK, FR, GN, KH, LD

Group II (affect only the excitation, matrix is not recomputed when these change):

> EX, NT, TL

Group III (affect only output calculations):

> CP, EN, GD, NE, NH, NX, PQ, PT, RP, WG, XQ

#### FR: Frequency

```
FR  ifrq  nfrq  0  0  freq_start  freq_step
```

Sets the frequency or frequency sweep. `ifrq` selects the step type:
- `0`: linear frequency step: frequencies are freq_start, freq_start + freq_step, …, freq_start + (nfrq-1)×freq_step (MHz).
- `1`: multiplicative step: each frequency is the previous value multiplied by freq_step.

If `nfrq` > 1, the frequency loop runs `nfrq` iterations, producing results at each frequency.

**Example: sweep 40m band, 10 kHz steps:**
```
FR  0  11  0  0  7.0  0.01
```

#### EK: Extended thin-wire kernel

```
EK
```

Activates the extended thin-wire kernel for all subsequent calculations. Should be used when any segment has Δ/a < 8, or when small wire ends, bends near sources, or fine-radius modelling requires extra accuracy. Has no additional parameters.

#### KH: Interaction approximation range

```
KH  0  0  0  0  range
```

For segments separated by more than `range` wavelengths, the program uses an approximate (far-field) interaction instead of the full near-field integral. This substantially reduces fill time for large models where distant interactions are small. Typical values: 0.5 to 1.0 wavelengths. Set to 0 to disable approximation (default).

#### GN: Ground parameters

```
GN  type  nradl  0  0  epsr  sigma  [cl  ch]
```

Specifies the ground model below the antenna.

| Field | Description |
|-------|-------------|
| type | Ground type: -1=free space, 0=image method auto, 1=perfect ground, 2=Sommerfeld-Norton, 3=NEC-4.2 Sommerfeld |
| nradl | Number of radial ground-screen wires (0=none) |
| epsr | Relative dielectric constant (permittivity) of earth |
| sigma | Conductivity of earth (S/m) |
| cl | Cliff edge distance (wavelengths); omit if no cliff |
| ch | Cliff height (wavelengths); omit if no cliff |

Typical ground parameter values are tabulated in [Appendix C](#appendix-c-typical-ground-values).

**GN -1 (free space):**
```
GN  -1
```

**GN 1 (perfect ground):**
```
GN  1
```

**GN 2 (average earth, Sommerfeld-Norton):**
```
GN  2  0  0  0  13  0.005
```

**GN 2 with 8-radial ground screen:**
```
GN  2  8  0  0  13  0.005  0.25  0.5
```

#### GD: Additional ground parameters
```
GD  0  0  0  0  epsr2  sigma2  [cl  ch]
```

Provides high-accuracy Sommerfeld table parameters to supplement a `GN` card. `epsr2` and `sigma2` specify the ground parameters for the second medium in a two-medium (cliff) configuration. In most models this card is not needed; OpenNEC computes appropriate defaults.

#### LD: Structure loading

```
LD  type  tag  segs_from  segs_to  r_or_sl  xl  xc
```

Loads specified segments with lumped-element impedance. `tag` selects the wire to load; `segs_from` and `segs_to` specify the range of segments within that tag (0,0 = all segments with that tag).

| Type | Model |
|------|-------|
| 0 | Series RLC: R (Ω/m), L (H/m), C (F/m), distributed per unit length |
| 1 | Series RLC: R (Ω), L (H), C (F), lumped at segment centre |
| 2 | Parallel RLC: R (Ω), L (H), C (F), lumped at segment centre |
| 3 | Series impedance Z = R + jX (Ω): F1=R, F2=X |
| 4 | Parallel admittance Y = G + jB (S): F1=G, F2=B |
| 5 | Wire conductivity loading: F1=conductivity (S/m) |
| 6 | LC trap (4nec2): F1=L (H), F2=C (F), parallel trap |
| 7 | Insulated wire (4nec2/Cebik): F1=εr, F2=thickness (m) |

**Example: resistive load at dipole feed:**
```
LD  3  1  11  11  50  0
```

**Example: copper wire conductivity on tag 1:**
```
LD  5  1  0  0  58000000
```

Wire conductivity values for common metals are tabulated in [Appendix D](#appendix-d-wire-conductivity-values).

#### EX: Excitation

```
EX  type  tag  seg  i4  re  im
```

Applies an excitation source to the antenna.

| Type | Excitation |
|------|------------|
| 0 | Applied-field voltage source (standard): re+j·im Volts on segment (tag, seg) |
| 1 | Incident plane wave, linear polarisation |
| 2 | Incident plane wave, right-hand elliptic polarisation |
| 3 | Incident plane wave, left-hand elliptic polarisation |
| 4 | Elementary current source (infinitesimal current dipole at a point in space) |
| 5 | Charge-discontinuity voltage source (located at the junction between two parallel equal-length segments) |
| 6 | Current source (4nec2 extension): re+j·im Amperes, see below |

For type 0 and 5, `tag` and `seg` identify the segment. A `seg` value ending with `%` specifies a percentage position along the wire (e.g., `50%` means the centre segment).

Multiple EX cards may be given for multiple sources at different segments.

**EX 0: voltage source at dipole centre (tag 1, 11th of 21 segments):**
```
EX  0  1  11  0  1  0
```

**EX 0 with percentage position:**
OpenNEC supports the 4nec2 extension to `EX` that allows the position to be specified by percentage-of-length. Using this format means changes to the segmentation of the target wire does not required you to find any `EX` cards and adjust their segment number. In practice, this is almost always 50%, meaning the middle of the target wire:

```
EX  0  1  50%  0  1  0
```

**EX 6, current source:**
OpenNEC supports the 4nec2 extension for current sources of (re + j·im) Amperes at segment (tag, seg):

```
EX  6  tag  seg  0  re  im
```

OpenNEC implements this by internally generating a synthetic dummy wire segment with a 1 V voltage source and connecting it to the target segment through an NT admittance network with Y₁₂ = −I_desired. The result is that the target segment carries the specified current.

**Example: 1 Amp current source at centre of 11-segment dipole (tag 1):**
```
EX  6  1  6  0  1.0  0.0
```

#### NT: Two-port network

```
NT  tag1  seg1  tag2  seg2  y11r  y11i  y12r  y12i  y22r  y22i
```

Connects two segments through an ideal two-port admittance network. The network equations are:

$$I_1 = Y_{11}V_1 + Y_{12}V_2$$
$$I_2 = Y_{12}V_1 + Y_{22}V_2$$

This is used to model general two-port networks, including transformers, hybrid couplers, and other passive elements. Currents are sign-convention positive flowing into the network from the segment.

**Example: 1-to-4 impedance transformer between tags 1 and 2:**
```
NT  1  6  2  6  0.02  0  -0.02  0  0.02  0
```

#### TL: Transmission line

```
TL  tag1  seg1  tag2  seg2  z0  len  ys_r  ys_i  yd_r  yd_i
```

Models a lossless transmission line connecting two segments. `z0` is the characteristic impedance (Ω); `len` is the physical length (metres, positive). If `z0` < 0, a crossed (180°-shifted) transmission line is modelled (useful for certain phasing networks). `ys_r/ys_i` and `yd_r/yd_i` are optional shunt admittances at each end of the line.

**Example: 50 Ω coaxial feed, 5 m long, between tag 1 and tag 2:**
```
TL  1  11  2  1  50  5
```

#### CP: Maximum coupling calculation

```
CP  tag1  seg1  tag2  seg2
```

Requests computation of the maximum theoretical coupling (in dB) between two antenna ports. The result is based on the S-parameters computed from the antenna currents and is printed in the output.

#### NE: Near electric field

```
NE  coord  n_i  n_j  n_k  x0  y0  z0  di  dj  dk
```

Requests calculation of the near electric field on a grid. `coord` selects the coordinate system (0=rectangular, 1=cylindrical, 2=spherical). The grid is defined by an origin (x0,y0,z0), number of points in each direction (n_i, n_j, n_k), and step sizes (di, dj, dk).

#### NH: Near magnetic field

Same format as `NE`; requests the near magnetic field.

#### PT: Wire current print control

```
PT  print_type
```

Controls printing of wire current data. `print_type`: -1=suppress all; 0=print currents; 1=print currents and charges; 2=print normalised currents.

#### PQ: Wire charge print control

```
PQ  print_type
```

Controls printing of wire charge density. `print_type`: -1=suppress; 0=print.

#### RP: Radiation pattern

```
RP  mode  n_theta  n_phi  options  theta0  phi0  d_theta  d_phi  [radial_dist]
```

Requests a far-field radiation pattern. `mode` 0 = normal pattern; `mode` 1 = surface-wave pattern. The pattern grid spans from (theta0, phi0) in steps of (d_theta, d_phi) for n_theta × n_phi points. `options` (a 4-digit integer XNOR) controls normalization, output format, and inductive-near-field mode.

**Example: elevation pattern at phi=0, 0.5° steps, 0-180°:**
```
RP  0  37  1  1000  0  0  5  0
```

**Example: full 3D pattern, 5° steps:**
```
RP  0  37  73  1000  0  0  5  5
```

#### XQ: Execute

```
XQ
```

Triggers computation of currents (and any near fields or patterns specified in preceding cards) without immediately requesting a radiation pattern. Used to force execution when no RP card follows.

In OpenNEC, `XQ` can be very useful when it is being used as a library in another program. Normally an `RP` or similar would be used to generate text output that would then be read and parsed. However, as OpenNEC exposes the calculation data directly, the output generation and parsing can be skipped by using an `XQ` and then reading the resulting data directly.

#### WG: Write Numerical Green's function file

```
WG  [filename]
```

Writes the current structure's interaction matrix to an NGF binary file for reuse by a subsequent `GF` card. If no filename is given, the NGF file is named from the deck filename with a `.ngf` extension.

#### NX: Next structure

```
NX
```

Terminates the current calculation section and begins a new one within the same deck file. All control card settings are reset. The geometry is re-read from the next geometry section in the file.

OpenNEC uses certain cards in the deck to mark off locations, like the start and end of the geometry section. Using an `NX` means there may be more than one of each section, which may lead to confusion. It is suggested that `NX` be avoided in modern decks.

#### EN: End of data

```
EN
```

Marks the end of the deck. Required. Any lines after `EN` are treated as comments.

V. OpenNEC extensions: units, symbols and formulas
--------------------------------------------------

### OpenNEC unit suffixes

OpenNEC extends the standard NEC-2 numeric fields with unit suffixes. Any numeric field (integer or float) may include a unit suffix that scales the value to the appropriate base unit before use.

#### Length suffixes

| Suffix | Meaning | Multiply by |
|--------|---------|-------------|
| `m` | metres | 1.0 |
| `cm` | centimetres | 0.01 |
| `mm` | millimetres | 0.001 |
| `in` or `"` | inches | 0.0254 |
| `ft` or `'` | feet | 0.3048 |

**Example:** `GW 1 11 0 0 0 0 0 5ft 0.001` places a wire from z=0 to z=5 feet. During calculations, the value is automatically converted to the underling measurement system, typically but not always meters, and then feeds the resulting 1.524 m value into subsiquent calculations.

#### Frequency scale

All FR card frequencies are in MHz by default. The suffix `kHz` or `GHz` may be used if needed:

| Suffix | Multiply by |
|--------|-------------|
| `Hz` | 1×10⁻⁶ |
| `kHz` | 1×10⁻³ |
| `MHz` | 1 (default) |
| `GHz` | 1000 |

#### Wire gauge (AWG)

Wire radius or diameter may be specified as an AWG gauge:

| Format | Example |
|--------|----------|
| `#NN` (radius) | `#14` → radius = 0.8153 mm |
| `NN awg` (radius) | `14awg` → same result |
| `#NN diam` (diameter) | `#14 diam` → diameter = 1.6306 mm |

A full AWG-to-millimetre conversion table is given in [Appendix A](#appendix-a-awg-conversion-table).

#### Electrical component suffixes

| Suffix | Meaning | Example |
|--------|---------|----------|
| `Ohm`, `kOhm`, `MOhm` | Resistance | `50 Ohm` |
| `H`, `mH`, `uH`, `nH`, `pH` | Inductance | `5.5uH` |
| `F`, `mF`, `uF`, `nF`, `pF` | Capacitance | `45pF` |

### Symbols and formulas (SY card)

`SY` cards were introduced by thr 4nec2 program. They allow you to define a variable or formula that can then be referred to in places where numeric values would otherwise be used. In antenna design, it is common to have dimensions that are based on the wavelength of the target frequency, and using `SY` cards to define these makes it much easier to change these values for experimentation. For decks designed using 4nec2, which are common on the internet, you will often find wavelength related symbols at the top of the deck:

```
SY freq=14.2
SY lambda=300/freq
SY diplen=lambda/2
```

Symbols may reference previously defined symbols, and can also include standard mathematical operators: `+`, `-`, `*`, `/`and `^`, and functions functions: `sin`, `cos`, `tan`, `sqrt`, `pow`, `abs`, `log`, `log10`, `exp`, `int`, `ceil`, `floor`, `round`.

Once defined, a symbol name may be used in any numeric field of any subsequent card:

```
GW 1 21  0 0 0  0 0 diplen/2  #14
```

Symbols can be defined anywhere in the deck, as long as the definition is before its use. This also allows a symbol to be re-defined to a new value, which will then be used for subsequent lines. It is common to see wire radius defined in this fashion, with an initial value and then changing it as the antenna moves towards longer elements that have larger diameter conductors.

Generally, it is considered good practice to define the symbols in their own block, between the `CE` and the first geometry card. 

VI. Output
----------

OpenNEC writes results to a `.out` file (same name and directory as the input deck). The output is plain text in a format compatible with NEC-2 output.

### Output sections

**Structure specification:** echoes the input geometry: wire table, segment table with coordinates, connection data, and tag numbers.

**Data cards:** echoes all non-comment control cards actually processed.

**Frequency:** prints the operating frequency and computed wavelength.

**Structure impedance loading:** prints the loaded impedance on each segment (if any LD cards were given).

**Antenna environment:** confirms the ground model in use.

**Matrix timing:** reports time to fill and factor the impedance matrix.

**Network data:** prints the admittance network parameters from NT and TL cards.

**Currents and location:** for each segment: coordinates, length, and the complex current in Amperes.

**Power budget:** input power, radiated power, gain, and efficiency.

**Near-field data:** (if NE/NH requested) field components at each grid point.

**Radiation patterns:** (if RP requested) for each (θ, φ) point: E-theta, E-phi magnitudes, total gain (dBi), vertical and horizontal gain.

### Reading the current table

The current table is the primary diagnostic output. For a source-driven antenna:

- Current magnitude peaks at the feed point and tapers toward wire ends.
- At a half-wave resonant dipole, the sinusoidal taper is clearly visible.
- Unexpected current nulls interior to the wire suggest a junction not connected (may be a rounding error in coordinates) or an error in tag/segment addressing.
- Very large or very small currents at a particular segment relative to neighbours suggest numerical problems from extreme Δ/a ratios.

### Reading the power budget

The power budget prints input power, radiated power, and efficiency. The feed-point impedance is not printed directly in the power budget section; it appears in the current table header for each source segment. For an EX 0 (voltage source) with voltage V and current I at the source segment:

$$Z_{in} = V/I \quad (\Omega)$$

The program prints the source current under the current table column for the corresponding segment.

VII. Examples
--------------

### Example 1: Half-wave dipole (free space)

A classic half-wave dipole in free space at 14.2 MHz with 21 segments:

```
CM  Half-wave dipole, 14.2 MHz, free space
CE
SY  f=14.2
SY  hl=142.5/f/2
GW  1  21  0  0  0    0  0  hl  #14
GE  0
GN  -1
FR  0  1  0  0  f
EX  0  1  11  0  1  0
RP  0  37  73  1000  0  0  5  5
EN
```

**Expected output:** Feed-point impedance ≈ 73 + j0 Ω at exact resonance. Peak gain ≈ 2.15 dBi broadside.

### Example 2: Vertical monopole over real ground

A quarter-wave vertical at 7 MHz over average earth:

```
CM  Quarter-wave vertical, 7.0 MHz, average earth
CE
SY  f=7.0
SY  ql=71.25/f
GW  1  21  0  0  0    0  0  ql  0.003
GE  0
GN  2  0  0  0  13  0.005
FR  0  1  0  0  f
EX  0  1  11  0  1  0
RP  0  37  73  1000  0  0  5  5
EN
```

**Expected output:** Feed-point impedance ≈ 36 Ω (half of the free-space dipole value, due to the ground image). Radiation pattern emphasizes low elevation angles.

### Example 3: Two-element Yagi

A simple 2-element Yagi at 144 MHz with a driven element and a director:

```
CM  2-element Yagi, 144 MHz
CE
SY  f=144
SY  lam=300/f
SY  el=0.48*lam           ' driven element half-length
SY  dl=0.45*lam           ' director half-length
SY  sp=0.2*lam            ' element spacing
GW  1  21  0  0  -el    0  0  el   0.001     ' driven element
GW  2  21  0  sp  -dl   0  sp  dl   0.001     ' director
GE  0
GN  -1
FR  0  1  0  0  f
EX  0  1  11  0  1  0
RP  0  37  73  1000  0  0  5  5
EN
```

### Example 4: Frequency sweep with formula variables

Sweeping a dipole over the 20m band to find the resonant frequency:

```
CM  Dipole frequency sweep, 14–14.4 MHz
CE
GW  1  21  0  0  0    0  0  5.26  0.001
GE  0
GN  -1
FR  0  9  0  0  14.0  0.05
EX  0  1  11  0  1  0
XQ
EN
```

The FR card runs 9 frequencies from 14.0 to 14.4 MHz in 0.05 MHz steps.

### Example 5: Horizontal square loop antenna

A small horizontal square loop at 14.2 MHz, 10 metres above average ground. Loop antennas exhibit a sharp null perpendicular to the plane of the loop and are useful for direction finding:

```
CM  Horizontal square loop, 14.2 MHz, 10m height, average earth
CE
SY  f=14.2
SY  lam=300/f
SY  half_lam=lam/2
SY  side=half_lam/4
SY  height=10
GW  1  11  0  0  height    side  0  height  0.001
GW  2  11  side  0  height    side  side  height  0.001
GW  3  11  side  side  height    0  side  height  0.001
GW  4  11  0  side  height    0  0  height  0.001
GE  1
GN  2  0  0  0  13  0.005
FR  0  1  0  0  f
EX  0  1  1  0  1  0
RP  0  37  73  1000  0  0  5  5
EN
```

**Expected output:** Feed-point impedance in range of 50-70 Ω at frequency. Radiation pattern shows broad side-fire pattern with nulls perpendicular to the loop plane. Horizontal loops at moderate heights are excellent for low-angle DX radiation.

### Example 6: Log-periodic dipole array (LPDA) for VHF

A four-element log-periodic dipole array at 144 MHz (2m amateur band). LDPAs are broadband antennas with relatively constant gain and impedance across a wide frequency range. Each element is slightly shorter and closer-spaced than the previous:

```
CM  Four-element LDPA, 144 MHz center, VHF
CE
SY  f=144
SY  lam=300/f
SY  len1=0.49*lam
SY  len2=len1*0.87
SY  len3=len2*0.87
SY  len4=len3*0.87
SY  z1=0
SY  z2=z1+0.15*lam
SY  z3=z2+0.18*lam
SY  z4=z3+0.20*lam
GW  1  13  -len1/2  0  z1    len1/2  0  z1  #18
GW  2  13  -len2/2  0  z2    len2/2  0  z2  #18
GW  3  13  -len3/2  0  z3    len3/2  0  z3  #18
GW  4  13  -len4/2  0  z4    len4/2  0  z4  #18
GW  5  41  0  0  z1    0  0  z4  0.001
GE  0
GN  -1
FR  0  1  0  0  f
EX  0  1  7  0  1  0
RP  0  37  73  1000  0  0  5  5
EN
```

**Expected output:** Feed-point impedance relatively flat across 130-150 MHz (approximately 50 Ω). Gain around 7-8 dBi. Radiation pattern exhibits moderate directivity with a primary lobe perpendicular to the boom axis. Suitable for satellite communication and weak-signal VHF work.

VIII. Error messages
--------------------

OpenNEC reports errors at two stages:

**Deck validation:** before simulation runs, the deck is checked for structural and semantic problems. Errors and warnings are printed to stderr. A FATAL error aborts the run.

**Simulation:** errors detected during matrix computation, geometry calculation, or output generation stop the run at that point with an explanatory message.

### Common error messages

| Message | Cause | Fix |
|---------|-------|-----|
| GW on line N: invalid tag or segment count | Tag ≤ 0 or segs < 1 | Correct GW parameters |
| Segment N extends below ground | Segment endpoint at z < 0 with GN ≥ 1 | Raise z1/z2 above 0 |
| Segment loop: segment M on card N connects to segment P | Segment connection graph has a cycle | Fix geometry coordinates |
| EX on line N: references invalid tag T, segment S | Tag/segment does not exist in geometry | Check tag numbers and segment counts |
| The length of segment N is too small | Segment has zero length | Check endpoint coordinates |
| The radius of segment N is too small | Radius ≤ 0 | Use positive wire radius |
| LD on line N: type 6 requires both L and C | LD 6 trap is missing a component | Provide both F1 (L) and F2 (C) |
| NT on line N: references invalid tag | Network connects to a nonexistent segment | Check NT tag parameters |
| GF on line N: cannot open NGF file | File not found or path incorrect | Check NGF file location |
| WARNING: large model detected | More than ~1000 segments | Reduce segment count or allow extra run time |\

Glossary of Terms
-----------------

**Antenna** — A conductor or array of conductors designed to emit or receive electromagnetic radiation. In NEC modeling, an antenna is represented as a collection of wire segments (and optionally surface patches) connected to excitation sources. The antenna's properties (impedance, gain, radiation pattern) are computed from the electromagnetic currents induced by the excitation.

**Card** — A single line in a NEC deck file. Each card begins with a two-letter mnemonic (e.g., `GW`, `FR`, `EX`) followed by numeric fields or parameters. The term originates from the era when NEC input was punched onto 80-column computer cards.

**Complexity** — A dimensionless estimate of the computational work required to simulate a deck, computed by the `estimate_time()` function, and denoted as `T` in the original NEC documentation. Complexity grows roughly as O(N³) for matrix fill-and-factor and O(N²) for far-field calculations. Values below roughly 10⁷ run in under 0.1 seconds on contemporary hardware; values above 10¹¹ may take several minutes.

**Conductor** / **Conductive Surface** — A material with high electrical conductivity (low resistance), such as copper, aluminum, or seawater. In antenna modeling, perfect conductors (infinite conductivity) are assumed unless ground parameters (GN card) specify a lossy earth material.

**Coupling Coefficient** — A measure of electromagnetic interaction between two or more elements (e.g., between antenna elements). Computed with the `CP` card; values range from 0 (no coupling) to 1 (complete coupling). High coupling indicates strong mutual impedance effects.

**Deck** — A plain-text input file containing one or more NEC cards describing an antenna and its simulation parameters. Decks typically use file extensions `.nec`, `.deck`, or `.onec` (OpenNEC format). A complete deck must include geometry, frequency, excitation, and control cards, terminated by an `EN` card.

**Dielectric Constant** (Relative Permittivity, εr) — A measure of how strongly a material responds to an electric field. Vacuum/air ≈ 1.0. Higher values indicate stronger electric field concentration (e.g., fresh water εr ≈ 80, ceramics εr ≈ 5–10). Used in the GN card to model lossy ground.

**Dipole** — A simple two-element antenna consisting of two equal-length conducting rods fed at the center. A half-wave dipole at its design frequency exhibits omnidirectional radiation in the plane perpendicular to its axis. Commonly used as a reference antenna for gain calculations.

**Directivity** — The ratio of maximum radiation intensity in the desired direction to the average radiation intensity in all directions. Directivity is independent of losses and ground effects; it characterizes the antenna's pattern shape only. Given in dBi (decibels relative to isotropic).

**Excitation** — A current or voltage source applied to the antenna (typically at a feed point) to drive its radiation. Specified with an `EX` card; includes source tag number, segment number, current/voltage magnitude, and phase.

**Element** — A single wire or surface patch used as a building block in antenna geometry. In a wire antenna, elements are defined with the `GW` card; collections of elements form the complete antenna structure. Example: a three-element Yagi consists of three wire elements (reflector, driven element, director) positioned and oriented according to the design.

**Far-Field** — The region far from the antenna where radiation patterns are computed. Typically defined at ranges where the antenna appears as a small point source and radiation is predominantly plane-wave. Computed with the `RP` card; results include field strengths, power density, and directivity.

**Frequency** — The oscillation frequency of the electromagnetic wave (Hz, kHz, MHz, GHz). Specified with the `FR` card. Common amateur radio bands: 7 MHz (40m), 14.2 MHz (20m), 28 MHz (10m), 144 MHz (2m). Higher frequencies require finer segmentation.

**Frequency Sweep** — Running multiple simulations at different frequencies with a single deck (specified via `FR` card frequency stepping mode 0 or 1). Allows quickly building impedance curves or pattern data across a frequency range.

**Gain** — Radiation efficiency × directivity. Gain includes the effects of antenna losses (resistance, ground losses). Given in dBi (decibels relative to isotropic) or dBd (decibels relative to dipole). Gain ≤ directivity for real antennas (with losses).

**Green's Function** — The fundamental solution to the wave equation for electromagnetic fields in the presence of a ground plane or free space. NEC computes Green's functions via Sommerfeld integrals (for real ground) or method-of-images (for perfect ground). Numerical Green's Function (NGF) tables can be cached in files (GF/WG cards) for reuse.

**Ground** — The earth or conducting surface beneath an antenna. Modeled via the `GN` card:
  - `GN -1` = free space (no ground)
  - `GN 1` = perfect ground (perfect conductor)
  - `GN 2` or `GN 3` = real ground (lossy, Sommerfeld-Norton method)

**Image Method** — A technique for computing radiation in the presence of a perfect conducting ground. Uses the principle of images: the ground is approximated by a mirror image current, eliminating the need for numerical integration. Very fast but only valid for perfectly conducting surfaces.

**Impedance** — A complex electrical property characterizing the resistance to current flow at the antenna feed point. Given as R + jX where R is resistance (real part) and X is reactance (imaginary part). Measured in Ohms (Ω). A well-matched antenna has impedance close to the transmission line characteristic impedance (typically 50 Ω).

**Incidence Angle** — The angle at which a wave approaches the antenna or ground surface, measured from the vertical (z-axis). Related to the theta (θ) and phi (φ) angles in radiation patterns.

**Kernel** — The mathematical function used to compute interactions between current-carrying segments. NEC-2 uses the thin-wire kernel by default; the `EK` card enables an extended kernel for more accurate computation of fields near small-radius wires.

**Loading** — A series impedance (resistance R, inductance L, or capacitance C) added to a wire segment. Used to model coils, matching networks, or resistive elements. Specified with the `LD` card.

**Matrix Solver** — The numerical engine that solves the system of linear equations derived from the Method of Moments. Available methods include Gaussian elimination (LU decomposition) and iterative solvers. Performance depends on matrix size (number of segments) and BLAS backend.

**Method of Moments** (MOM) — The fundamental numerical technique used by NEC. The antenna geometry is discretized into segments, and the integral equations for electromagnetic fields are converted into a matrix equation: **Z** · **I** = **V**, where **Z** is the impedance matrix, **I** is segment current, and **V** is excitation voltage. Solving for **I** gives the current distribution, from which radiation patterns and impedance are computed.

**Near-Field** — The region very close to the antenna where the field structure is complex and not plane-wave-like. Computed with the `NE` (rectangular grid) or `NH` (spherical) cards. Useful for analyzing mutual coupling, ground effects, and feed-point phenomena.

**NEC** — Numerical Electromagnetics Code; the foundational antenna simulation software originally developed at Lawrence Livermore National Laboratory in the 1970s–1980s. Multiple implementations exist: NEC-2 (original Fortran release), NEC-4 (extensions), nec2c (C port), and OpenNEC (modern C re-implementation).

**Numerical Integration** — Computational method for evaluating definite integrals with high accuracy. The Sommerfeld-Norton ground model uses numerical integration (Romberg method) to compute ground field contributions. More accurate than image method but slower.

**Patch** — A triangular surface element used to model thin conducting sheets or surfaces. Generated with the `SP` (single patch) or `SM` (multiple patches) cards. Useful for modeling antenna arrays mounted on aircraft fuselages or ships.

**Permittivity** — See "Dielectric Constant".

**Radiation Pattern** — A polar or Cartesian plot showing the antenna's radiated power intensity as a function of direction (theta θ and phi φ angles). Specified with the `RP` card; results are written to the `.out` file and can be visualized with external tools.

**Reflection Coefficient** — A complex number describing how a wave reflects off a surface (ground, or from an impedance discontinuity). Depends on the surface conductivity, frequency, and incidence angle. For perfect ground, magnitude = 1 (complete reflection); for lossy ground, magnitude < 1.

**Resistivity** — The resistance of a 1-meter cube of material; inverse of conductivity (σ = 1/ρ). Often specified instead of conductivity in legacy references. Example: seawater ≈ 0.2 Ω·m (conductivity 5 S/m).

**Segment** — A straight-line subdivision of a wire, used to discretize geometry for the Method of Moments. Each segment carries a piecewise-sinusoidal current distribution. More segments improve accuracy but increase computation time. Segment length should be roughly λ/10 to λ/20 for good results.

**Sommerfeld-Norton Method** — The standard numerical technique for computing ground effects for a lossy (real) earth. Integrates contributions from an infinite lossy half-space, accounting for ground conductivity and permittivity. More accurate than perfect ground but slower. Used when `GN 2` or `GN 3` is specified.

**SY Card** — Symbol card; defines a variable or constant for use in subsequent cards. Example: `SY freq=14.2, lambda=300/freq`. Variables are evaluated at parse time and substituted into all referenced fields. Extends the base NEC-2 format (originates in 4nec2 and nec2c variants).

**Tag** — An integer identifier (1–9999 in standard NEC) assigned to each segment or patch to facilitate referencing in cards like `EX`, `LD`, `RP`, etc. Tags are user-assigned; multiple segments can share the same tag for convenience.

**Theta (θ)** — Elevation angle in spherical coordinates; angle measured downward from the antenna's vertical axis (z-axis). θ=0° is straight up; θ=90° is horizontal; θ=180° is straight down. Used in radiation pattern calculations (`RP` card).

**Thin-Wire Kingdom** — Modeling assumption that antenna wires are thin compared to their length (radius << length). Allows simplification of field computation. For very thin wires or very high frequencies, use the `EK` card (extended kernel) for improved accuracy.

**Transmission Line** — A two-conductor structure (e.g., coaxial cable) connecting two antenna segments. Specified with the `TL` card; includes characteristic impedance, physical length, and velocity factor. Used to model feed networks, collinear arrays, and transmission line matching.

**Unit Suffixes** — Shorthand notation for SI units and common engineering units. Length: `ft`, `in`, `mm`, `cm`, `m`. Frequency: `Hz`, `kHz`, `MHz`, `GHz`. Impedance: `Ohm`, `kOhm`, `MOhm`. Inductance: `H`, `mH`, `µH`, `nH`. Capacitance: `F`, `µF`, `nF`, `pF`. Wire gauge: `awg` (American Wire Gauge). Example: `14.2 MHz` instead of `14200000 Hz`.

**Velocity Factor** — The ratio of the speed of light in a transmission line medium to the speed of light in vacuum. Typically 0.66 for coaxial cable, 0.95 for air-insulated transmission line. Used in the `TL` card to compute electrical length.

**Wire Gauge** (AWG, American Wire Gauge) — A standard system for specifying wire diameter. Finer gauges (higher numbers) are thinner; thus #10 AWG (2.588 mm) is thicker than #22 AWG (0.644 mm). OpenNEC recognizes `#NN` or `NN awg` format and converts to diameter in meters. See [AWG Conversion Table](#awg-conversion-table) for a complete lookup.

**Yagi Antenna** — A directional antenna consisting of a driven element (fed with current/voltage), a reflector (slightly longer, parasitically coupled), and one or more directors (slightly shorter, parasitically coupled). Simple to build and commonly used at VHF/UHF frequencies. Characterized by high directivity and moderate gain (relative to dipole).

Appendices
----------

### Appendix A: AWG conversion table

| AWG | Diameter (in) | Diameter (mm) | Radius (mm) |
|-----|--------------|---------------|-------------|
| 4/0 | 0.4599 | 11.6838 | 5.8419 |
| 3/0 | 0.4096 | 10.4050 | 5.2025 |
| 2/0 | 0.3648 | 9.2659 | 4.6330 |
| 1/0 | 0.3249 | 8.2523 | 4.1262 |
| 1 | 0.2893 | 7.3482 | 3.6741 |
| 2 | 0.2576 | 6.5437 | 3.2719 |
| 3 | 0.2294 | 5.8268 | 2.9134 |
| 4 | 0.2043 | 5.1892 | 2.5946 |
| 5 | 0.1819 | 4.6209 | 2.3105 |
| 6 | 0.1620 | 4.1148 | 2.0574 |
| 7 | 0.1443 | 3.6652 | 1.8326 |
| 8 | 0.1285 | 3.2639 | 1.6320 |
| 9 | 0.1144 | 2.9058 | 1.4529 |
| 10 | 0.1019 | 2.5883 | 1.2942 |
| 11 | 0.0907 | 2.3046 | 1.1523 |
| 12 | 0.0808 | 2.0523 | 1.0262 |
| 13 | 0.0720 | 1.8288 | 0.9144 |
| 14 | 0.0641 | 1.6281 | 0.8141 |
| 15 | 0.0571 | 1.4503 | 0.7252 |
| 16 | 0.0508 | 1.2903 | 0.6452 |
| 17 | 0.0453 | 1.1506 | 0.5753 |
| 18 | 0.0403 | 1.0236 | 0.5118 |
| 19 | 0.0359 | 0.9119 | 0.4560 |
| 20 | 0.0320 | 0.8128 | 0.4064 |
| 21 | 0.0285 | 0.7239 | 0.3620 |
| 22 | 0.0253 | 0.6426 | 0.3213 |
| 23 | 0.0226 | 0.5741 | 0.2871 |
| 24 | 0.0201 | 0.5105 | 0.2553 |
| 25 | 0.0179 | 0.4547 | 0.2274 |
| 26 | 0.0159 | 0.4039 | 0.2020 |
| 27 | 0.0142 | 0.3607 | 0.1804 |
| 28 | 0.0126 | 0.3200 | 0.1600 |
| 29 | 0.0113 | 0.2870 | 0.1435 |
| 30 | 0.0100 | 0.2540 | 0.1270 |
| 32 | 0.0080 | 0.2032 | 0.1016 |
| 34 | 0.0063 | 0.1600 | 0.0800 |
| 36 | 0.0050 | 0.1270 | 0.0635 |
| 38 | 0.0040 | 0.1016 | 0.0508 |
| 40 | 0.0031 | 0.0787 | 0.0394 |

### Appendix B: Typical coaxial and transmission line values

These values are recognised by OpenNEC when referenced via SY-defined variables and may also be used in `TL` card entries:

| Line | Z₀ (Ω) | Velocity Factor |
|------|--------|----------------|
| RG-6 | 75 | 0.75 |
| RG-8 | 52 | 0.66 |
| RG-8A | 52 | 0.66 |
| RG-8X | 50 | 0.78 |
| RG-8 foam | 50 | 0.78 |
| RG-11 | 75 | 0.66 |
| RG-11 foam | 75 | 0.78 |
| RG-58 | 53.5 | 0.66 |
| RG-58A | 50 | 0.66 |
| RG-58C | 50 | 0.66 |
| RG-59 | 75 | 0.66 |
| RG-59 foam | 75 | 0.79 |
| RG-213 | 50 | 0.86 |
| 300 Ω twin-lead (flat) | 300 | 0.82 |
| 300 Ω twin-lead (tubular) | 300 | 0.80 |
| 450 Ω open line | 450 | 0.95 |

###  Appendix C: Typical ground values

For use with the `GN` card. Source: ITU-R P.527; ARRL Antenna Book; pe2bz.philpem.me.uk.

| Ground Type | Conductivity (S/m) | Relative Permittivity (εr) |
|-------------|--------------------|-----------------------------|
| Poor (dry, thin soil) | 0.001 | 5 |
| Moderate | 0.003 | 4 |
| Average | 0.005 | 13 |
| Good | 0.015 | 17 |
| Dry, sandy, coastal | 0.001 | 10 |
| Pastoral hills, rich soil | 0.007 | 17 |
| Medium hills and forest | 0.004 | 13 |
| Mountainous hills (< 1000 m) | 0.002 | 5 |
| Rocky, steep hills | 0.002 | 13 |
| Fertile land | 0.002 | 10 |
| Rich agricultural, low hills | 0.010 | 15 |
| Marshy, densely wooded | 0.0075 | 12 |
| Marshy, forested, flat | 0.008 | 12 |
| Highly moist ground | 0.005 | 30 |
| City industrial area | 0.0001 | 3 |
| City industrial (average) | 0.001 | 5 |
| City industrial (max. atten.) | 0.0004 | 3 |
| Fresh water | 0.001 | 80 |
| Sea water | 5.0 | 81 |
| Sea water (0.2 Ω, ≤ 1 GHz) | 4.0 | 80 |
| Sea ice | 0.001 | 4 |
| Polar ice | 0.0003 | 3 |

### Appendix D: Wire conductivity values

For use with `LD 5` (surface impedance loading by conductivity). These are bulk DC conductivities; at high frequencies, skin effect reduces effective conductivity. Use `LD 0` (distributed series impedance) when accurate frequency-dependent wire loss is needed.

| Material | Conductivity (S/m) |
|----------|--------------------|
| Perfect conductor | 9.9×10⁹⁹ (use GN 1) |
| Silver | 62,900,000 |
| Copper | 58,000,000 |
| Aluminium | 37,700,000 |
| Aluminium alloy T832 | 30,800,000 |
| Aluminium alloy T6 | 24,900,000 |
| Brass | 15,600,000 |
| Phosphor bronze | 9,090,000 |
| Stainless steel | 1,390,000 |
| Insulator (reference) | 0.00001 |

### Appendix E: Insulation and dielectric constant values

For use with `LD 7` (insulated wire).

| Material | Relative Permittivity (εr) |
|----------|-----------------------------|
| Air | 1.0 |
| Styrofoam | 1.03 |
| Paraffin | 2.0 |
| Polyethylene | 2.4 |
| Polystyrene | 2.4-3.0 |
| Teflon (PTFE) | 2.1 |
| Rubber | 2.7-3.2 |
| Plexiglas | 2.6-3.5 |
| Paper | 1.6-2.6 |
| Polycarbonate | 2.9-3.2 |
| Polyamide (nylon) | 3.4-3.5 |
| Shellac (natural) | 2.9-3.9 |
| PVC (hard) | 3.0-4.0 |
| PVC (soft) | 4.0-5.0 |
| Bakelite | 3.5-4.5 |
| Oil | 1.5-4.7 |
| Mica | 4.0-8.0 |
| Neoprene | 4.0-6.7 |
| Porcelain | 5.0-6.5 |
| Glass | 5.0-9.0 |
| Glass (window) | 7.6 |
| Aluminium oxide | 10.0 |
| Copper oxide | 18.1 |
| Distilled water | 34-78 |
| Wood (dry) | 1.4-2.9 |

The dielectric constant of air is approximately 1.0; specifying εr = 1.0 produces the same result as bare wire.

References
----------

1. Burke, G.J. and Poggio, A.J. (1981). *Numerical Electromagnetics Code (NEC) — Method of Moments*. NOSC Technical Document 116. (Part I: Theory.)
2. Burke, G.J. (1992). *Numerical Electromagnetics Code (NEC-2) — Program Description and User's Manual*. LLNL UCID-18834. (Part III: User's Manual.)
3. Burke, G.J. (1981). *Numerical Electromagnetics Code (NEC-2) — Program Description Code*. NOSC Technical Note N-833. (Part II: Code.)
4. Voors, A. (2021). *4nec2: NEC-based antenna modeler*, version 5.9.3. https://www.qsl.net/4nec2/
5. Cebik, L. B. *Antenna Modeling Notes* (series). https://www.antennex.com/shack/
6. ARRL. *Antenna Modeling Course*. American Radio Relay League.
