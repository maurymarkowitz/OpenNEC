OpenNEC validation tests
========================

This document summarizes the validation and guidance checks implemented in OpenNEC to help authors catch common deck issues early. These checks run in the tests-only mode and during normal processing, reporting warnings vs. error conditions with different severity codes.

## Severity levels
- 0: Warning — calculation may proceed, but the condition is discouraged.
- 1: Error — likely to cause calculation failure or incorrect results; fix strongly recommended.
- 2: Critical — deck cannot be processed (e.g., missing required data).

## Structural checks
- CE presence: Warn if no `CE` is present.
- Geometry presence: Require at least one geometry card (`Gx` such as `GW`).
- GE presence and uniqueness: Require a single `GE`; warn on duplicates.
- Geometry after GE: Warn if any geometry card appears after `GE`.
- Control placement before GE: Warn when `EX`, `TL`, `LD`, `FR`, `RP`, `GN`, or `GD` appear before `GE`.
- EK placement: Specific warning if `EK` appears before `GE`.
- RP after FR: Warn if `RP` is used before an `FR`.
- GF duplication and placement: Warn on duplicate `GF`; `GF` should follow `CE` or `SY`.
- GC→GW order: `GC` must follow `GW`.
- GN↔GD pairing: `GD` must follow `GN`; conversely, `GN` should be followed immediately by `GD`.
- Geometry modifiers: `GM`, `GR`, `GX` must follow `GA`, `GH`, `GW`, `SP`, or `CW`.
- SC placement: `SC` must follow `SP`, `SM`, or another `SC`.
- SM placement and immediate follower: `SM` must follow `SP` or `SM`, and must be immediately followed by `SC`.
- SC chain ancestry: If `SC` follows `SC`, ensure an earlier `SP` or `SM` exists before the chain; warn otherwise.
- SY and CE relation: Warn if `SY` exists but no `CE`.
- EN presence and position: Warn if `EN` missing; when present, it should be the last card.
- EX/LD presence: Warn if neither `EX` nor `LD` is present.
- GD without GN: Warn if `GD` appears but no preceding `GN`.
- GE -1 requires GN: Warn if `GE I1=-1` is used without a `GN`.

## Card input checks
- FR:
  - Base frequency required in `F1`.
  - Single-frequency: `I2=0` implies `F2=0`.
  - Stepped: `I2>0` requires positive `F2` (step).
  - Angle ranges and steps: validate start ranges, non-zero steps when counts > 1, end angles within sensible bounds, and step direction consistency.
- GW:
  - Require tag and positive segment count.
  - Endpoints must differ (non-zero length).
  - Radius required in `F7` and must be positive.
  - If radius is zero, the next card must be `GC`.
- TL:
  - Require 4 integer locators (tag/segment for both ends) and non-zero `Z0` in `F1`.
  - Locator sanity: all tag/segment locators must be positive.
  - Tag existence: both endpoint tags must exist in prior geometry.
  - Segment bounds: each endpoint segment index must be within its wire’s segment count.
  - Self-loop: warn if both ends reference the same tag and segment.
- EX:
  - Require sufficient integer inputs and non-zero amplitude (`F1`).
  - Locator sanity: positive tag/segment.
  - Tag existence check against prior geometry.
  - Open-end placement: warn if placed on segment 1 or last segment of a wire with more than one segment when that end is not connected.
- LD:
  - Require type, tag, segment(s) and at least one non-zero load value.
  - Type and locator sanity (non-negative where applicable; positive tag/segment).
  - Segment range order: warn if `I4 < I3` (unless `I4=0` indicating single segment).
  - Tag existence check against prior geometry.
  - Open-end placement: warn when load starts or ends at an open wire end.
  - Segment bounds: validate start/end segments against the wire’s segment count.

## Geometry and connectivity checks
- Open-end EX/LD placement: Ensure sources/loads are not at open wire ends.
- Mid-segment connectivity: Warn if a wire endpoint connects near the middle of another wire; connections must be at segment ends (footpoint tolerance ≈ `len/1000`).
- Overlapping wires: Warn if any two wires have identical endpoints (same or reversed).
- Parallel wires: Warn when parallel wires closer than `0.05` wavelengths have different segmentation (counts or lengths differing by >10%).
- Segment length vs. wavelength (WL):
  - `>= 0.1 WL`: warn.
  - `>= 0.05 WL`: advisory (critical regions should be < `0.05 WL`).
  - `< 0.001 WL`: advisory (excessive segmentation).
- Radius guidance (thin-wire assumptions):
  - Default kernel: warn if `radius >= len/2`; advise if `radius >= len/10`.
  - Extended kernel (`EK`): warn if `radius >= 2*len`.
- Ground interactions:
  - When ground is enabled (`GE I1 ∈ {1,2}` or any `GN`): warn if wires lie below `z=0` or cross the ground plane.
- GE low-height hazard:
  - With `GE I1=1`, warn when horizontal wire height `< 1e-3 x segment length` (ends may connect to ground); suggest `GE -1`.
- Junction segmentation consistency:
  - Warn when connected wire endpoints have segment lengths differing by > 20%.

## Kernel-specific checks (EK)
- EK placement: warn if `EK` appears before `GE`.
- EK radius guidance: when enabled (`I1 != 0`), apply extended kernel radius thresholds (see above).

## Notes
- Most checks are conservative and aim to help produce robust decks; some advisories may be acceptable depending on the design.
- Tests can be run via the `-t -n` flags to validate decks without simulation.
