/*
 * reporting.c - Card-by-Card Sequential Processing for OpenNEC
 *
 * Implements sequential (one-at-a-time) card processing following the
 * original Fortran NEC-2 / nec2c design pattern.
 *
 * References:
 * - Fortran NEC: ~/Downloads/Nec2dXS_src/nec2dxs.f (lines 14, 40-120, 293-307)
 * - C port nec2c: ~/Developer/nec2c-1.3/main.c (lines 241, 306-580, 607-2025)
 */

#include "reporting.h"
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

/* Forward declarations */
static int execute_frequency_loop_sequential(context_t *ctx, deck_t *deck,
                                             int card_idx, card_state_t *state);

/* ============================================================================
 * Initialization and State Management
 * ========================================================================== */

void init_card_state(card_state_t *state)
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

void free_card_state(card_state_t *state)
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

/******************************************************************************
 * reset_coupling_buffers()
 *
 * Reset and free coupling buffers. Called when starting a new batch.
 */
static void reset_coupling_buffers(context_t *ctx)
{
    if (ctx->yparm.num_pairs > 0) {
        mem_free(ctx, (void **)&ctx->yparm.pair_tags);
        mem_free(ctx, (void **)&ctx->yparm.pair_segs);
        ctx->yparm.num_pairs = 0;
    }
    /* Reset coupling calculation state for new frequency */
    ctx->yparm.coupling_flag = 0;
    if (ctx->yparm.y11 != NULL) {
        mem_free(ctx, (void **)&ctx->yparm.y11);
    }
    if (ctx->yparm.y12 != NULL) {
        mem_free(ctx, (void **)&ctx->yparm.y12);
    }
}

/* ============================================================================
 * Geometry Scaling Utility Functions
 * ========================================================================== */

static int save_geometry_for_scaling(context_t *ctx, card_state_t *state)
{
    if (!ctx || !state) return -1;
    
    geometry_t *geom = &ctx->geometry;
    
    /* Save wire geometry if not already saved */
    if (!state->wire_geometry_saved && geom->num_segs > 0) {
        size_t n = geom->num_segs;
        
        state->wire_x_saved = (double *)malloc(n * sizeof(double));
        state->wire_y_saved = (double *)malloc(n * sizeof(double));
        state->wire_z_saved = (double *)malloc(n * sizeof(double));
        state->wire_half_length_saved = (double *)malloc(n * sizeof(double));
        state->wire_radius_saved = (double *)malloc(n * sizeof(double));
        
        if (!state->wire_x_saved || !state->wire_y_saved || !state->wire_z_saved ||
            !state->wire_half_length_saved || !state->wire_radius_saved) {
            add_error(ctx, &ctx->errors,
                     "Memory allocation failed for geometry scaling", FATAL);
            return -1;
        }
        
        memcpy(state->wire_x_saved, geom->x_center, n * sizeof(double));
        memcpy(state->wire_y_saved, geom->y_center, n * sizeof(double));
        memcpy(state->wire_z_saved, geom->z_center, n * sizeof(double));
        memcpy(state->wire_half_length_saved, geom->half_len, n * sizeof(double));
        memcpy(state->wire_radius_saved, geom->radius, n * sizeof(double));
        
        state->wire_geometry_saved = 1;  /* ifrtmw */
    }
    
    /* Save patch geometry if not already saved */
    if (!state->patch_geometry_saved && geom->num_patches > 0) {
        size_t m = geom->num_patches;
        
        state->patch_x_saved = (double *)malloc(m * sizeof(double));
        state->patch_y_saved = (double *)malloc(m * sizeof(double));
        state->patch_z_saved = (double *)malloc(m * sizeof(double));
        state->patch_area_saved = (double *)malloc(m * sizeof(double));
        
        if (!state->patch_x_saved || !state->patch_y_saved || !state->patch_z_saved ||
            !state->patch_area_saved) {
            add_error(ctx, &ctx->errors,
                     "Memory allocation failed for patch scaling", FATAL);
            return -1;
        }
        
        memcpy(state->patch_x_saved, geom->patch_x_center, m * sizeof(double));
        memcpy(state->patch_y_saved, geom->patch_y_center, m * sizeof(double));
        memcpy(state->patch_z_saved, geom->patch_z_center, m * sizeof(double));
        memcpy(state->patch_area_saved, geom->patch_area, m * sizeof(double));
        
        state->patch_geometry_saved = 1;  /* ifrtmp */
    }
    
    return 0;
}

static void scale_geometry_for_frequency(context_t *ctx, const card_state_t *state,
                                        double fr)
{
    if (!ctx || !state) return;
    
    geometry_t *geom = &ctx->geometry;
    
    /* Scale wire segments */
    if (state->wire_geometry_saved && geom->num_segs > 0) {
        for (int i = 0; i < geom->num_segs; i++) {
            geom->x_center[i] = state->wire_x_saved[i] * fr;  /* xw1 */
            geom->y_center[i] = state->wire_y_saved[i] * fr;  /* yw1 */
            geom->z_center[i] = state->wire_z_saved[i] * fr;  /* zw1 */
            geom->half_len[i] = state->wire_half_length_saved[i] * fr;  /* sw1 */
            geom->radius[i] = state->wire_radius_saved[i] * fr;  /* rad1 */
        }
    }
    
    /* Scale patch geometry */
    if (state->patch_geometry_saved && geom->num_patches > 0) {
        double fr2 = fr * fr;
        for (int i = 0; i < geom->num_patches; i++) {
            geom->patch_x_center[i] = state->patch_x_saved[i] * fr;  /* xp1 */
            geom->patch_y_center[i] = state->patch_y_saved[i] * fr;  /* yp1 */
            geom->patch_z_center[i] = state->patch_z_saved[i] * fr;  /* zp1 */
            geom->patch_area[i] = state->patch_area_saved[i] * fr2;  /* ap1 */
        }
    }
}

static void restore_geometry(context_t *ctx, const card_state_t *state)
{
    if (!ctx || !state) return;
    
    geometry_t *geom = &ctx->geometry;
    
    /* Restore wire segments */
    if (state->wire_geometry_saved && geom->num_segs > 0) {
        memcpy(geom->x_center, state->wire_x_saved, geom->num_segs * sizeof(double));  /* xw1 */
        memcpy(geom->y_center, state->wire_y_saved, geom->num_segs * sizeof(double));  /* yw1 */
        memcpy(geom->z_center, state->wire_z_saved, geom->num_segs * sizeof(double));  /* zw1 */
        memcpy(geom->half_len, state->wire_half_length_saved, geom->num_segs * sizeof(double));  /* sw1 */
        memcpy(geom->radius, state->wire_radius_saved, geom->num_segs * sizeof(double));  /* rad1 */
    }
    
    /* Restore patches */
    if (state->patch_geometry_saved && geom->num_patches > 0) {
        memcpy(geom->patch_x_center, state->patch_x_saved, 
               geom->num_patches * sizeof(double));  /* xp1 */
        memcpy(geom->patch_y_center, state->patch_y_saved, 
               geom->num_patches * sizeof(double));  /* yp1 */
        memcpy(geom->patch_z_center, state->patch_z_saved, 
               geom->num_patches * sizeof(double));  /* zp1 */
        memcpy(geom->patch_area, state->patch_area_saved, 
               geom->num_patches * sizeof(double));  /* ap1 */
    }
}

/* ============================================================================
 * Main Processing Loop
 * ========================================================================== */

