/*
 * output.h - Output functions for OpenNEC
 * 
 * Public interface for writing deck and simulation output.
 */

#ifndef OUTPUT_H
#define OUTPUT_H

#include "types.h"
#include <stdio.h>

/* Main output functions - called from main.c */
void write_nec_output(nec_context_t *ctx, deck_t *deck, FILE *pfile);
void write_deck_onec(nec_context_t *ctx, deck_t *deck, FILE *pfile);

/* Internal output functions used by control.c */
void write_greens_matrix(FILE *file, nec_context_t *ctx, int nrow, complex double *cm);

#endif /* OUTPUT_H */
