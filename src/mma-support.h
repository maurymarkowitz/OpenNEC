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
int read_deck_maa(deck_t *deck, FILE *fp);

#endif // MMA_SUPPORT_H