int process_deck_sequential(context_t *ctx, deck_t *deck)
{
    // have to have a deck and a context
    if (!ctx || !deck) return -1;

    // and a valid output file
    if (!ctx->output_fp) return -1;

    // and the deck must have at least one card
    if (deck->num_cards <= 0) return -1;

    // Phase 4: Process each section independently
    if (deck->num_sections == 0) {
        /* No sections created - deck_create_sections() should have been called during parsing */
        add_error(ctx, &ctx->errors, "Internal error: no sections in deck", FATAL);
        return -1;
    }

    /* Loop over all sections */
    for (int section_num = 0; section_num < deck->num_sections; section_num++) {
        section_t *section = deck->sections[section_num];
        
        /* Output section header if multi-section deck */
        if (deck->num_sections > 1) {
            fprintf(ctx->output_fp, "\n");
            fprintf(ctx->output_fp, "                             - - - SECTION %d - - -\n", section_num + 1);
            fprintf(ctx->output_fp, "\n");
        }
        
        /* Step 1: Calculate geometry for this section */
        /* Clear previous section's geometry */
        if (section_num > 0) {
            /* Reset geometry for new section */
            if (ctx->geometry.end1_x != NULL) { free(ctx->geometry.end1_x); ctx->geometry.end1_x = NULL; }
            if (ctx->geometry.end1_y != NULL) { free(ctx->geometry.end1_y); ctx->geometry.end1_y = NULL; }
            if (ctx->geometry.end1_z != NULL) { free(ctx->geometry.end1_z); ctx->geometry.end1_z = NULL; }
            if (ctx->geometry.end2_x != NULL) { free(ctx->geometry.end2_x); ctx->geometry.end2_x = NULL; }
            if (ctx->geometry.end2_y != NULL) { free(ctx->geometry.end2_y); ctx->geometry.end2_y = NULL; }
            if (ctx->geometry.end2_z != NULL) { free(ctx->geometry.end2_z); ctx->geometry.end2_z = NULL; }
            if (ctx->geometry.x_center != NULL) { free(ctx->geometry.x_center); ctx->geometry.x_center = NULL; }
            if (ctx->geometry.y_center != NULL) { free(ctx->geometry.y_center); ctx->geometry.y_center = NULL; }
            if (ctx->geometry.z_center != NULL) { free(ctx->geometry.z_center); ctx->geometry.z_center = NULL; }
            if (ctx->geometry.half_len != NULL) { free(ctx->geometry.half_len); ctx->geometry.half_len = NULL; }
            if (ctx->geometry.radius != NULL) { free(ctx->geometry.radius); ctx->geometry.radius = NULL; }
            if (ctx->geometry.dir_cos_x != NULL) { free(ctx->geometry.dir_cos_x); ctx->geometry.dir_cos_x = NULL; }
            if (ctx->geometry.dir_cos_y != NULL) { free(ctx->geometry.dir_cos_y); ctx->geometry.dir_cos_y = NULL; }
            if (ctx->geometry.dir_cos_z != NULL) { free(ctx->geometry.dir_cos_z); ctx->geometry.dir_cos_z = NULL; }
            if (ctx->geometry.seg_end1_conn != NULL) { free(ctx->geometry.seg_end1_conn); ctx->geometry.seg_end1_conn = NULL; }
            if (ctx->geometry.seg_end2_conn != NULL) { free(ctx->geometry.seg_end2_conn); ctx->geometry.seg_end2_conn = NULL; }
            if (ctx->geometry.tag_nums != NULL) { free(ctx->geometry.tag_nums); ctx->geometry.tag_nums = NULL; }
            if (ctx->geometry.card_nums != NULL) { free(ctx->geometry.card_nums); ctx->geometry.card_nums = NULL; }
            ctx->geometry.num_segs = 0;
            ctx->geometry.num_segs_sym = 0;
            ctx->geometry.num_patches = 0;
            ctx->geometry.num_patches_sym = 0;
        }
        
        /* Temporarily make this section the "primary" for legacy code */
        section_t *saved_primary = deck->sections[0];
        deck->sections[0] = section;
        
        errors_list_t geometry_errors = {0};
        calculate_geometry(ctx, deck, &geometry_errors, &ctx->outputs);
        
        if (geometry_errors.num_errors > 0) {
            transfer_errors(&geometry_errors, &ctx->errors);
            deck->sections[0] = saved_primary;
            return -1;
        }

        // Write the structure and segments to the output file for this section
        write_structure(ctx, deck, ctx->output_fp);
        write_segments(ctx, deck, ctx->output_fp);
        
        /* Setup calculation_defaults after geometry */
        if (ctx->geometry.num_segs > 0 || ctx->geometry.num_patches > 0) {
            if (ctx->netcx.num_eq_sym == 0 || section_num > 0) {
                /* Initialize matrix parameters from geometry */
                ctx->netcx.num_eq_sym = ctx->geometry.num_segs_sym + 2 * ctx->geometry.num_patches_sym;
                ctx->netcx.num_eq = ctx->geometry.num_segs + 2 * ctx->geometry.num_patches;
            }
        }
        
        card_state_t state;
        init_card_state(&state);
        
        /* Start processing cards after geometry section */
        int start_idx = (section->geometry_end >= 0) ? section->geometry_end + 1 : section->global_start;
        int end_idx = section->global_end;
        
        if (start_idx < end_idx) {
            /* Main card processing loop for this section - Fortran line 14 */
            for (int i = start_idx; i <= end_idx; i++) {
                card_t *card = &deck->cards[i];
                
                /* Skip comments and ignored cards (but write them if configured) */
                if (card->ignore || is_comment(card)) {
                    if (ctx->output_fp && is_comment(card)) {
                        fprintf(ctx->output_fp, "%s\n", card->card_str ? card->card_str : "");
                    }
                    continue;
                }
                
                /* Skip pattern cards (RP, NE, NH, PT, PQ) that were already output as part of XQ look-ahead */
                if ((strcmp(card->card_code, "RP") == 0 ||
                     strcmp(card->card_code, "NE") == 0 ||
                     strcmp(card->card_code, "NH") == 0 ||
                     strcmp(card->card_code, "PT") == 0 ||
                     strcmp(card->card_code, "PQ") == 0) &&
                    i <= state.last_processed_pattern_idx) {
                    continue;
                }
                
                /* Skip NX and EN termination cards */
                if (strcmp(card->card_code, "NX") == 0 || strcmp(card->card_code, "EN") == 0) {
                    /* Output the card for documentation */
                    if (ctx->output_fp) {
                        fprintf(ctx->output_fp, "%s\n", card->card_str ? card->card_str : "");
                    }
                    break;
                }
                
                /* Dispatch to card-specific handler */
                int result = dispatch_card(ctx, deck, i, &state);
                if (result != 0) {
                    free_card_state(&state);
                    deck->sections[0] = saved_primary;
                    return result;
                }
            }
        }
        
        /* Auto-execute if FR + EX + pattern cards present but no XQ was encountered */
        if (state.num_frequencies > 0 && 
            ctx->ex_queue.num_queued > 0 &&
            (ctx->fpat.is_near_field >= 0 || ctx->gnd.far_field_type >= 0)) {
            
            /* Simulate XQ execution for automatic calculation */
            card_state_t *exec_state = &state;
            int exec_result = execute_frequency_loop_sequential(ctx, deck, end_idx, exec_state);
            
            if (exec_result != 0) {
                free_card_state(&state);
                deck->sections[0] = saved_primary;
                return exec_result;
            }
        }
        
        free_card_state(&state);
        
        /* Restore original primary section */
        deck->sections[0] = saved_primary;
    }
    
    /* Don't call write_footer here - it will be called by main.c after we return */
    
    return 0;
}

/* ============================================================================
 * Frequency Loop Helper Functions
 * ========================================================================== */

/**
 * Allocate storage for matrix and working vectors during frequency loop
 */
static int allocate_frequency_loop_storage(context_t *ctx, int num_equations)
{
    if (!ctx || num_equations <= 0) return -1;
    
    /* Set equation count */
    ctx->netcx.num_eq = num_equations;
    
    /* Allocate matrix storage (done in execute_frequency_loop_sequential)
     * Each frequency loop needs a fresh matrix allocation and factorization
     * This is handled locally in the frequency loop to match control.c pattern
     */
    
    return 0;
}

