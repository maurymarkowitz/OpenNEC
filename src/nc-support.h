/*
 * nc-support.h
 *
 * Public interface for cocoaNEC .nc file import/export support.
 *
 * OpenNEC can both export (`write_deck_nc`) and import (`read_deck_nc`) the
 * cocoaNEC `.nc` scripting language.  The importer accepts a practical subset
 * of cocoaNEC sufficient to round-trip the example files included with the
 * project:
 *   - unit suffixes: inches ("), feet ('), and AWG (#N) are recognised when
 *     parsing. Inches and feet are expanded to metres (e.g. `48"` →
 *     `48*0.0254`); AWG markers are accepted in geometry arguments and are
 *     converted to wire radii when emitting `GW` cards.
 *   - symbol declarations and assignments (`real` / `int` / `SY`) are parsed
 *     into SY cards.
 *   - geometry and inline wires (`wire()`, `line()`, `taperedWire()`),
 *     element assignments and inline `wire()` in feed calls are supported.
 *   - excitations (`voltageFeed` / `currentFeed`), loads, frequencies (FR),
 *     ground helpers and single-cut RP requests are supported.
 *   - `control()` blocks and most user-defined functions are ignored.
 *
 * The exporter emits a minimal cocoaNEC `model("name") { ... }` program that
 * reproduces the deck's geometry, symbols, feeds, loads, frequencies and ground
 * choice.  Some advanced NC-only constructs are intentionally skipped.
 */

#ifndef NC_SUPPORT_H
#define NC_SUPPORT_H

#include "types.h"
#include <stdio.h>

/**
 * @brief Export an OpenNEC deck as a cocoaNEC .nc file.
 *
 * Translates NEC cards to cocoaNEC scripting language constructs:
 *   SY real variable declarations and assignments
 *   GW wire() calls (assigned to element vars for fed/loaded wires)
 *   EX voltageFeed() or currentFeed()
 *   LD impedanceLoad()
 *   FR setFrequency() + addFrequency() for additional frequencies
 *   GN freespace(), averageGround(), goodGround(), saltWaterGround(),
 *          perfectGround(), or useSommerfeldGround()
 *   RP azimuthPlotForElevationAngle() or elevationPlotForAzimuthAngle()
 *         for single-cut patterns; full-sphere RP cards are skipped.
 *
 * Cards with no NC equivalent (GE, EN, GS, GH, GM, TL, NT, NX, …) are
 * silently ignored.  The output is a single model(){} block with no
 * control() block.
 *
 * Note: coordinates and radii are written as-is (metres).  No unit-suffix
 * conversion (feet, inches, AWG) is performed by the exporter; the importer
 * performs unit-suffix expansion when reading `.nc` files.
 *
 * @param deck  Source deck.  Should have i[] and f[] arrays populated (i.e.
 *              loaded via the main input path, not built from raw text).
 * @param fp    Open file handle for writing.
 * @return 0 on success, -1 if either argument is NULL.
 */
int write_deck_nc(const deck_t *deck, FILE *fp);

/**
 * @brief Import a cocoaNEC .nc file into an OpenNEC deck.
 *
 * Parses a cocoaNEC `.nc` program and appends the equivalent NEC cards to
 * `deck` (CM/CE, SY, GW, GE, EX, LD, FR, GN, RP, EN).  The importer handles
 * common cocoaNEC idioms such as unit suffixes, inline `wire()` calls inside
 * `voltageFeed(...)`, and symbol expressions.  `control()` blocks are
 * skipped.  The function returns 0 on success and -1 on error.
 *
 * @param deck  Destination deck (must be non-NULL).
 * @param fp    Open file handle for reading.
 * @return 0 on success, -1 on error.
 */
int read_deck_nc(deck_t *deck, FILE *fp);

#endif /* NC_SUPPORT_H */
