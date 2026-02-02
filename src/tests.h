/*
 * tests.h - Deck validation functions for OpenNEC
 * 
 * Public interface for deck structure and content validation.
 */

#ifndef TESTS_H
#define TESTS_H

#include "types.h"

/* Validation functions - called from main.c and batch tester */
void test_deck_structure(nec_context_t *ctx, deck_t *deck, errors_list_t *errors);
void test_duplicate_tags(nec_context_t *ctx, deck_t *deck, errors_list_t *errors);
void test_card_inputs(nec_context_t *ctx, deck_t *deck, errors_list_t *errors);

#endif /* TESTS_H */
