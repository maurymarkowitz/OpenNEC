/*
 * input.h - Deck input and parsing functions for OpenNEC
 * 
 * Public interface for reading and parsing NEC deck files.
 */

#ifndef INPUT_H
#define INPUT_H

#include "types.h"
#include <stdio.h>

/* Deck reading and parsing - called from main.c and batch tester */
void read_deck(nec_context_t *ctx, deck_t *deck, FILE *pfile);
void parse_deck(nec_context_t *ctx, deck_t *deck, errors_list_t *errors);

/* 4nec2 compatibility */
void mark_4nec2_cards_invisible(nec_context_t *ctx, deck_t *deck);

#endif /* INPUT_H */