/**
 * Free frequency loop storage
 */
static void free_frequency_loop_storage(context_t *ctx)
{
    if (!ctx) return;
    
    /* Matrix is freed locally in execute_frequency_loop_sequential */
    /* Pivot array is stored in ctx->save.pivot and managed there */
}

/**
 * Setup excitation - call excitation setup for current frequency
 */
/**
 * Calculate input power delivered to the antenna
 */
static double __attribute__((unused)) calculate_input_power(context_t *ctx)
{
    if (!ctx) return 0.0;
    
    double power_in = 0.0;
    
    /* Sum power from all voltage sources */
    for (int i = 0; i < ctx->ex_queue.num_queued; i++) {
        if (ctx->ex_queue.queued[i].type == 0) {  /* Voltage source */
            /* Power = V * conj(I) */
            complex double voltage = ctx->ex_queue.queued[i].voltage;
            /* Current would come from solution - stub for now */
            complex double current = 1.0 + 0.0*I;  /* TODO: get from solution */
            power_in += creal(voltage * conj(current)) / 2.0;  /* Real power */
        }
    }
    
    return power_in;
}

/**
 * Calculate radiated power
 */
static double __attribute__((unused)) calculate_radiated_power(context_t *ctx)
{
    if (!ctx) return 0.0;
    
    /* Radiated power would be calculated from far-field pattern */
    /* For now, return 0 - this would be filled in when radiation pattern is calculated */
    return 0.0;  /* TODO: integrate with radiation pattern calculation */
}

/* ============================================================================
 * Card Dispatcher
 * ========================================================================== */

/**
 * Calculate coupling parameters (wrapper)
 */
static int __attribute__((unused)) calculate_coupling_parameters(context_t *ctx)
{
    if (!ctx) return -1;
    
    /* Calculate coupling between source and load */
    if (ctx->yparm.num_pairs == 0) return 0;
    
    /* Would call compute_coupling here */
    return 0;
}

static int dispatch_card(context_t *ctx, deck_t *deck, int card_idx,
                        card_state_t *state)
{
    fflush(stderr);
    
    if (!ctx || !deck || card_idx < 0 || card_idx >= deck->num_cards || !state) {
        fflush(stderr);
        return -1;
    }
    
    card_t *card = &deck->cards[card_idx];
    const char *code = card->card_code;
    
    fflush(stderr);
    
    /* Write card echo to output if configured */
    if (ctx->output_fp) {
        state->total_cards_processed++;
        fprintf(ctx->output_fp,
            "  DATA CARD No: %3d "
            "%s %3d %5d %5d %5d %12.5E %12.5E %12.5E %12.5E %12.5E %12.5E\n",
            state->total_cards_processed, code, card->i[1], card->i[2], card->i[3], card->i[4],
            card->f[1], card->f[2], card->f[3], card->f[4], card->f[5], card->f[6]);
    }
    
    /* Dispatch based on card code - Fortran ATST array, nec2c switch */
    if (strcmp(code, "FR") == 0) {
        return process_fr_card(ctx, card, state);
    }
    else if (strcmp(code, "LD") == 0) {
        return process_ld_card(ctx, card, state);
    }
    else if (strcmp(code, "GN") == 0) {
        return process_gn_card(ctx, card, state);
    }
    else if (strcmp(code, "EX") == 0) {
        return process_ex_card(ctx, card, state);
    }
    else if (strcmp(code, "NT") == 0 || strcmp(code, "TL") == 0) {
        return process_nt_tl_card(ctx, card, state);
    }
    else if (strcmp(code, "XQ") == 0) {
        return process_xq_card(ctx, deck, card_idx, state);
    }
    else if (strcmp(code, "RP") == 0) {
        return process_rp_card(ctx, card, state);
    }
    else if (strcmp(code, "NE") == 0) {
        return process_ne_card(ctx, card, state);
    }
    else if (strcmp(code, "NH") == 0) {
        return process_nh_card(ctx, card, state);
    }
    else if (strcmp(code, "PT") == 0) {
        return process_pt_card(ctx, card, state);
    }
    else if (strcmp(code, "PQ") == 0) {
        return process_pq_card(ctx, card, state);
    }
    else if (strcmp(code, "KH") == 0) {
        return process_kh_card(ctx, card, state);
    }
    else if (strcmp(code, "EK") == 0) {
        return process_ek_card(ctx, card, state);
    }
    else if (strcmp(code, "CP") == 0) {
        return process_cp_card(ctx, card, state);
    }
    else if (strcmp(code, "GD") == 0) {
        return process_gd_card(ctx, card, state);
    }
    else if (strcmp(code, "NX") == 0) {
        return process_nx_card(ctx, deck, card_idx, state);
    }
    else if (strcmp(code, "PL") == 0) {
        /* Plot flags - not typically used in this implementation */
        return 0;
    }
    else if (strcmp(code, "EN") == 0) {
        /* End of deck - handled by caller */
        return 0;
    }
    else {
        /* Unknown card type - record as warning but continue */
        char msg[MAX_ERROR_LEN];
        snprintf(msg, sizeof(msg), "Unknown control card type: %s at card %d",
                code, state->total_cards_processed);
        add_error(ctx, &ctx->errors, msg, WARNING);
        return 0;
    }
}

/* ============================================================================
 * Card Processor Functions - Each Returns 0 on Success, -1 on Error
 * ========================================================================== */

/* ============================================================================
 * Wrapper Functions for Missing or Problematic Functions
 * ========================================================================== */

/**
 * Add voltage source to context
 * Simple wrapper around existing voltage source queue system
 */
static int add_voltage_source(context_t *ctx, int tag, int seg, complex double voltage)
{
    if (!ctx) return -1;
    
    /* Queue EX card for later processing */
    if (ctx->ex_queue.num_queued >= 150) {
        add_error(ctx, &ctx->errors, "Too many EX cards queued", WARNING);
        return -1;
    }
    
    int idx = ctx->ex_queue.num_queued++;
    ctx->ex_queue.queued[idx].type = 0;  /* Voltage source */
    ctx->ex_queue.queued[idx].tag = tag;
    ctx->ex_queue.queued[idx].seg_index = seg;
    ctx->ex_queue.queued[idx].voltage = voltage;
    
    return 0;
}

/**
 * Add current source to context (similar to voltage source)
 */
static int add_current_source(context_t *ctx, int tag, int seg, complex double current)
{
    if (!ctx) return -1;
    
    /* Queue EX card as current source */
    if (ctx->ex_queue.num_queued >= 150) {
        add_error(ctx, &ctx->errors, "Too many EX cards queued", WARNING);
        return -1;
    }
    
    int idx = ctx->ex_queue.num_queued++;
    ctx->ex_queue.queued[idx].type = 5;  /* Current source */
    ctx->ex_queue.queued[idx].tag = tag;
    ctx->ex_queue.queued[idx].seg_index = seg;
    ctx->ex_queue.queued[idx].voltage = current;
    
    return 0;
}

/**
 * Add loading impedance (wrapper around existing mechanism)
 */
