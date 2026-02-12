/**
 * @file input.h
 * @brief Deck input and high-level parsing functions.
 *
 * Provides the interface for reading NEC deck files into memory and 
 * performing initial structural parsing.
 */

#ifndef INPUT_H
#define INPUT_H

#include "types.h"
#include <stdio.h>

/** 
 * @brief Reads a full deck from a file into a deck_t structure.
 * 
 * This function reads the file line-by-line, creating a card_t for each line.
 * It handles the merging of split lines and captures comments.
 * 
 * @param ctx The simulation context.
 * @param deck The deck structure to populate.
 * @param pfile Open file pointer to read from.
 */
void read_deck(nec_context_t *ctx, deck_t *deck, FILE *pfile);

/**
 * @brief Performs structural analysis and field parsing on a deck.
 * 
 * Identifies the start and end of geometry and control sections,
 * and processes individual card fields for all cards in the deck.
 * 
 * @param ctx The simulation context.
 * @param deck The deck to parse.
 * @param errors List to populate with parsing errors or warnings.
 */
void parse_deck(nec_context_t *ctx, deck_t *deck, errors_list_t *errors);

/**
 * @brief Marks 4nec2-specific metadata cards to be ignored by the engine.
 * 
 * Used for compatibility with decks containing 4nec2 extensions that 
 * are not directly processed by the NEC2 core.
 * 
 * @param ctx The simulation context.
 * @param deck The deck to filter.
 */
void mark_4nec2_cards_invisible(nec_context_t *ctx, deck_t *deck);

#endif /* INPUT_H */
