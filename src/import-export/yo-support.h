/*
 * yo-support.h
 *
 * Public interface for Yagi Optimizer .yo/.ant/.yag import/export support.
 * Applications can use these functions to read a Yagi Optimizer file into
 * an OpenNEC deck_t structure, or to write an existing deck_t back out in
 * a simplified .yo-compatible text format.
 *
 * The importer handles all documented YO features: multi-frequency lines,
 * option lines (Height, material, Stacked, Dual), taper definitions, and
 * both absolute and relative (spacing) element positioning.  The exporter
 * writes a minimal subset sufficient to round-trip simple GW/FR decks.
 */

#ifndef YO_SUPPORT_H
#define YO_SUPPORT_H

#include "types.h"
#include <stdio.h>

/**
 * @brief Import a Yagi Optimizer file into an OpenNEC deck.
 *
 * Parses a YO-format text file and appends the corresponding NEC cards to
 * the supplied deck.  On success, the deck will contain CM, CE, GW, GS, GE,
 * GM (if stacked), LD (if a material was defined), FR, and EN cards.
 *
 * The element axis is X, the boom axis is Z, and the height offset (from a
 * Height option line) is placed on the Y axis.  All GW coordinates retain
 * the original measurement unit from the file; a GS scale card is appended
 * immediately before GE to convert them to metres for NEC.
 *
 * @param deck Destination deck.  Must be initialised (zeroed) by the caller.
 *             Cards are appended using insert_card(), so existing cards are
 *             left intact.
 * @param fp   Open, rewound file handle for reading.
 * @return 0 on success, -1 on I/O or parse error.
 */
int read_deck_yo(deck_t *deck, FILE *fp);

/**
 * @brief Export an OpenNEC deck as a Yagi Optimizer .yo file.
 *
 * Inspects the deck for GW wires and FR frequency cards and emits a minimal
 * YO-format text representation.  The exporter assumes elements are oriented
 * along the X axis and the boom runs along Z; other orientations will produce
 * geometrically incorrect output.
 *
 * @param deck Source deck containing at least one GW and one FR card.
 * @param fp   Open file handle for writing.
 * @return 0 on success, -1 if either argument is NULL.
 */
int write_deck_yo(const deck_t *deck, FILE *fp);

#endif /* YO_SUPPORT_H */