static int add_loading(context_t *ctx, int ldtyp, int ldtag, int ldtagf, 
                      int ldtagt, double zlr, double zli, double zlc)
{
    fflush(stderr);
    
    if (!ctx) return -1;
    
    /* Validate and add to loading impedance system */
    if (ctx->zload.num_loads >= 200) {  /* Reasonable limit */
        add_error(ctx, &ctx->errors, "Too many loading impedances", WARNING);
        return -1;
    }
    
    fflush(stderr);
    
    ctx->zload.num_loads++;
    int idx = ctx->zload.num_loads - 1;
    
    fflush(stderr);
    
    /* Allocate/reallocate all arrays for this new load */
    size_t mreq = (size_t)ctx->zload.num_loads * sizeof(int);
    if (mem_realloc(ctx, (void **)&ctx->zload.load_types, mreq) != 0 ||
        mem_realloc(ctx, (void **)&ctx->zload.load_tags, mreq) != 0 ||
        mem_realloc(ctx, (void **)&ctx->zload.load_tag_from, mreq) != 0 ||
        mem_realloc(ctx, (void **)&ctx->zload.load_tag_to, mreq) != 0 ||
        mem_realloc(ctx, (void **)&ctx->zload.ldcard_num, mreq) != 0) {
        fflush(stderr);
        return -1;
    }
    
    fflush(stderr);
    
    mreq = (size_t)ctx->zload.num_loads * sizeof(double);
    if (mem_realloc(ctx, (void **)&ctx->zload.load_r, mreq) != 0 ||
        mem_realloc(ctx, (void **)&ctx->zload.load_l, mreq) != 0 ||
        mem_realloc(ctx, (void **)&ctx->zload.load_c, mreq) != 0 ||
        mem_realloc(ctx, (void **)&ctx->zload.load_freq, mreq) != 0) {
        fflush(stderr);
        return -1;
    }
    
    fflush(stderr);
    
    ctx->zload.load_types[idx] = ldtyp;
    ctx->zload.load_tags[idx] = ldtag;
    ctx->zload.load_tag_from[idx] = ldtagf;
    ctx->zload.load_tag_to[idx] = ldtagt;
    ctx->zload.load_r[idx] = zlr;
    ctx->zload.load_l[idx] = zli;
    ctx->zload.load_c[idx] = zlc;
    ctx->zload.load_freq[idx] = 0.0;  /* Use first FR card frequency */
    
    fflush(stderr);
    
    return 0;
}

/**
 * Add network (transmission line or lumped network)
 */
static int add_network(context_t *ctx, int ntyp, int tag1, int seg1, 
                      int tag2, int seg2, double x11r, double x11i, 
                      double x12r, double x12i, double x22r, double x22i)
{
    if (!ctx) return -1;
    
    if (ctx->netcx.num_networks >= 200) {  /* Reasonable limit */
        add_error(ctx, &ctx->errors, "Too many networks", WARNING);
        return -1;
    }
    
    /* Allocate/reallocate network arrays as needed - CRITICAL FIX */
    ctx->netcx.num_networks++;
    size_t mreq = (size_t)ctx->netcx.num_networks * sizeof(int);
    mem_realloc(ctx, (void **)&ctx->netcx.net_types, mreq);
    mem_realloc(ctx, (void **)&ctx->netcx.net_seg1, mreq);
    mem_realloc(ctx, (void **)&ctx->netcx.net_seg2, mreq);
    
    mreq = (size_t)ctx->netcx.num_networks * sizeof(double);
    mem_realloc(ctx, (void **)&ctx->netcx.y11_real, mreq);
    mem_realloc(ctx, (void **)&ctx->netcx.y11_imag, mreq);
    mem_realloc(ctx, (void **)&ctx->netcx.y12_real, mreq);
    mem_realloc(ctx, (void **)&ctx->netcx.y12_imag, mreq);
    mem_realloc(ctx, (void **)&ctx->netcx.y22_real, mreq);
    mem_realloc(ctx, (void **)&ctx->netcx.y22_imag, mreq);
    
    int idx = ctx->netcx.num_networks - 1;
    ctx->netcx.net_types[idx] = ntyp;
    ctx->netcx.net_seg1[idx] = seg1;
    ctx->netcx.net_seg2[idx] = seg2;
    ctx->netcx.y11_real[idx] = x11r;
    ctx->netcx.y11_imag[idx] = x11i;
    ctx->netcx.y12_real[idx] = x12r;
    ctx->netcx.y12_imag[idx] = x12i;
    ctx->netcx.y22_real[idx] = x22r;
    ctx->netcx.y22_imag[idx] = x22i;
    
    return 0;
}

/**
 * Add coupling pair
 */
static int add_coupling_pair(context_t *ctx, int tag1, int seg1, int tag2, int seg2)
{
    if (!ctx) return -1;
    
    /* Coupling is stored differently - just mark that we have coupling */
    ctx->yparm.coupling_flag = 1;  /* Mark that coupling is requested */
    
    return 0;
}

/**
 * Reset loading buffers (inline simplified version)
 */
static void reset_loading_buffers_seq(context_t *ctx)
{
    if (!ctx) return;
    ctx->zload.num_loads = 0;
}

/**
 * Reset voltage source buffers (inline simplified version)
 */
static void reset_vsorc_buffers_seq(context_t *ctx)
{
    if (!ctx) return;
    ctx->ex_queue.num_queued = 0;
}

/**
 * Reset network buffers (inline simplified version)
 */
static void reset_network_buffers(context_t *ctx)
{
    if (!ctx) return;
    ctx->netcx.num_networks = 0;
    ctx->netcx.network_type = 0;  /* Force network matrix rebuild (matches Fortran NTSOL=0) */
}

    // ORIGINAL CODE FROM CONTROL>C
//     /******************************************************************************
//  * reset_network_buffers()
//  *
//  * Reset and free network buffers. Called when starting a new batch.
//  */
// static void reset_network_buffers(context_t *ctx)
// {
//     if (ctx->netcx.num_networks > 0) {
//         mem_free(ctx, (void **)&ctx->netcx.net_types);
//         mem_free(ctx, (void **)&ctx->netcx.net_seg1);
//         mem_free(ctx, (void **)&ctx->netcx.net_seg2);
//         mem_free(ctx, (void **)&ctx->netcx.y11_real);
//         mem_free(ctx, (void **)&ctx->netcx.y11_imag);
//         mem_free(ctx, (void **)&ctx->netcx.y12_real);
//         mem_free(ctx, (void **)&ctx->netcx.y12_imag);
//         mem_free(ctx, (void **)&ctx->netcx.y22_real);
//         mem_free(ctx, (void **)&ctx->netcx.y22_imag);
//         ctx->netcx.num_networks = 0;
//     }
//     /* Force network matrix rebuild (matches Fortran NTSOL=0) */
//     ctx->netcx.network_type = 0;
// }
// }

/**
 * Transfer errors from one list to another
 * (Already defined in misc.c - just use it)
 */
/* transfer_errors() is already declared in misc.h, so we use the existing one */

/* ============================================================================
 * Card Processor Functions - Fixed Versions
 * ========================================================================== */

/**
 * FR Card - Frequency
 * Fortran label 11-18, nec2c case 0
 */
static int process_fr_card(context_t *ctx, const card_t *card, card_state_t *state)
{
    if (!ctx || !card || !state) return -1;
    
    /* Just set state variables - output happens in frequency loop */
    state->freq_stepping_mode = card->i[1];  /* ifrq - Frequency mode: 0=linear, 1=multiplicative */
    state->num_frequencies = card->i[2];  /* nfrq - Number of frequencies */
    if (state->num_frequencies == 0) state->num_frequencies = 1;
    
    state->current_frequency_mhz = card->f[1];  /* fmhz - Starting frequency in MHz */
    state->frequency_delta = card->f[2];  /* delfrq - Frequency step or multiplier */
    
    /* Reset print normalization */
    if (state->impedance_norm_type == 1) state->impedance_norm_value = 0.0;
    
    state->processing_stage = 1;    /* igo - Need matrix */
    state->card_sequence_state = 1;  /* iflow - Frequency mode */
    
    return 0;
}

/**
 * LD Card - Structure Impedance Loading
 * Fortran label 17-20, nec2c case 1
 */
