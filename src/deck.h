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
bool is_comment(const card_t *card);
bool is_geometry(const card_t *card);
bool is_control(const card_t *card);
bool is_extension(const card_t *card);

/* Card field counts */
int min_int_fields(const card_t *card);
int max_int_fields(const card_t *card);
int min_flt_fields(const card_t *card);
int max_flt_fields(const card_t *card);

/* Deck lifecycle */
void free_deck(deck_t *deck);
void update_deck_values(nec_context_t *ctx, deck_t *deck);
void initialize_symbol_table(deck_t *deck, errors_list_t *errors);
void evaluate_formula(nec_context_t *ctx, key_value_t *formula, deck_t *deck, errors_list_t *errors);
void evaluate_symbols_in_comments(nec_context_t *ctx, deck_t *deck, errors_list_t *errors);

/* Cross-module deck functions */
void add_key_value(const card_t *card, key_value_t **list, char *key, char *value, char separator);
const char* lookup_formula(const card_t *card, const char *key);

#endif /* DECK_H */
