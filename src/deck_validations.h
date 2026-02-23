/*
 * deck_validations.h - Deck validation functions for OpenNEC
 * 
 * Public interface for deck structure and content validation.
 */

#ifndef DECK_VALIDATIONS_H
#define DECK_VALIDATIONS_H

#include "types.h"

/* Validation functions - called from main.c and batch tester */
void test_deck_structure(const nec_context_t *ctx, const deck_t *deck, errors_list_t *errors);
void test_duplicate_tags(const nec_context_t *ctx, const deck_t *deck, errors_list_t *errors);
void test_card_inputs(const nec_context_t *ctx, const deck_t *deck, errors_list_t *errors);
void test_field_separators(const nec_context_t *ctx, const deck_t *deck, errors_list_t *errors);

#endif /* DECK_VALIDATIONS_H */
