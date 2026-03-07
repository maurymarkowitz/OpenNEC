/**
 * @file nec2-support.h
 *
 * Utilities for working with raw NEC-2 decks.  This module is intentionally
 * lightweight; the core OpenNEC parser (`input.c`, `deck.c`, etc.) already
 * handles the vast majority of NEC-2 syntax.  The functions exposed here are
 * provided as a stable place for any future NEC-2–specific helpers that do not
 * naturally belong in the general parser.
 */

#ifndef NEC2_SUPPORT_H
#define NEC2_SUPPORT_H

#include "types.h"
#include <stdio.h>

/**
 * @brief Read a NEC-2 deck from a file and append the cards to `deck`.
 *
 * This is a simple wrapper around the existing `read_deck` implementation
 * that is guaranteed to work only for NEC-2 syntax.  Returns 0 on success or
 * -1 on error (e.g. NULL arguments).
 * 
 * @param deck The deck to read.
 * @param file Input file pointer.
*/
int read_deck_nec2(deck_t *deck, FILE *fp);

/**
 * @brief Write the contents of `deck` to `fp` in NEC-2 format.
 *
 * Writes a deck in the original NEC-2 format. This strips out any
 * extensions like SY, replaces formulas and variables with their
 * numeric values, and strips out any inline or in-deck
 * comments.
 * 
 * @param deck The deck to write.
 * @param file Output file pointer.
*/
int write_deck_nec2(const deck_t *deck, FILE *fp);  /* writes NEC-2 deck, inline comments removed */

#endif /* NEC2_SUPPORT_H */