static int process_ld_card(context_t *ctx, const card_t *card, card_state_t *state)
{
    fflush(stderr);
    
    if (!ctx || !card || !state) {
        return -1;
    }
    
    int ldtyp = card->i[1];
    if (state->card_sequence_state != 3) {
        /* First LD card - reset loading buffers */
        fflush(stderr);
        
        reset_loading_buffers_seq(ctx);
        
        fflush(stderr);
        
        state->card_sequence_state = 3;  /* iflow - LD cards */
        if (state->processing_stage > 2) state->processing_stage = 2;  /* igo - Reset to need TL */
        
        if (ldtyp == -1) {
            return 0;  /* LD -1 means cancel loading */
        }
    }
    
    if (ldtyp == -1) {
        return 0;  /* Cancel loading */
    }
    
    /* Extract loading parameters */
    int ldtag = card->i[2];
    int ldtagf = card->i[3];
    int ldtagt = card->i[4];
        
    if (ldtagt == 0) ldtagt = ldtagf;
    
    fflush(stderr);
    
    double zlr = card->f[1];  /* Resistance */
    double zli = card->f[2];  /* Reactance */
    double zlc = card->f[3];  /* Additional parameter (varies by ldtyp) */
    
    /* Validate tag range */
    if (ldtagt < ldtagf) {        
        char msg[MAX_ERROR_LEN];
        snprintf(msg, sizeof(msg),
                "LD card: tag end (%d) < tag start (%d)", ldtagt, ldtagf);
        add_error(ctx, &ctx->errors, msg, WARNING);
    }
    
    
    /* Add loading to context - use existing add_loading or queue system */
    int result = add_loading(ctx, ldtyp, ldtag, ldtagf, ldtagt, zlr, zli, zlc);    return result;
}

/**
 * GN Card - Ground Parameters
 * Fortran label 21-23, nec2c case 2
 */
static int process_gn_card(context_t *ctx, const card_t *card, card_state_t *state)
{
    if (!ctx || !card || !state) return -1;
    
    state->card_sequence_state = 4;  /* iflow - GN card */
    if (state->processing_stage > 2) state->processing_stage = 2;  /* igo - Reset to need TL */
    
    int iperf = card->i[1];
    
    if (iperf == -1) {
        /* GN -1: Free space */
        ctx->gnd.is_perfect = 1;
        ctx->gnd.has_ground = 1;  /* 1=no ground/free space */
        ctx->gnd.num_radials = 0;
        return 0;
    }
    
    ctx->gnd.is_perfect = iperf;
    ctx->gnd.num_radials = card->i[2];
    ctx->gnd.has_ground = 2;  /* 2=ground present */
    ctx->gnd.impedance_ratio = card->f[1] + I * 0.0;  /* Relative permittivity */
    ctx->gnd.impedance_ratio2 = card->f[2] + I * 0.0; /* Conductivity */
    
    if (ctx->gnd.num_radials != 0) {
        /* Radial wire ground screen */
        if (ctx->gnd.is_perfect == 2) {
            add_error(ctx, &ctx->errors,
                     "Radial wires not allowed with high impedance ground", WARNING);
        }
        ctx->gnd.screen_wire_len = card->f[3];
        ctx->gnd.screen_wire_radius = card->f[4];
    } else {
        /* Two-medium ground parameters */
        ctx->gnd.cliff_dist = card->f[3];
        ctx->gnd.cliff_height = card->f[4];
    }
    
    return 0;
}

/**
 * EX Card - Excitation
 * Fortran label 24-27, nec2c case 3
 */
static int process_ex_card(context_t *ctx, const card_t *card, card_state_t *state)
{
    if (!ctx || !card || !state) return -1;
    
    if (state->card_sequence_state != 5) {
        /* First EX card - reset excitation buffers */
        reset_vsorc_buffers_seq(ctx);
        state->card_sequence_state = 5;
        if (state->processing_stage > 3) state->processing_stage = 3;
    }
    
    int extype = card->i[1];
    int masym = card->i[4] / 10;  /* Asymmetry flag */
    state->impedance_norm_type = card->i[4] - masym * 10;  /* iped - Impedance normalization flag */
    state->impedance_norm_value = card->f[3];  /* zpnorm */
    
    if (state->impedance_norm_type == 1 && state->impedance_norm_value > 0) {
        state->impedance_norm_type = 2;  /* Switch to admittance normalization */
    }
    
    if (extype == 0 || extype == 5) {
        /* Voltage source (0) or Current source (5) */
        int tag = card->i[2];
        int seg = card->i[3];
        complex double voltage = card->f[1] + I * card->f[2];
        
        if (cabs(voltage) < 1e-20) {
            voltage = 1.0 + 0.0*I;  /* Default to 1 volt */
        }
        
        if (extype == 0) {
            return add_voltage_source(ctx, tag, seg, voltage);
        } else {
            return add_current_source(ctx, tag, seg, voltage);
        }
    }
    else if (extype >= 1 && extype <= 4) {
        /* Plane wave excitation */
        state->excitation_type = extype;  /* ixtyp */
        state->num_theta_angles = card->i[2];  /* nthi - Number of theta angles */
        state->num_phi_angles = card->i[3];  /* nphi - Number of phi angles */
        
        /* Store plane wave parameters in field pattern structure */
        ctx->fpat.theta_start = card->f[1];  /* Theta */
        ctx->fpat.phi_start = card->f[2];  /* Phi */
        /* Note: eta (376.73 ohms) is constant, not stored */
        ctx->fpat.theta_step = card->f[4];  /* Theta step */
        ctx->fpat.phi_step = card->f[5];  /* Phi step */
        
        reset_vsorc_buffers_seq(ctx);  /* Plane wave replaces voltage sources */
        return 0;
    }
    
    return 0;
}

/**
 * NT/TL Card - Network Data
 * Fortran label 28-30, nec2c case 4-5
 */
static int process_nt_tl_card(context_t *ctx, const card_t *card, card_state_t *state)
{
    if (!ctx || !card || !state) return -1;
    
    if (state->card_sequence_state != 6) {
        /* First network card */
        reset_network_buffers(ctx);
        state->card_sequence_state = 6;  /* iflow - NT/TL cards */
        if (state->processing_stage > 3) state->processing_stage = 3;  /* igo - Need XQ */
        
        if (card->i[2] == -1) {
            return 0;  /* NT -1 means cancel networks */
        }
    }
    
    int ntyp = (strcmp(card->card_code, "NT") == 0) ? 1 : 2;  /* 1=NT, 2=TL */
    int tag1 = card->i[1];
    int seg1_within_tag = card->i[2];
    int tag2 = card->i[3];
    int seg2_within_tag = card->i[4];
    
    /* CRITICAL FIX: Convert (tag, segment_within_tag) to global segment numbers */
    int seg1 = segment_number(ctx, tag1, seg1_within_tag);
    int seg2 = segment_number(ctx, tag2, seg2_within_tag);
    
    if (seg1 <= 0 || seg2 <= 0) {
        char msg[MAX_ERROR_LEN];
        snprintf(msg, sizeof(msg), "Network card: Invalid segment reference. Tag1=%d Seg1=%d Tag2=%d Seg2=%d", 
                 tag1, seg1_within_tag, tag2, seg2_within_tag);
        add_error(ctx, &ctx->errors, msg, WARNING);
        return -1;
    }
    
    double x11r = card->f[1];
    double x11i = card->f[2];
    double x12r = card->f[3];
    double x12i = card->f[4];
    double x22r = card->f[5];
    double x22i = card->f[6];
    
    /* Add to network system */
    return add_network(ctx, ntyp, tag1, seg1, tag2, seg2,
                      x11r, x11i, x12r, x12i, x22r, x22i);
}

/**
 * XQ Card - Execute Frequency Loop
 * Fortran label 37-40, nec2c case 6
 */
