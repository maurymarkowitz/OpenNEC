/**
 * @file output.h
 * @brief Output and report generation functions.
 *
 * Provides functions for exporting simulation results in traditional .out format
 * and saving modified decks in the OpenNEC format.
 */

#ifndef OUTPUT_H
#define OUTPUT_H

#include "types.h"
#include <stdio.h>

/**
 * @brief Generates the standard NEC-format simulation output report.
 * 
 * Writes detailed information including structure coordinates, currents,
 * gains, and radiation patterns to the provided file pointer.
 * 
 * @param ctx The simulation context.
 * @param deck The deck associated with the simulation.
 * @param pfile Output file pointer.
 */
void write_nec_output(nec_context_t *ctx, const deck_t *deck, FILE *pfile);

/**
 * @brief Saves the current deck state in OpenNEC format.
 * 
 * Maintains all formulas, symbols, and comments. Useful for saving 
 * user edits made in a GUI.
 * 
 * @param ctx The simulation context.
 * @param deck The deck to save.
 * @param pfile Output file pointer.
 */
void write_deck_onec(const nec_context_t *ctx, const deck_t *deck, FILE *pfile);

/**
 * @brief Writes the Green's function matrix to a file.
 * 
 * Used for persistent storage of the interaction matrix for Numerical
 * Green's Function (NGF) calculations.
 * 
 * @param file Output file pointer.
 * @param ctx The simulation context.
 * @param nrow Number of rows in the matrix.
 * @param cm The complex interaction matrix.
 */
void write_greens_matrix(FILE *file, const nec_context_t *ctx, int nrow, const complex double *cm);

#endif /* OUTPUT_H */
