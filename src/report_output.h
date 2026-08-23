/**
 * @file report_output.h
 * @brief Output and report generation functions.
 *
 * Provides functions for exporting simulation results in traditional .out format
 * and saving modified decks in the OpenNEC format.
 */

#ifndef REPORT_OUTPUT_H
#define REPORT_OUTPUT_H

#include "opennec.h"
#include "types.h"
#include <stdio.h>

/* Forward declarations for functions called by write_nec_preamble and write_segments */
void write_header(const context_t *ctx, const deck_t *deck, FILE *file);
void write_comments(const context_t *ctx, const deck_t *deck, FILE *file);
int write_structure(context_t *ctx, const deck_t *deck, FILE *file);
int write_segments(context_t *ctx, const deck_t *deck, FILE *file);
void write_patches(const context_t *ctx, const deck_t *deck, FILE *file);
void write_frequency_data(FILE *file, const context_t *ctx);
void write_frequency_step_output(FILE *file, context_t *ctx);
void write_loading_data(FILE *file, const context_t *ctx);
void write_environment_data(FILE *file, const context_t *ctx);
void write_matrix_timing(FILE *file, const context_t *ctx);
void write_network_data(FILE *file, const context_t *ctx);
void write_matrix_asymmetry(FILE *file, const context_t *ctx);
void write_network_excitation(FILE *file, const context_t *ctx);
void write_antenna_input_parameters(FILE *file, const context_t *ctx);
void write_coupling_data(context_t *ctx);
void write_remaining_execution_cards(FILE *file, const context_t *ctx, const deck_t *deck);
void write_currents(FILE *file, const context_t *ctx);
void write_patch_currents(FILE *file, const context_t *ctx);
void write_power_budget(FILE *file, const context_t *ctx);
void write_radiation_pattern_header(FILE *file, const context_t *ctx);
void write_radiation_pattern_data(FILE *file, const context_t *ctx);
void write_average_power_gain(FILE *file, const context_t *ctx);
void write_normalized_gain(FILE *file, const context_t *ctx);
void write_near_field_data(FILE *file, const context_t *ctx);
void write_near_field_plot(const context_t *ctx);

/**
 * @brief Writes the output file header.
 *
 * Outputs the title, structure specification, and initial sections.
 * Called once per simulation before the frequency loop.
 *
 * @param ctx   The simulation context.
 * @param deck  The deck associated with the simulation.
 * @param file  Output file pointer.
 */
void write_header(const context_t *ctx, const deck_t *deck, FILE *file);

/**
 * @brief Writes the comments section.
 *
 * Outputs any comment cards from the deck. Includes section header
 * and individual comment lines.
 *
 * @param ctx   The simulation context.
 * @param deck  The deck associated with the simulation.
 * @param file  Output file pointer.
 */
void write_comments(const context_t *ctx, const deck_t *deck, FILE *file);

/**
 * @brief Writes the one-time geometry preamble section.
 *
 * Writes the file header, structure specification, segments, patches, and
 * input card listing.  Called once per simulation section before the
 * frequency loop begins.
 */
void write_nec_preamble(context_t *ctx, const deck_t *deck, FILE *file);

/**
 * @brief Writes the NEC header.
 *
 */
void write_header(const context_t *ctx, const deck_t *deck, FILE *pfile);

/**
 * @brief Writes the comments header and cards.
 *
 */
void write_comments(const context_t *ctx, const deck_t *deck, FILE *pfile);

/**
 * @brief Echos the geometry cards.
 *
 */
int write_structure(context_t *ctx, const deck_t *deck, FILE *pfile);

/**
 * @brief Writes the segment data after geometry is calculated.
 *
 */
int write_segments(context_t *ctx, const deck_t *deck, FILE *pfile);

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
 * DEPRECATED - BATCH MODE ONLY: Used by execute_extra_patterns() when a
 * second RP/NE/NH card follows with no new FR card — mirrors nec2c's
 * igo==4→5→6 path that skips the frequency header, loading, matrix timing,
 * and power budget and jumps straight to the pattern computation.
 *
 * This function is only called from control.c (batch mode). The active
 * sequential processing pathway in reporting.c does not use this.
 */
void write_extra_pattern_output(FILE *file, context_t *ctx);

/**
 * @brief Writes only the excitation output section (no frequency header).
 *
 * DEPRECATED - BATCH MODE ONLY: Used when processing a subsequent EX card at
 * the same frequency — skips the frequency header and loading sections,
 * jumping straight to antenna input, currents, power budget, and radiation
 * patterns. This mirrors Fortran behavior where the frequency header is
 * output only once per unique frequency.
 *
 * This function is only called from control.c (batch mode). The active
 * sequential processing pathway in reporting.c does not use this.
 */
void write_subsequent_excitation_output(FILE *file, context_t *ctx, const deck_t *deck);

/**
 * @brief Writes a single radiation pattern header and data.
 *
 * Called from sequential processing (reporting.c) when outputting multiple
 * radiation patterns per frequency. Outputs the pattern header and data for
 * the current radiation pattern in ctx->rpat.
 *
 * @param file Output file pointer.
 * @param ctx  The simulation context (provides rpat data).
 */
void write_single_radiation_pattern(FILE *file, context_t *ctx);

/**
 * @brief Echoes the current batch's control cards before its frequency output.
 *
 * DEPRECATED - BATCH MODE ONLY: Writes the DATA CARD echo lines for
 * ctx->batch_start_card through ctx->batch_end_card with correct sequential
 * numbering, accounting for all previously echoed batches.  Called before
 * each non-first batch's frequency output to mirror Fortran behavior: each
 * FR/RP pair is echoed immediately before its own output section rather
 * than all cards appearing at the top.
 *
 * This function is only called from control.c (batch mode). The active
 * sequential processing pathway in reporting.c does not use this.
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
void write_end_cards(FILE *file, const context_t *ctx, const deck_t *deck);

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



#endif // REPORT_OUTPUT_H