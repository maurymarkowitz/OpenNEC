EZNEC `.EZ` File Format
=======================

Introduction
------------

The `.EZ` file format is the proprietary binary format used by EZNEC and EZNEC Pro antenna-modelling software by Roy Lewellan (W7EL). It stores antenna geometry, excitation, loading, ground parameters and display settings in a compact little-endian binary representation. In data terms, it is similar to a wire-only NEC-2 file.

The format was reverse-engineered by inspecting 5,478 `.EZ` / `.ez` files from L.B. Cebik's published model collection, cross-referencing them with matching `.NEC` text files that EZNEC exports. Because it's reverse-engineered, the field names below are not official. Fields that we can be sure of are marked ✓, uncertain fields are marked ?.

File Identification
-------------------

Almost all `.EZ` files begin with the byte sequence:

```
xx xx 00 00 52 ...
```

where `xx xx` is a little-endian 16-bit wire count and `0x52` = `'R'` appears to be a constant version/format marker.

File sizes range from ~680 bytes for a file with 1 wire and no loads, to several hundred kilobytes for models with hundreds of wires.

Top-Level Structure
-------------------

```
┌────────────────────────────────────────────────────────────────┐
│  Header Block 1   (bytes 0x00–0x2F, 48 bytes)                  │
├────────────────────────────────────────────────────────────────┤
│  Header Block 2   (bytes 0x30–0xB9, 138 bytes)                 │
│    includes EZNEC version code, unit code, display constants   │
├────────────────────────────────────────────────────────────────┤
│  Wire Records     (starting at 0x00BA)                         │
│    N × 170-byte records, one per wire                          │
├────────────────────────────────────────────────────────────────┤
│  Source / Load / Ground / RP blocks  (after wire records)      │
│    Partially decoded — see section below                       │
└────────────────────────────────────────────────────────────────┘
```

Header Block 1 (0x00–0x2F)
--------------------------

| Offset | Size | Type       | Description |
|--------|------|------------|-------------|
| 0x00   | 2    | int16      | ✓ Number of wires (`N`) |
| 0x02   | 2    | int16      | (?) Usually 0; sometimes 1 |
| 0x04   | 1    | char       | ✓ Format marker — always `'R'` (0x52) in examined files |
| 0x05   | 4    | float32    | ✓ Design frequency in **MHz** |
| 0x09   | 1    | char       | (?) Unit / coordinate-system flag.  `'A'` is most common; `'E'` observed in some models.  See Unit System section. |
| 0x0A   | 2    | —          | (?) Reserved; always `0x00 0x00` |
| 0x0C   | 4    | float32    | (?) Ground dielectric constant or display scale; often 0.0 |
| 0x10   | 2    | —          | Sentinel bytes — nearly always `0x80 0x3F` |
| 0x12   | 30   | char[30]   | ✓ Model title, ASCII, space-padded (ends at 0x2F) |

**Example** (`dblt8.ez`, 3.5 MHz doublet):

