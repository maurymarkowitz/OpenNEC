/*
 * nc-support.h
 *
 * Public interface for cocoaNEC .nc file import/export support.
 * write_deck_nc() converts an OpenNEC deck_t to the cocoaNEC .nc scripting
 * language.  read_deck_nc() is not yet implemented.
 */

#ifndef NC_SUPPORT_H
#define NC_SUPPORT_H

#include "types.h"
#include <stdio.h>

/**
 * @brief Export an OpenNEC deck as a cocoaNEC .nc file.
 *
 * Translates NEC cards to cocoaNEC scripting language constructs:
 *   SY  → real variable declarations and assignments
 *   GW  → wire() calls (assigned to element vars for fed/loaded wires)
 *   EX  → voltageFeed() or currentFeed()
 *   LD  → impedanceLoad()
 *   FR  → setFrequency() + addFrequency() for additional frequencies
 *   GN  → freespace(), averageGround(), goodGround(), saltWaterGround(),
 *          perfectGround(), or useSommerfeldGround()
 *   RP  → azimuthPlotForElevationAngle() or elevationPlotForAzimuthAngle()
 *         for single-cut patterns; full-sphere RP cards are skipped.
 *
 * Cards with no NC equivalent (GE, EN, GS, GH, GM, TL, NT, NX, …) are
 * silently ignored.  The output is a single model(){} block with no
 * control() block.
 *
 * Note: coordinates and radii are written as-is (metres).  No unit-suffix
 * conversion (feet, inches, AWG) is performed.
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
 * Not yet implemented.  Always returns -1.
 *
 * @param deck  Destination deck.
 * @param fp    Open file handle for reading.
 * @return -1 (not implemented).
 */
int read_deck_nc(deck_t *deck, FILE *fp);

#endif /* NC_SUPPORT_H */