static int process_xq_card(context_t *ctx, deck_t *deck, int card_idx,
                          card_state_t *state)
{
    if (!ctx || !deck || !state) return -1;
    
    const card_t *card = &deck->cards[card_idx];
    
    /* Check if this is a request to skip */
    if (state->card_sequence_state == 10 && card->i[1] == 0) {
        return 0;  /* Skip */
    }
    if (state->num_frequencies == 1 && card->i[1] == 0 && state->card_sequence_state > 7) {
        return 0;  /* Skip single frequency */
    }
    
    /* Set card_sequence_state based on XQ parameter (matches Fortran IFLOW logic) */
    if (card->i[1] != 0) {
        state->card_sequence_state = 10;  /* iflow=7, XQ with parameter (sets default RP) */
    } else if (state->card_sequence_state > 7) {
        state->card_sequence_state = 11;  /* iflow=11, XQ without parameter, after patterns */
    } else {
        state->card_sequence_state = 7;   /* iflow=7, XQ without parameter, before patterns */
    }
    
    /* Look ahead to collect RP/NE/NH cards that follow this XQ */
    state->num_rp_cards = 0;  /* Reset RP card collection for this XQ */
    int last_pattern_idx = card_idx;
    
    for (int i = card_idx + 1; i < deck->num_cards; i++) {
        card_t *next_card = &deck->cards[i];
        
        /* Skip comments and ignored cards */
        if (is_comment(next_card) || next_card->ignore) {
            continue;
        }
        
        /* Collect RP/NE/NH cards */
        if (strcmp(next_card->card_code, "RP") == 0 && state->num_rp_cards < MAX_RP_CARDS_PER_FREQUENCY) {
            /* Extract RP card parameters */
            int n_theta = (next_card->i[2] == 0) ? 1 : next_card->i[2];
            int n_phi = (next_card->i[3] == 0) ? 1 : next_card->i[3];
            double theta_start = next_card->f[1];
            double phi_start = next_card->f[2];
            double theta_step = next_card->f[3];
            double phi_step = next_card->f[4];
            
            state->rp_cards[state->num_rp_cards].num_theta = n_theta;
            state->rp_cards[state->num_rp_cards].num_phi = n_phi;
            state->rp_cards[state->num_rp_cards].theta_start = theta_start;
            state->rp_cards[state->num_rp_cards].phi_start = phi_start;
            state->rp_cards[state->num_rp_cards].theta_step = theta_step;
            state->rp_cards[state->num_rp_cards].phi_step = phi_step;
            state->num_rp_cards++;
            
            last_pattern_idx = i;
            /* Set far_field_type to 0 if not already set (matches NEC-2 RP card default) */
            if (ctx->gnd.far_field_type == -1) {
                ctx->gnd.far_field_type = 0;
            }
        } else if (strcmp(next_card->card_code, "NE") == 0 || strcmp(next_card->card_code, "NH") == 0) {
            /* NE/NH cards are near-field requests, mark but don't collect yet */
            last_pattern_idx = i;
        } else if (strcmp(next_card->card_code, "PT") == 0 || strcmp(next_card->card_code, "PQ") == 0) {
            /* PT/PQ cards follow patterns, mark position */
            last_pattern_idx = i;
        } else {
            /* Any other card type ends the pattern sequence */
            break;
        }
    }
    
    /* Store last pattern index so main card loop can skip these cards */
    state->last_processed_pattern_idx = last_pattern_idx;
    
    /* Output RP/NE/NH/PT/PQ DATA CARD entries BEFORE frequency loop
     * These should appear immediately after the XQ card in the output */
    if (ctx->output_fp) {
        for (int i = card_idx + 1; i <= last_pattern_idx; i++) {
            card_t *pattern_card = &deck->cards[i];
            
            /* Skip comments and ignored cards */
            if (is_comment(pattern_card) || pattern_card->ignore) {
                continue;
            }
            
            /* Output RP, NE, NH, PT, PQ cards with DATA CARD numbers */
            if (strcmp(pattern_card->card_code, "RP") == 0 ||
                strcmp(pattern_card->card_code, "NE") == 0 ||
                strcmp(pattern_card->card_code, "NH") == 0 ||
                strcmp(pattern_card->card_code, "PT") == 0 ||
                strcmp(pattern_card->card_code, "PQ") == 0) {
                
                state->total_cards_processed++;
                fprintf(ctx->output_fp,
                    "  DATA CARD No: %3d "
                    "%s %3d %5d %5d %5d %12.5E %12.5E %12.5E %12.5E %12.5E %12.5E\n",
                    state->total_cards_processed, pattern_card->card_code,
                    pattern_card->i[1], pattern_card->i[2], pattern_card->i[3], pattern_card->i[4],
                    pattern_card->f[1], pattern_card->f[2], pattern_card->f[3], 
                    pattern_card->f[4], pattern_card->f[5], pattern_card->f[6]);
            }
        }
    }
    
    /* Execute frequency loop */
    fflush(stderr);
    
    int result = execute_frequency_loop_sequential(ctx, deck, card_idx, state);
    
    fflush(stderr);
    
    return result;
}

/**
 * RP Card - Radiation Pattern
 * Fortran label 36, nec2c case 8
 */
static int process_rp_card(context_t *ctx, const card_t *card, card_state_t *state)
{
    if (!ctx || !card || !state) return -1;
    
    /* Set up radiation pattern calculation using field_pattern_t */
    ctx->fpat.num_theta = card->i[2];
    ctx->fpat.num_phi = card->i[3];
    
    ctx->fpat.theta_start = card->f[1];
    ctx->fpat.phi_start = card->f[2];
    ctx->fpat.theta_step = card->f[3];
    ctx->fpat.phi_step = card->f[4];
    
    ctx->fpat.is_near_field = 0;  /* This is far-field */
    
    return 0;
}

/**
 * NE Card - Near Field (Equatorial Plane)
 * Fortran label 33, nec2c case 9
 */
static int process_ne_card(context_t *ctx, const card_t *card, card_state_t *state)
{
    if (!ctx || !card || !state) return -1;
    
    /* Set up near-field calculation */
    ctx->fpat.is_near_field = 1;
    ctx->fpat.near_field_type = 0;  /* E-field */
    
    ctx->fpat.grid_nx = card->i[2];
    ctx->fpat.grid_ny = card->i[3];
    ctx->fpat.grid_nz = card->i[4];
    
    ctx->fpat.grid_x0 = card->f[1];
    ctx->fpat.grid_y0 = card->f[2];
    ctx->fpat.grid_z0 = card->f[3];
    ctx->fpat.grid_dx = card->f[4];
    ctx->fpat.grid_dy = card->f[5];
    ctx->fpat.grid_dz = card->f[6];
    
    state->card_sequence_state = 8;  /* iflow - NE card */
    return 0;
}

/**
 * NH Card - Near Field (Horizontal Plane)
 * Fortran label 34, nec2c case 10
 */
static int process_nh_card(context_t *ctx, const card_t *card, card_state_t *state)
{
    if (!ctx || !card || !state) return -1;
    
    /* Set up near-field calculation - horizontal plane */
    ctx->fpat.is_near_field = 1;
    ctx->fpat.near_field_type = 0;  /* E-field */
    
    ctx->fpat.grid_nx = card->i[2];
    ctx->fpat.grid_ny = card->i[3];
    ctx->fpat.grid_nz = card->i[4];
    
    ctx->fpat.grid_x0 = card->f[1];
    ctx->fpat.grid_y0 = card->f[2];
    ctx->fpat.grid_z0 = card->f[3];
    ctx->fpat.grid_dx = card->f[4];
    ctx->fpat.grid_dy = card->f[5];
    ctx->fpat.grid_dz = card->f[6];
    
    state->card_sequence_state = 9;  /* iflow - NH card */
    return 0;
}

/**
 * PT Card - Print Control (Currents)
 * Fortran label 35, nec2c case 11
 */
