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
void write_nec_output(context_t *ctx, const deck_t *deck, FILE *pfile);

/**
 * @brief Writes the one-time geometry preamble section.
 *
 * Writes the file header, structure specification, segments, patches, and
 * input card listing.  Called once per simulation section before the
 * frequency loop begins.
 */
void write_nec_preamble(context_t *ctx, const deck_t *deck, FILE *file);

/**
 * @brief Writes all per-frequency-step output sections.
 *
 * Writes frequency data, loading, environment, matrix timing, network data,
 * antenna input parameters, currents, power budget, radiation patterns, and
 * near-field data for the current frequency step.  Called at the end of each
 * frequency iteration inside execute_frequency_loop().
 */
void write_frequency_step_output(FILE *file, context_t *ctx);

/**
 * @brief Writes only the radiation-pattern (or near-field) output section.
 *
 * Used by execute_extra_patterns() when a second RP/NE/NH card follows with
 * no new FR card — mirrors nec2c's igo==4→5→6 path that skips the frequency
 * header, loading, matrix timing, and power budget and jumps straight to
 * the pattern computation.
 */
void write_extra_pattern_output(FILE *file, context_t *ctx);

/**
 * @brief Writes only the excitation output section (no frequency header).
 *
 * Used when processing a subsequent EX card at the same frequency — skips
 * the frequency header and loading sections, jumping straight to antenna
 * input, currents, power budget, and radiation patterns. This mirrors
 * Fortran behavior where the frequency header is output only once per
 * unique frequency.
 */
void write_subsequent_excitation_output(FILE *file, context_t *ctx);

/**
 * @brief Echoes the current batch's control cards before its frequency output.
 *
 * Writes the DATA CARD echo lines for ctx->batch_start_card through
 * ctx->batch_end_card with correct sequential numbering, accounting for all
 * previously echoed batches.  Called before each non-first batch's frequency
 * output to mirror Fortran behavior: each FR/RP pair is echoed immediately
 * before its own output section rather than all cards appearing at the top.
 *
 * @param file Output file pointer.
 * @param ctx  The simulation context (provides batch_start_card/batch_end_card).
 * @param deck The deck associated with the simulation.
 */
void write_batch_card_echo(FILE *file, const context_t *ctx, const deck_t *deck);

/**
 * @brief Writes the EN and NX end cards as separate batches.
 *
 * Outputs EN and NX cards as individual batches with their own DATA CARD No:
 * lines, appearing near the end of the output file before the footer.
 *
 * @param file Output file pointer.
 * @param deck The deck associated with the simulation.
 */
void write_end_cards(FILE *file, const deck_t *deck);

/**
 * @brief Writes the output file footer.
 *
 * Outputs timing and summary information at the end of the output file.
 * Called after all frequency and pattern data has been written.
 *
 * @param file Output file pointer.
 * @param ctx The simulation context.
 * @param deck The deck associated with the simulation.
 */
void write_footer(FILE *file, const context_t *ctx, const deck_t *deck);


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
void write_deck_onec(const context_t *ctx, const deck_t *deck, FILE *pfile);

/**
 * @brief Writes an OpenNEC binary NGF (Green's function) file.
 *
 * Stores all geometry and the unfactored CM interaction matrix so that a
 * subsequent simulation run using a GF card can restore them without
 * recomputing. Called by the WG control-card handler after fill_interaction_matrix().
 *
 * @param file  Output file pointer (must be opened in binary mode).
 * @param ctx   The simulation context.
 * @param neq   Matrix dimension (number of equations).
 * @param cm    Unfactored CM matrix, column-major, neq×neq complex doubles.
 * @return      true on success, false on I/O error.
 */
bool write_greens_binary(FILE *file, const context_t *ctx,
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
bool read_greens_binary(FILE *file, context_t *ctx);

#endif /* OUTPUT_H */
