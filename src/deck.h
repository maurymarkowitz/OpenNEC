/*
 * deck.h - Deck and card utilities for OpenNEC
 *
 * Card type checking and deck lifecycle functions
 * used across input, output, control, and test modules.
 */

#ifndef DECK_H
#define DECK_H

#include "types.h"

/* Card insertion, removal, and reordering */
int insert_card(deck_t *deck, card_t *card, int location);
int move_card(deck_t *deck, int src, int dst);
int remove_card(deck_t *deck, int location);

/* Card enable/disable (comment out / uncomment) for GUI toggling */
bool card_is_toggleable(const card_t *card);
void card_disable(deck_t *deck, card_t *card);
void card_enable(deck_t *deck, card_t *card);

/**
 * card_is_invisible - card is annotated invisible (ignore=true, no leading marker).
 * Geometry IS generated but goes to ignored_geometry rather than live geometry.
 * The card remains visible to the GUI and can be toggled back on.
 */
static inline bool card_is_invisible(const card_t *card)
{
  return card->ignore && card->cmt_code[0] == '\0';
}

/**
 * card_is_commented_out - card has a leading comment marker (e.g. '!', '\'', '#').
 * Entirely skipped during geometry and calculation — no geometry is generated at all.
 */
static inline bool card_is_commented_out(const card_t *card)
{
  return card->ignore && card->cmt_code[0] != '\0';
}

/* Card type checking - heavily used across modules */
bool is_comment(const card_t *card);
bool is_geometry(const card_t *card);
bool is_control(const card_t *card);
bool is_extension(const card_t *card);

/* Return true if this card type assigns an ITG tag to generated segments */
bool card_has_itag(const card_t *card);

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
const char *lookup_formula(const card_t *card, const char *key);

#endif /* DECK_H */