static int process_pt_card(context_t *ctx, const card_t *card, card_state_t *state)
{
    if (!ctx || !card || !state) return -1;
    
    state->currents_print_control = card->i[1];  /* iptflg */
    state->card_sequence_state = 7;  /* iflow - PT card */
    
    return 0;
}

/**
 * PQ Card - Print Control (Charges)
 * Fortran label (similar to PT), nec2c case 12
 */
static int process_pq_card(context_t *ctx, const card_t *card, card_state_t *state)
{
    if (!ctx || !card || !state) return -1;
    
    state->charges_print_control = card->i[1];  /* iptflq */
    state->card_sequence_state = 7;  /* iflow - PQ card */
    
    return 0;
}

/**
 * KH Card - Matrix Limit (Integration Limit)
 * Fortran label (part of processing), nec2c case 13
 */
static int process_kh_card(context_t *ctx, const card_t *card, card_state_t *state)
{
    if (!ctx || !card || !state) return -1;
    
    state->matrix_integration_limit = card->f[1];  /* rkh - Integration limit in wavelengths */
    
    return 0;
}

/**
 * EK Card - Extended Kernel
 * Fortran label (part of processing), nec2c case 14
 */
static int process_ek_card(context_t *ctx, const card_t *card, card_state_t *state)
{
    if (!ctx || !card || !state) return -1;
    
    state->use_extended_kernel = card->i[1];  /* iexk - Extended kernel flag */
    
    return 0;
}

/**
 * CP Card - Coupling
 * Fortran label 31-32, nec2c case 15
 */
static int process_cp_card(context_t *ctx, const card_t *card, card_state_t *state)
{
    if (!ctx || !card || !state) return -1;
    
    if (state->card_sequence_state != 2) {
        reset_coupling_buffers(ctx);
        state->card_sequence_state = 2;  /* iflow - CP cards */
    }
    
    if (card->i[1] == -1) {
        return 0;  /* CP -1 means cancel coupling */
    }
    
    /* Add coupling pair */
    int tag1 = card->i[1];
    int seg1 = card->i[2];
    int tag2 = card->i[3];
    int seg2 = card->i[4];
    
    return add_coupling_pair(ctx, tag1, seg1, tag2, seg2);
}

/**
 * GD Card - Ground Detail
 * Fortran label 25-26, nec2c case 16
 */
static int process_gd_card(context_t *ctx, const card_t *card, card_state_t *state)
{
    if (!ctx || !card || !state) return -1;
    
    /* Extended ground parameters - normally handled inline with GN */
    /* This is for additional ground features not in basic GN card */
    
    return 0;
}

/**
 * NX Card - Next Structure
 * Fortran label 1, nec2c label l_1
 */
static int process_nx_card(context_t *ctx, deck_t *deck, int card_idx,
                          card_state_t *state)
{
    if (!ctx || !deck || !state) return -1;
    
    /* Find next structure geometry after NX card */
    int new_geom_start = -1, new_geom_end = -1;
    
    for (int i = card_idx + 1; i < deck->num_cards; i++) {
        card_t *c = &deck->cards[i];
        if (is_geometry(c)) {
            if (new_geom_start == -1) new_geom_start = i;
            new_geom_end = i;
            if (strcmp(c->card_code, "GE") == 0) break;
        }
    }
    
    if (new_geom_start == -1) {
        /* No more geometry - end of all structures */
        return 0;
    }
    
    /* Save old geometry info */
    int old_geom_start = deck->geometry_start;
    int old_geom_end = deck->geometry_end;
    
    /* Update deck to point to new geometry section */
    deck->geometry_start = new_geom_start;
    deck->geometry_end = new_geom_end;
    
    /* Recalculate geometry for new structure */
    errors_list_t geom_errors = {0};
    calculate_geometry(ctx, deck, &geom_errors, &ctx->outputs);
    
    if (geom_errors.num_errors > 0) {
        /* Restore old geometry and return error */
        deck->geometry_start = old_geom_start;
        deck->geometry_end = old_geom_end;
        transfer_errors(&geom_errors, &ctx->errors);
        return -1;
    }
    
    /* Reset state for new structure */
    free_card_state(state);
    init_card_state(state);
    
    /* Note: Caller will continue reading cards after NX position */
    /* The new geometry will be used for subsequent frequency loops */
    
    return 0;
}

/* ============================================================================
 * Frequency Loop Implementation
 * ========================================================================== */

/**
 * Main frequency loop with inline output formatting (Fortran style)
 * Fortran lines 41-120, nec2c lines 607-2025
 */
