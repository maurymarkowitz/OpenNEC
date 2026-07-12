/*
 * reporting.c - produces NEC-2 style output reports for OpenNEC
 *
 * This code replaces the implementation found in output.c, although
 * it also uses many of the work methods found there. The key difference
 * here is that it processes the entire instruction section of the deck
 * in a single loop, instead of trying to break the instructions up into
 * batches. It turns out that certain cards (like EX) can be repeated
 * across frequency steps, so getting the batch processing correct is tricky.
 * This version returns to the original Fortran layout, and differs mostly
 * in the names of the functions and status variables to make them more
 * obvious than things like "igo" and "iflow".
 *
 * References:
 * - Fortran NEC: nec2dxs.f (lines 14, 40-120, 293-307)
 * - C port nec2c: main.c (lines 241, 306-580, 607-2025)
 */

#include "reporting.h"
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
    if (!ctx || !deck) return -1;
    
    /* Step 1: Calculate geometry if not already done (like run_simulation does) */
    if (ctx->geometry.num_segs == 0 && ctx->geometry.num_patches == 0) {
        errors_list_t geometry_errors = {0};
        calculate_geometry(ctx, deck, &geometry_errors, &ctx->outputs);
        
        if (geometry_errors.num_errors > 0) {
            transfer_errors(&geometry_errors, &ctx->errors);
            return -1;
        }
    }
    
    /* Need to declare and call calculation_defaults after geometry */
    /* This initializes num_eq_sym and other matrix parameters */
    if (ctx->geometry.num_segs > 0 || ctx->geometry.num_patches > 0) {
        /* This is necessary even though it's also called by run_simulation */
        /* because process_deck_sequential may be called independently */
        if (ctx->netcx.num_eq_sym == 0) {
            /* Initialize matrix parameters from geometry */
            ctx->netcx.num_eq_sym = ctx->geometry.num_segs_sym + 2 * ctx->geometry.num_patches_sym;
            if (ctx->netcx.num_eq == 0) {
                ctx->netcx.num_eq = ctx->geometry.num_segs + 2 * ctx->geometry.num_patches;
            }
        }
    }
    
    card_state_t state;
    init_card_state(&state);
    
    /* Output preamble (header, comments, structure, segmentation data)
     * This should happen once at the very start, before any frequency processing */
    if (ctx->output_fp) {
        write_nec_preamble(ctx, deck, ctx->output_fp);
    }
    
    /* Start processing cards after geometry section */
    int start_idx = (deck->geometry_end >= 0) ? deck->geometry_end + 1 : 0;
    int end_idx = (deck->deck_end >= 0) ? deck->deck_end : deck->num_cards;
    
    if (start_idx >= end_idx) {
        /* No control cards to process */
        free_card_state(&state);
        return 0;
    }
    
    /* Main card processing loop - Fortran line 14 */
    for (int i = start_idx; i < end_idx; i++) {
        card_t *card = &deck->cards[i];
        
        /* Skip comments and ignored cards (but write them if configured) */
        if (card->ignore || is_comment(card)) {
            if (ctx->output_fp && is_comment(card)) {
                fprintf(ctx->output_fp, "%s\n", card->card_str ? card->card_str : "");
            }
            continue;
        }
        
        /* Dispatch to card-specific handler */
        int result = dispatch_card(ctx, deck, i, &state);
        if (result != 0) {
            free_card_state(&state);
            return result;
        }
        
        /* Check for EN (end of deck) card */
        if (strcmp(card->card_code, "EN") == 0) {
            break;
        }
    }
    
    /* Output footer (EN card echo and runtime) */
    if (ctx->output_fp) {
        write_footer(ctx->output_fp, ctx, deck);
    }
    
    free_card_state(&state);
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

/* ============================================================================
 * Card Dispatcher
 * ========================================================================== */
