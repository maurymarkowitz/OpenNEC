/*
 * mma-support.h
 *
 * Public interface for MMANA-GAL .maa import/export support.  Applications
 * can use these functions to read antenna geometry from a .maa file into an
 * OpenNEC deck_t, or to write an existing deck_t back out in .maa
 * format.  The routines are simple wrappers around the normal deck APIs and
 * perform only minimal validation.
 */

#ifndef MMA_SUPPORT_H
#define MMA_SUPPORT_H

#include "types.h"
#include <stdio.h>

/**
 * @brief Export an OpenNEC deck as an MMANA-GAL .maa file.
 *
 * The exporter looks for GW/LD/EX/FR cards in the supplied deck and emits
 * the equivalent lines that MMANA-GAL expects.  The output is very
 * simplistic; exotic load types or excitation conventions are not handled.
 *
 * @param deck Source deck containing geometry, loads and excitations.
 * @param fp   Open file handle to write the .maa data.  Must be writable.
 * @return 0 on success, -1 on error (typically a NULL pointer).
 */
/**
 * @brief Export a deck to MMANA-GAL .maa format.
 *
 * The function inspects the provided NEC deck for wire geometry (GW cards),
 * lumped loads (LD) and excitations (EX), and emits a sequence of text
 * lines that conform to the informal .maa specification used by MMANA-GAL.
 *
 * Only a subset of possible NEC cards is translated; any other cards in the
 * deck are ignored.  The produced file begins with a title line, frequency
 * line, counts line, followed by wire/load/source sections and a trivial
 * ground block.
 *
 * @param deck Pointer to the source deck.  Must not be NULL.
 * @param fp   File pointer already opened for writing.  The caller retains
 *             responsibility for opening/closing the handle.
 * @return 0 on success, -1 if either argument is NULL or a write error occurs.
 */
int write_deck_maa(const deck_t *deck, FILE *fp);

/**
 * @brief Import an MMANA-GAL .maa file into an OpenNEC deck.
 *
 * This function parses the textual .maa format and appends the corresponding
 * GW/LD/EX/FR cards to the supplied deck.  The caller is responsible for
 * initializing the deck (e.g. zeroing it) before the call.  Existing cards
 * in the deck will be left untouched; the MMANA data is tacked on at the
 * end.
 *
 * @param deck Destination deck to populate.  Cards are appended using
 *             insert_card(), so deck pointers must be valid.
 * @param fp   Open file handle for reading.  Must be readable and rewinded.
 * @return 0 on success, -1 on parse failure or I/O error.
 */
/**
 * @brief Import MMANA-GAL .maa data into an OpenNEC deck.
 *
 * The parser reads line-oriented .maa contents from the provided file
 * pointer.  It generates corresponding GW, LD, EX and FR cards and appends
 * them to the supplied deck using the normal deck manipulation APIs.  The
 * function does not clear or otherwise modify existing cards already present
 * in the deck.
 *
 * The routine is tolerant of minor formatting variations (commas vs.
 * spaces) and will skip invalid entries, but it will fail if the counts line
 * cannot be located.
 *
 * @param deck Pointer to an initialized deck_t structure; cards are appended.
 * @param fp   File pointer opened for reading.  Must be positioned at the
 *             start of the .maa content.
 * @return 0 on success, -1 on null arguments or I/O/parse errors.
 */
int read_deck_maa(deck_t *deck, FILE *fp);

#endif // MMA_SUPPORT_H