```
0000  01 00 00 00 52 00 00 60 40 41 00 00 00 00 00 00  ....R..`@A......
0010  80 3f 33 2e 35 20 64 6f 75 62 6c 65 74 20 38 20  .?3.5 doublet 8
0020  77 6c 20 20 20 20 20 20 20 20 20 20 20 20 20 20  wl              
```

Decoding:
- `01 00` → 1 wire
- `52` → format marker `'R'`
- `00 00 60 40` → float32 = **3.5 MHz** ✓
- `41` → unit flag `'A'`
- `80 3f` → sentinel
- bytes 0x12–0x2F → `"3.5 doublet 8 wl              "` ✓

Header Block 2 (0x30–0xB9)
--------------------------

| Offset | Size | Type       | Description |
|--------|------|------------|-------------|
| 0x30   | 2    | int16      | ✓ Wire count (repeated) |
| 0x32   | 2    | int16      | (?) Source count |
| 0x34   | 2    | —          | Reserved |
| 0x36   | 1    | char       | ✓ NEC ground type: `'R'`=real/Sommerfeld, `'F'`=free space (GN −1), `'P'`=perfect |
| 0x37   | 1    | —          | Reserved |
| 0x38   | 8    | —          | Reserved flags; often contains `00 ff ff 18` |
| 0x40   | 1    | byte       | (?) EZNEC version major digit; value `0x05` observed for v5 |
| 0x41   | 1    | char       | (?) Units code in display: `'I'`=inches, `'F'`=feet, `'M'`=meters, `'W'`=wavelength |
| 0x42   | 2    | char[2]    | (?) Constant `"FT"` in examined files |
| 0x44   | 4    | float32    | (?) Constant 1303.8 — possibly max frequency or display limit |
| 0x48   | 4    | float32    | (?) 0.0 |
| 0x4C   | 4    | float32    | (?) Constant 360.0 — possibly display distance limit |
| 0x50   | 2    | char       | (?) `'Z'` byte + padding |
| 0x51   | 104  | —          | Antenna view / display settings (zoom, rotation, etc.) |

Wire Records (0x00BA onward)
----------------------------

Wire records begin at byte offset **0x00BA** (186 decimal) and each record is **170 bytes** (0xAA) long.  Records are contiguous:

```
wire[0] at 0x00BA
wire[1] at 0x00BA + 170  = 0x0164
wire[2] at 0x00BA + 340  = 0x020E
wire[k] at 0x00BA + k × 170
```

### Wire Record Layout (170 bytes)

| Record offset | Size | Type       | Description |
|---------------|------|------------|-------------|
| +0x00         | 4    | float32    | ✓ X coordinate of endpoint 1 |
| +0x04         | 4    | float32    | ✓ Y coordinate of endpoint 1 |
| +0x08         | 4    | float32    | ✓ Z coordinate of endpoint 1 |
| +0x0C         | 4    | float32    | ✓ X coordinate of endpoint 2 |
| +0x10         | 4    | float32    | ✓ Y coordinate of endpoint 2 |
| +0x14         | 4    | float32    | ✓ Z coordinate of endpoint 2 |
| +0x18         | 4    | float32    | ✓ Wire **diameter** (= 2 × NEC GW radius) |
| +0x1C         | 2    | —          | ✓ Record separator: always `0xFF 0xFF` |
| +0x1E         | 2    | int16      | ✓ Segment count for this wire |
| +0x20         | 2    | int16      | (?) Wire tag / sequential number |
| +0x22         | 4    | —          | (?) Reserved |
| +0x26         | 4    | float32    | (?) Source voltage (1.0 for default 1-V excitation) |
| +0x2A         | 4    | float32    | (?) Source imaginary component or phase |
| +0x2E         | 1    | byte       | (?) `0x56` = `'V'` observed — possibly "voltage source" flag |
| +0x2F–+0xA9   | 107  | —          | Remaining source/load per-wire data; structure not fully decoded |

### Notes on Wire Geometry

- **Units** — Coordinates are stored in the same unit the user chose in EZNEC (feet or meters).  The two most common cases observed in the collection are:
  - **Feet** — typical for US-authored HF models (values like 342.62, 85.655)
  - **Meters** — typical for metric authors and VHF/UHF models (values like −13.356, 1.895)
  - The unit byte at header offset 0x09 and/or 0x41 likely encodes the system, but the exact mapping has not been fully confirmed.
- **Diameter vs radius** — EZNEC stores wire *diameter*; NEC uses *radius*.
  When converting to GW cards divide by 2: `GW_radius = EZ_diameter / 2`.
- **Tag numbers** — EZNEC numbers wires 1 to N.  The tag written by EZNEC to a NEC file equals the wire's sequential position, matching the record order.

After the Wire Array
--------------------

Immediately after the `N` wire records, the file contains blocks for:

### End-of-Wires Marker

The last wire record already carries `0xFF 0xFF` at its `+0x1C` offset. An additional `0xEC 0x45` or `EC xx` sequence has been observed shortly after the last wire record in some files and may act as a second-level sentinel.

### Source (EX) Records

At varying offsets (not yet formula-derived) there is a region containing:

| Approx offset from file start | Type       | Description |
|-------------------------------|------------|-------------|
| (after wires + padding)       | int16      | Source count |
| +0x00                         | int16      | Wire index (1-based) |
| +0x02                         | int16      | Segment number within wire |
| +0x04                         | float32    | Voltage magnitude (typically 1.0) |
| +0x08                         | float32    | Phase (degrees?) or imaginary part |
| (repeat 12 bytes per source)  |            |             |

### Load (LD) Records

Lumped loads are embedded within the per-wire record data starting around `+0x40` relative to each wire record. Structure not yet fully decoded; observed patterns include load type, segment position, and R/L/C values.

### Ground (GN) Parameters

Ground type is stored in header block 2 at offset 0x36.  Additional ground parameters (conductivity, dielectric constant) appear in a block following the source records; values 13.0 and 0.005 (EZNEC "Average" ground) and 0.0 (free space) have been observed as float32 LE pairs.

### Radiation Pattern (RP) Settings

Angular sweep parameters are stored at the tail of the file.  Two int16 values (elevation/azimuth step counts) and float32 start/increment pairs have been observed around offset 0x02A0 in single-wire files.

Unit System
-----------

The coordinate unit (feet vs. meters) is per-file. Determination method:

1. Check the title for explicit unit keywords ("ft", "m", "meters", etc.).
2. Cross-check computed wire length against a known formula:
   - At frequency f (Hz), free-space λ = 299,792,458 / f.
   - A resonant half-wave dipole has length ≈ 0.95 × λ / 2.
   - If computed wire length in the stored values divided by expected λ/2 is
     near 0.95 in **meters**, unit is meters; if near 0.95 in **feet**, unit is
     feet.
3. The byte at header 0x09 (`'A'` vs. `'E'`) and 0x41 (`'F'`/`'M'`/`'I'`) are
   candidate unit indicators, but their exact encoding remains uncertain.

When converting to NEC, emit a `GS` scale card if the NEC engine expects a different unit:

```
GS 0 0 0.3048    ' if EZ is in feet and NEC expects meters
GS 0 0 1.0       ' if EZ is already in meters (no scaling)
```

Mapping to NEC Cards
--------------------

| EZ field                        | NEC equivalent |
|---------------------------------|----------------|
| Wire endpoints + diameter       | `GW tag segs x1 y1 z1 x2 y2 z2 r` (r = diam/2) |
| Design frequency                | `FR 0 1 0 0 f` |
| Ground type `'F'`               | `GN -1` (free space) |
| Ground type `'R'`               | `GN 2 0 0 0 ε σ` (real / Sommerfeld) |
| Ground type `'P'`               | `GN 1` (perfect) |
| Source on wire w segment s      | `EX 0 w s 0 Vr Vi` |
| Load R,L,C on wire w segment s  | `LD 0 w s s R_Ω X L C` |

Example: Simple Dipole
----------------------

File `dblt8.ez` (742 bytes) — 3.5 MHz doublet, 8 wavelengths, free space:

```
Header:
  n_wires  = 1
  freq     = 3.5 MHz
  title    = "3.5 doublet 8 wl"
  gnd_type = 'F'  (free space)

