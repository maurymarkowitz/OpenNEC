/*
 * reporting.c - reporting loop implementation for OpenNEC
 */

#include "report_logic.h"
#include "report_output.h"
#include "internals.h"
#include "control.h"
#include "calculations.h"
#include "geometry.h"
#include "deck.h"
#include "misc.h"
#include "matrix.h"
#include "network.h"
#include "output.h"
#include <string.h>
#include <math.h>
#include <time.h>

/* ============================================================================
 * Main Processing Loop
 * ========================================================================== */
int run_report(context_t *ctx, deck_t *deck)
{
    int iflow = 0;

    // check that we have a deck and a context
    if (!ctx || !deck) return -1;

    // and that the context has a valid output file
    if (!ctx->output_fp) return -1;

    // and the deck has at least one card
    if (deck->num_cards <= 0) return -1;

    // and now we start looping over the deck
    int card_idx = 0;
    char card_code[2] = "";
    do {
        // cache these
        card_t *card = &deck->cards[card_idx];
        char card_code[2] = card->card_code;

        // each section gets a header at a minimum
        write_header(ctx, deck, ctx->output_fp);

        // write any comments for this section
        write_comments(ctx, deck, ctx->output_fp);

        // calculate geometry if not already done
        if (ctx->geometry.num_segs == 0 && ctx->geometry.num_patches == 0) {
            errors_list_t geometry_errors = {0};
            calculate_geometry(ctx, deck, &geometry_errors, &ctx->outputs);
            
            if (geometry_errors.num_errors > 0) {
                transfer_errors(&geometry_errors, &ctx->errors);
                return -1;
            }
        }

        // write the structure and segments to the output file
        write_structure(ctx, deck, ctx->output_fp);
        write_segments(ctx, deck, ctx->output_fp);
    
        // setup calculation_defaults
        if (ctx->geometry.num_segs > 0 || ctx->geometry.num_patches > 0) {
            if (ctx->netcx.num_eq_sym == 0) {
                // initialize matrix parameters based on the segment and patch counts
                ctx->netcx.num_eq_sym = ctx->geometry.num_segs_sym + 2 * ctx->geometry.num_patches_sym;
                if (ctx->netcx.num_eq == 0) {
                    ctx->netcx.num_eq = ctx->geometry.num_segs + 2 * ctx->geometry.num_patches;
                }
            }
        }
        
        // set up the report state
        report_state_t state;
        init_report_state(&state);
    
        // start processing the cards after geometry section
        int start_idx = (DECK_GEOMETRY_END(deck) >= 0) ? DECK_GEOMETRY_END(deck) + 1 : 0;
        section_t *primary = DECK_PRIMARY_SECTION(deck);
        int end_idx = (primary && primary->global_end >= 0) ? primary->global_end : deck->num_cards;
    
        if (start_idx >= end_idx) {
            //o control cards to process, we're done
            free_report_state(&state);
            return 0;
        }
    
    // MAIN PROCESSING LOOP - starts at Fortran line 14
    for (int i = start_idx; i < end_idx; i++) {
        card_t *card = &deck->cards[i];
        
        // skip comments and ignored cards (but write them if configured)
        if (card->ignore || is_comment(card)) {
            if (ctx->output_fp && is_comment(card)) {
                fprintf(ctx->output_fp, "%s\n", card->card_str ? card->card_str : "");
            }
            continue;
        }
        
        // skip pattern cards (RP, NE, NH, PT, PQ) that were already output as part of XQ look-ahead
        if ((strcmp(card->card_code, "RP") == 0 ||
             strcmp(card->card_code, "NE") == 0 ||
             strcmp(card->card_code, "NH") == 0 ||
             strcmp(card->card_code, "PT") == 0 ||
             strcmp(card->card_code, "PQ") == 0) &&
            i <= state.last_processed_pattern_idx) {
            continue;
        }
        
        // dispatch to card-specific handler
        int result = dispatch_card(ctx, deck, i, &state);
        if (result != 0) {
            free_report_state(&state);
            return result;
        }
        
        // check for EN (end of deck) card
        if (strcmp(card->card_code, "EN") == 0) {
            break;
        }
    }
    
    // Don't call write_footer here - it will be called by main.c after we return
    
    free_report_state(&state);
    return 0;
    } while(true);
}

/* ============================================================================
 * Initialization and State Management
 * ========================================================================== */

void init_report_state(report_state_t *state)
{
    if (!state) return;
    
    /* Card counting */
    state->total_cards_processed = 0;  /* mpcnt */
    
    /* Processing stages */
    state->processing_stage = 0;  /* igo - Not started */
    state->card_sequence_state = 0; /* iflow - Not started */
    
    /* Frequency parameters */
    state->num_frequencies = 1;  /* nfrq */
    state->freq_stepping_mode = 0;  /* ifrq - Linear frequency stepping */
    state->current_frequency_mhz = 0.0;  /* fmhz */
    state->frequency_delta = 0.0;  /* delfrq */
    state->freq_iteration = 0;  /* mhz */
    
    /* Print control */
    state->currents_print_control = 0;  /* iptflg */
    state->charges_print_control = 0;  /* iptflq */
    state->impedance_norm_type = 0;  /* iped */
    state->impedance_norm_value = 0.0;  /* zpnorm */
    
    /* Pattern control */
    state->excitation_type = 0;  /* ixtyp */
    state->num_theta_angles = 0;  /* nthi */
    state->num_phi_angles = 0;  /* nphi */
    
    /* Matrix parameters */
    state->matrix_integration_limit = 1.0;  /* rkh - Default matrix integration limit, matches batch processor */
    state->use_extended_kernel = 0;  /* iexk */
    
    /* Geometry storage */
    state->wire_x_saved = NULL;  /* xtemp */
    state->wire_y_saved = NULL;  /* ytemp */
    state->wire_z_saved = NULL;  /* ztemp */
    state->wire_half_length_saved = NULL;  /* sitemp */
    state->wire_radius_saved = NULL;  /* bitemp */
    state->patch_x_saved = NULL;  /* patch_xtemp */
    state->patch_y_saved = NULL;  /* patch_ytemp */
    state->patch_z_saved = NULL;  /* patch_ztemp */
    state->patch_area_saved = NULL;  /* patch_atemp */
    state->wire_geometry_saved = 0;  /* ifrtmw */
    state->patch_geometry_saved = 0;  /* ifrtmp */
    
    /* Pattern tracking */
    state->last_processed_pattern_idx = -1;
    state->num_rp_cards = 0;
    state->last_freq_output_mhz = -999.0;  /* Initialize to impossible frequency */
}

void free_report_state(report_state_t *state)
{
    if (!state) return;
    
    free(state->wire_x_saved);
    free(state->wire_y_saved);
    free(state->wire_z_saved);
    free(state->wire_half_length_saved);
    free(state->wire_radius_saved);
    free(state->patch_x_saved);
    free(state->patch_y_saved);
    free(state->patch_z_saved);
    free(state->patch_area_saved);
    
    state->wire_x_saved = NULL;
    state->wire_y_saved = NULL;
    state->wire_z_saved = NULL;
    state->wire_half_length_saved = NULL;
    state->wire_radius_saved = NULL;
    state->patch_x_saved = NULL;
    state->patch_y_saved = NULL;
    state->patch_z_saved = NULL;
    state->patch_area_saved = NULL;
}