static int execute_frequency_loop_sequential(context_t *ctx, deck_t *deck,
                                            int xq_card_idx,
                                            card_state_t *state)
{    
    if (!ctx || !deck || !state) {
        return -1;
    }
    
    geometry_t *geom = &ctx->geometry;
    
    /* Allocate matrix and other structures for frequency loop */
    if (allocate_frequency_loop_storage(ctx, geom->num_segs + geom->num_patches) != 0) {
        add_error(ctx, &ctx->errors, "Failed to allocate frequency loop storage", FATAL);
        return -1;
    }
    
    /* Save geometry for frequency scaling (do this once before loop) */
    if (save_geometry_for_scaling(ctx, state) != 0) {
        return -1;
    }
    
    double fmhz1 = state->current_frequency_mhz;  /* fmhz1 */
    
    /* Allocate matrix and pivot array for frequency loop
     * This follows the pattern from control.c line ~1770
     */
    size_t iresrv = ctx->netcx.num_eq * (ctx->netcx.num_eq + 2);
    size_t mreq = iresrv * sizeof(complex double);
    
    /* Allocate interaction matrix */
    complex double *cm = NULL;
    mem_alloc(ctx, (void **)&cm, mreq);
    if (!cm) {
        add_error(ctx, &ctx->errors, "Failed to allocate interaction matrix", FATAL);
        free_frequency_loop_storage(ctx);
        return -1;
    }
    
    /* Allocate pivot array for LU factorization */
    mreq = ctx->netcx.num_eq * sizeof(int);
    if (!ctx->save.pivot) {
        mem_alloc(ctx, (void **)&ctx->save.pivot, mreq);
        if (!ctx->save.pivot) {
            add_error(ctx, &ctx->errors, "Failed to allocate pivot array", FATAL);
            mem_free(ctx, (void *)&cm);
            free_frequency_loop_storage(ctx);
            return -1;
        }
    } else {
        /* Reuse existing pivot array - just clear it */
        memset(ctx->save.pivot, 0, ctx->netcx.num_eq * sizeof(int));
    }
    
    /* Allocate current vector */
    mreq = (size_t)ctx->geometry.num_segs_3xpatches * sizeof(complex double);
    if (!ctx->crnt.surface_cur) {
        mem_alloc(ctx, (void **)&ctx->crnt.surface_cur, mreq);
        if (!ctx->crnt.surface_cur) {
            add_error(ctx, &ctx->errors, "Failed to allocate surface current array", FATAL);
            mem_free(ctx, (void *)&cm);
            free_frequency_loop_storage(ctx);
            return -1;
        }
    }
    
    /* Allocate current coefficient arrays if needed */
    mreq = (size_t)ctx->geometry.num_segs_and_patches * sizeof(double);
    if (!ctx->crnt.a_real) mem_alloc(ctx, (void **)&ctx->crnt.a_real, mreq);
    if (!ctx->crnt.a_imag) mem_alloc(ctx, (void **)&ctx->crnt.a_imag, mreq);
    if (!ctx->crnt.b_real) mem_alloc(ctx, (void **)&ctx->crnt.b_real, mreq);
    if (!ctx->crnt.b_imag) mem_alloc(ctx, (void **)&ctx->crnt.b_imag, mreq);
    if (!ctx->crnt.c_real) mem_alloc(ctx, (void **)&ctx->crnt.c_real, mreq);
    if (!ctx->crnt.c_imag) mem_alloc(ctx, (void **)&ctx->crnt.c_imag, mreq);
    
    /* Set up matrix block structure for symmetry handling (critical for factorization) */
    if (ctx->matpar.core_used == 0) {
        if (factor_block_matrix(ctx, ctx->netcx.num_eq_sym, ctx->netcx.num_eq, (int)iresrv, ctx->geometry.symmetry_flag) != 0) {
            add_error(ctx, &ctx->errors, "Failed to set up block matrix structure", FATAL);
            mem_free(ctx, (void *)&cm);
            free_frequency_loop_storage(ctx);
            return -1;
        }
    }
    
    for (state->freq_iteration = 1; state->freq_iteration <= state->num_frequencies; state->freq_iteration++) {        
        /* Calculate current frequency - Fortran lines 42-43 */
        if (state->freq_iteration > 1) {
            if (state->freq_stepping_mode == 1) {
                /* Multiplicative frequency stepping */
                state->current_frequency_mhz = fmhz1 * pow(state->frequency_delta, state->freq_iteration - 1);
            } else {
                /* Linear frequency stepping */
                state->current_frequency_mhz = fmhz1 + (state->freq_iteration - 1) * state->frequency_delta;
            }
        }
        
        double fr = state->current_frequency_mhz / CVEL;  /* fr - Frequency in 1/meters */
        double wlam = CVEL / state->current_frequency_mhz;  /* wlam - Wavelength in meters */
        
        /* Save frequency for output */
        ctx->save.freq_mhz = state->current_frequency_mhz;
        
        /* Scale geometry for current frequency - Fortran lines 44-307 */
        scale_geometry_for_frequency(ctx, state, fr);
        
        /* Set wavelength in context for calculations */
        geom->wavelength = wlam;
        
        /* Progress from stage 1 (need matrix) to stage 2 (ready to fill) */
        if (state->processing_stage == 1) {
            state->processing_stage = 2;  /* Ready to fill and factor matrix */
        }
        
        /* Process based on processing_stage state - Fortran line 40: GO TO (41,46,53,71,78), processing_stage */
        
        /* Skip matrix operations if processing_stage > 4 (already complete from prior frequency/XQ)
           This happens in sequential mode when multiple XQ cards process the same frequency */
        if (state->processing_stage > 4) {
            /* Skip matrix fill, factor, and solve - just output patterns if any */
            /* processing_stage stays at 5 (complete) */
        } else {
        
        /* processing_stage=2: Structure loading - Fortran label 46 (line 146) */
        if (state->processing_stage >= 2) {
            /* Apply loading to impedance matrix */
            if (ctx->zload.num_loads > 0) {
                if (apply_impedance_loading(ctx, 
                    ctx->zload.load_types,
                    ctx->zload.load_tags,
                    ctx->zload.load_tag_from,
                    ctx->zload.load_tag_to,
                    ctx->zload.load_r,
                    ctx->zload.load_l,
                    ctx->zload.load_c) != 0) {
                    restore_geometry(ctx, (const card_state_t *)state);
                    return -1;
                }
            }
        }
        
        /* Fill and factor matrix - Fortran lines 50, label 323 */
        if (state->processing_stage >= 2) {
            double tim1, tim2;
            
            /* Get start time for fill operation */
            get_time_ms(ctx, &tim1);
            
            /* Fill interaction matrix */
            if (fill_interaction_matrix(ctx, ctx->netcx.num_eq, 
                                       cm, state->matrix_integration_limit, state->use_extended_kernel) != 0) {
                restore_geometry(ctx, (const card_state_t *)state);
                mem_free(ctx, (void *)&cm);
                return -1;
            }
            
            get_time_ms(ctx, &tim2);
            double fill_time = (tim2 - tim1) / 1000.0;  /* Convert ms to seconds */
            
            /* Factor matrix (LU decomposition) */
            double tim3;
            factor_matrix_symmetric(ctx, ctx->netcx.num_eq_sym, ctx->netcx.num_eq, 
                                   cm, ctx->save.pivot);
            
            get_time_ms(ctx, &tim3);
            double factor_time = (tim3 - tim2) / 1000.0;  /* Convert ms to seconds */
            
            /* Store matrix timing in context for output.c to use */
            ctx->mat_fill_time = fill_time;
            ctx->mat_factor_time = factor_time;
            
            state->processing_stage = 3;
        }
        
        /* Excitation setup - Fortran label 53-56 */
        if (state->processing_stage >= 3) {
            /* Process queued EX cards to populate ctx->vsorc (voltage sources) */
            process_ex_batch(ctx);
            
            /* Fill excitation vector (right-hand side) */
            fill_excitation_vector(ctx, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 
                                  state->excitation_type, ctx->crnt.surface_cur);
        }
        
        /* Matrix solving - Fortran line 60 */
        if (state->processing_stage >= 3) {
            /* Solve for currents using network solver */
            network(ctx, cm, ctx->save.pivot, ctx->crnt.surface_cur);
            ctx->netcx.network_type = 1;  /* Mark network as solved (matches Fortran NTSOL=1) */
            
            state->processing_stage = 4;  /* igo - Done */
        }
        
        } /* End skip matrix operations block */
        
        /* Write all frequency-dependent output (antenna input, currents, power) BEFORE patterns
           Only output once per unique frequency, not for every XQ in that frequency */
        if (state->processing_stage >= 4 && ctx->output_fp &&
            fabs(state->current_frequency_mhz - state->last_freq_output_mhz) > 1e-6) {
            write_frequency_step_output(ctx->output_fp, ctx);
            state->last_freq_output_mhz = state->current_frequency_mhz;
        }
        
        /* Compute and output radiation patterns if collected from look-ahead */
        if (state->processing_stage >= 4 && state->num_rp_cards > 0 && ctx->gnd.far_field_type != -1) {
            ctx->fpat.power_in = ctx->netcx.power_in;
            ctx->fpat.network_loss = ctx->netcx.power_net_loss;
            
            /* Process each collected RP card */
            for (int rp_idx = 0; rp_idx < state->num_rp_cards; rp_idx++) {
                /* Set up field pattern structure for this RP card */
                ctx->fpat.num_theta = state->rp_cards[rp_idx].num_theta;
                ctx->fpat.num_phi = state->rp_cards[rp_idx].num_phi;
                ctx->fpat.theta_start = state->rp_cards[rp_idx].theta_start;
                ctx->fpat.phi_start = state->rp_cards[rp_idx].phi_start;
                ctx->fpat.theta_step = state->rp_cards[rp_idx].theta_step;
                ctx->fpat.phi_step = state->rp_cards[rp_idx].phi_step;
                
                /* Compute radiation pattern */
                compute_radiation_pattern(ctx);
                
                if (ctx->rpat.num_points > 0 && ctx->output_fp) {
                    /* Output the pattern if computation succeeded */
                    write_single_radiation_pattern(ctx->output_fp, ctx);
                    
                    /* Free pattern points for next RP card */
                    if (ctx->rpat.points != NULL) {
                        mem_free(ctx, (void **)&ctx->rpat.points);
                        ctx->rpat.points = NULL;
                    }
                    ctx->rpat.num_points = 0;
                }
            }
        }
        
    } /* End frequency loop - Fortran line 120 */
    
    /* Restore geometry to original unscaled values - Fortran line 121 */
    restore_geometry(ctx, (const card_state_t *)state);
    
    /* Free matrix (keep pivot in context for potential reuse) */
    mem_free(ctx, (void *)&cm);
    
    /* Free frequency loop storage */
    free_frequency_loop_storage(ctx);
    
    /* After frequency loop, ready for next card */
    state->processing_stage = 5;
    
    return 0;
}
