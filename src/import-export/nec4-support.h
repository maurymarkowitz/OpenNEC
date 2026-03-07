/**
 * @file nec4-support.h
 *
 * Helpers for reading and writing NEC-4 decks.  The core OpenNEC code already
 * understands NEC-2/NEC-4 syntax via the generic parser, but having a dedicated
 * support module makes it easier to expose NEC-4-specific helpers in the future
 * and keeps external code from depending on `input.c` / `output.c` internals.
 *
 * The functions here are intentionally lightweight wrappers; currently they
 * simply delegate to the existing `read_deck`/`write_deck_nec4` implementations
 * in the main codebase.
 */

#ifndef NEC4_SUPPORT_H
#define NEC4_SUPPORT_H

#include "types.h"
#include <stdio.h>

/**
 * Read a NEC-4 deck from the given file into `deck`.
 * Returns 0 on success, -1 on error (e.g. null arguments).
 */
int read_deck_nec4(deck_t *deck, FILE *fp);

/**
 * Write `deck` to `fp` in NEC-4 syntax.  Inline comments are removed.
 * Returns 0 on success or -1 on error.
 */
int write_deck_nec4(const deck_t *deck, FILE *fp);

#endif /* NEC4_SUPPORT_H */
