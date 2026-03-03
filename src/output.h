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
 * @brief Writes a deck in the original NEC-2 format.
 *
 * Extension cards (SY etc.) are omitted.  Formulas are emitted as their
 * original text (not yet evaluated).  Set remove_inline_comments=1 to
 * produce a file compatible with the original Fortran NEC-2.
 *
 * @param ctx The simulation context.
 * @param deck The deck to write.
 * @param file Output file pointer.
 * @param remove_inline_comments 1 to strip inline '!' comments, 0 to keep.
 */
void write_deck_nec2(const nec_context_t *ctx, const deck_t *deck, FILE *file, int remove_inline_comments);

/**
 * @brief Saves the current deck state in NEC-4 format. (TODO: stub)
 *
 * @param ctx The simulation context.
 * @param deck The deck to write.
 * @param file Output file pointer.
 */
void write_deck_nec4(const nec_context_t *ctx, const deck_t *deck, FILE *file);

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
 * @brief Writes an OpenNEC binary NGF (Green's function) file.
 *
 * Stores all geometry and the unfactored CM interaction matrix so that a
 * subsequent simulation run using a GF card can restore them without
 * recomputing. Called by the WG control-card handler after cmset().
 *
 * @param file  Output file pointer (must be opened in binary mode).
 * @param ctx   The simulation context.
 * @param neq   Matrix dimension (number of equations).
 * @param cm    Unfactored CM matrix, column-major, neq×neq complex doubles.
 * @return      true on success, false on I/O error.
 */
bool write_greens_binary(FILE *file, const nec_context_t *ctx,
                          int neq, const complex double *cm);

/**
 * @brief Reads an OpenNEC binary NGF (Green's function) file.
 *
 * Populates ctx->geometry with stored segment data and installs the cached
 * CM matrix in ctx->ngf_cm. Sets ctx->has_ngf on success.
 * Called by the GF geometry-card handler.
 *
 * @param file  Input file pointer (must be opened in binary mode).
 * @param ctx   The simulation context to populate.
 * @return      true on success, false on format/I/O error.
 */
bool read_greens_binary(FILE *file, nec_context_t *ctx);

#endif /* OUTPUT_H */