Wire 0 record @ 0x00BA:
  x1 = −342.62 ft   y1 = 0.0 ft   z1 = 85.655 ft
  x2 = +342.62 ft   y2 = 0.0 ft   z2 = 85.655 ft
  diam = 0.004064 ft  → radius = 0.002032 ft
  segs = 161
```

Equivalent NEC (as exported by EZNEC):

```nec
CM 3.5 doublet 8 wl
CE
GW 1,161,-342.62,0.,85.655,342.62,0.,85.655,.002032
GE 0
FR 0,1,0,0,3.5
GN -1
EX 0,1,81,0,1.414214,0.
EN
```

File Statistics (Cebik collection)
----------------------------------

| Metric | Value |
|--------|-------|
| Total `.EZ`/`.ez` files examined | 5,478 |
| Smallest file | ~680 bytes (1 wire) |
| Most common size cluster | 924 – 1,530 bytes (1–6 wire simple antennas) |
| Largest file | ~419 KB (tri-corner reflector with 2461 wires) |
| Files with confirmed foot-unit coordinates | observed for HF models |
| Files with confirmed meter-unit coordinates | observed for many 60m+ models |

Known Unknowns
--------------

- Exact encoding of unit flag bytes (0x09 and 0x41).
- Full decode of the per-wire load sub-record (`+0x2E`–`+0xA9`).
- Source block location formula for multi-source models.
- Ground conductivity / permittivity storage location.
- Transmission line (TL card) representation.
- Differential (DL), networks (NT/TL/CP) representation.
- EZNEC Pro/4 vs. EZNEC+ vs. EZNEC v5 format differences (the `0x40` version byte may discriminate these).
- Non-ASCII (Cyrillic / multibyte) titles — some files in the collection have non-Latin titles that cause confusion in the title field.