static int dispatch_card(context_t *ctx, deck_t *deck, int card_idx,
                        card_state_t *state)
{
    if (!ctx || !deck || card_idx < 0 || card_idx >= deck->num_cards || !state) {
        return -1;
    }
    
    card_t *card = &deck->cards[card_idx];
    const char *code = card->card_code;
    
    /* Write card echo to output if configured */
    if (ctx->output_fp) {
        state->total_cards_processed++;
        fprintf(ctx->output_fp,
            "\n  DATA CARD No: %3d "
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
    if (!ctx) return -1;
    
    /* Validate and add to loading impedance system */
    if (ctx->zload.num_loads >= 200) {  /* Reasonable limit */
        add_error(ctx, &ctx->errors, "Too many loading impedances", WARNING);
        return -1;
    }
    
    int idx = ctx->zload.num_loads++;
    ctx->zload.load_types[idx] = ldtyp;
    ctx->zload.load_tags[idx] = ldtag;
    ctx->zload.load_tag_from[idx] = ldtagf;
    ctx->zload.load_tag_to[idx] = ldtagt;
    ctx->zload.load_r[idx] = zlr;
    ctx->zload.load_l[idx] = zli;
    ctx->zload.load_c[idx] = zlc;
    ctx->zload.load_freq[idx] = 0.0;  /* Use first FR card frequency */
    
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
 * Reset coupling buffers (wrapper)
 */
static void reset_coupling_buffers(context_t *ctx)
{
    if (!ctx) return;
    ctx->yparm.num_pairs = 0;
    ctx->yparm.coupling_flag = 0;
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
}

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
    if (!ctx || !card || !state) return -1;
    
    int ldtyp = card->i[1];
    
    if (state->card_sequence_state != 3) {
        /* First LD card - reset loading buffers */
        reset_loading_buffers_seq(ctx);
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
    int result = add_loading(ctx, ldtyp, ldtag, ldtagf, ldtagt, zlr, zli, zlc);
    
    return result;
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
    int seg1 = card->i[2];
    int tag2 = card->i[3];
    int seg2 = card->i[4];
    
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
    
    /* If XQ has a parameter, it may set default radiation pattern */
    if (card->i[1] != 0) {
        state->card_sequence_state = 10;  /* iflow - XQ card */
    }
    
    /* Execute frequency loop */
    int result = execute_frequency_loop_sequential(ctx, deck, card_idx, state);
    
    /* CRITICAL FIX: Reset card_sequence_state after XQ execution so that
     * the next XQ block's NT cards will properly reset network buffers.
     * Without this, multi-case decks (multiple FR/NT/XQ blocks) reuse stale
     * network data from the first XQ block instead of reloading for each case. */
    if (result == 0) {
        state->card_sequence_state = 5;  /* Reset to allow next NT block to initialize */
    }
    
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
    if (!ctx || !deck || !state) return -1;
    
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
        
        /* Scale geometry for current frequency - Fortran lines 44-307 */
        scale_geometry_for_frequency(ctx, state, fr);
        
        /* Set wavelength in context for calculations */
        geom->wavelength = wlam;
        
        /* Process based on processing_stage state - Fortran line 40: GO TO (41,46,53,71,78), processing_stage */
        
        /* processing_stage=2: Structure loading - Fortran label 46 (line 146) */
        if (state->processing_stage >= 2) {
            /* Apply loading to impedance matrix - TODO: integrate with load arrays */
            /* if (apply_impedance_loading(ctx, ldtyp, ldtag, ldtagf, ldtagt, zlr, zli, zlc) != 0) {
                restore_geometry(ctx, (const card_state_t *)state);
                return -1;
            } */
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
            
            fprintf(stderr, "[DEBUG-reporting] Before fill_excitation_vector: num_vsrcs=%d, num_qdsrcs=%d\n",
                    ctx->vsorc.num_vsrcs, ctx->vsorc.num_qdsrcs);
            
            /* Fill excitation vector (right-hand side) */
            fill_excitation_vector(ctx, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 
                                  state->excitation_type, ctx->crnt.surface_cur);
            
            fprintf(stderr, "[DEBUG-reporting] After fill_excitation_vector: surface_cur[0]=(%.6e,%.6e)\n",
                    creal(ctx->crnt.surface_cur[0]), cimag(ctx->crnt.surface_cur[0]));
        }
        
        /* Matrix solving - Fortran line 60 */
        if (state->processing_stage >= 3) {
            /* Solve for currents using network solver */
            fprintf(stderr, "[DEBUG-reporting] Before network(): einc[0]=(%.6e,%.6e)\n",
                    creal(ctx->crnt.surface_cur[0]), cimag(ctx->crnt.surface_cur[0]));
            
            network(ctx, cm, ctx->save.pivot, ctx->crnt.surface_cur);
            
            fprintf(stderr, "[DEBUG-reporting] After network(): einc[0]=(%.6e,%.6e)\n",
                    creal(ctx->crnt.surface_cur[0]), cimag(ctx->crnt.surface_cur[0]));
            
            state->processing_stage = 4;  /* igo - Done */
        }
        
        /* Write all frequency-dependent output (antenna input, currents, power, patterns) */
        if (state->processing_stage >= 4 && ctx->output_fp) {
            write_frequency_step_output(ctx->output_fp, ctx);
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
