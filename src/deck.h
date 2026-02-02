/*
 * deck.h - Deck and card utilities for OpenNEC
 * 
 * Card type checking and deck lifecycle functions
 * used across input, output, control, and test modules.
 */

#ifndef DECK_H
#define DECK_H

#include "types.h"

/* Card type checking - heavily used across modules */
int is_comment(const card_t *card);
int is_geometry(const card_t *card);
int is_control(const card_t *card);
int is_extension(const card_t *card);

/* Card field counts */
int min_int_fields(const card_t *card);
int max_int_fields(const card_t *card);
int min_flt_fields(const card_t *card);
int max_flt_fields(const card_t *card);

/* Deck lifecycle */
void free_deck(deck_t *deck);
void update_deck_values(deck_t *deck);

/* Cross-module deck functions */
void add_key_value(const card_t *card, key_value_t **list, char *key, char *value, char separator);

#endif /* DECK_H */
