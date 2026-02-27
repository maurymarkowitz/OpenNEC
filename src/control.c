/******************************************************************************
 * control.c
 *
 * Control card processing for OpenNEC. This module processes the control
 * cards (FR, LD, GN, EX, NT, TL, XQ, GD, RP, NX, PT, KH, NE, NH, PQ, EK, 
 * CP, PL, EN) that configure the calculation parameters in the context.
 *
 * This is extracted from the old main.c card input loop (lines 365-940).
 *
 *****************************************************************************/

#include "internals.h"
#include "control.h"
#include "geometry.h"
#include "matrix.h"
#include "network.h"
#include "somnec.h"
#include "fields.h"
#include "radiation.h"
#include "calculations.h"
#include "output.h"

// Forward declarations for static functions
static int nec_calculation_defaults(nec_context_t *ctx);
static int execute_frequency_loop(nec_context_t *ctx, int nfrq, int ifrq, double delfrq);
static void reset_loading_buffers(nec_context_t *ctx);
static void reset_network_buffers(nec_context_t *ctx);
static void reset_coupling_buffers(nec_context_t *ctx);
static void reset_vsorc_buffers(nec_context_t *ctx);
static int process_next_batch(nec_context_t *ctx, deck_t *deck, int *batch_start, int *batch_end);
static int count_tag_segments(const nec_context_t *ctx, int tag);
static int resolve_pct_segment(const nec_context_t *ctx, const card_t *card, int field_idx, int tag);

/******************************************************************************
 * count_tag_segments()
 *
 * Returns the total number of geometry segments carrying the given tag.
 * tag == 0 means all segments (NEC convention).
 */
static int count_tag_segments(const nec_context_t *ctx, int tag)
{
    if (tag == 0) return ctx->geometry.n;
    int count = 0;
    for (int i = 0; i < ctx->geometry.n; i++) {
        if (ctx->geometry.tag_nums[i] == tag) count++;
    }
    return count;
}

/******************************************************************************
 * resolve_pct_segment()
 *
 * 4nec2 allows specifying a segment as a percentage of the wire's total
 * segment count, e.g. "50%" on an EX card means the middle segment.
 *
 * If the I<field_idx> formula ends with '%', this computes:
 *   seg = round(pct / 100.0 * count_tag_segments(tag)), clamped to [1, count].
 * Otherwise returns card->i[field_idx] unchanged.
 */
static int resolve_pct_segment(const nec_context_t *ctx, const card_t *card,
                               int field_idx, int tag)
{
    if (!card->int_form_inline[field_idx]) return card->i[field_idx];

    char key[3] = { 'I', (char)('0' + field_idx), '\0' };
    const key_value_t *kv = card->formulas;
    while (kv) {
        if (kv->key && strcmp(kv->key, key) == 0 && kv->value) {
            size_t vlen = strlen(kv->value);
            if (vlen > 1 && kv->value[vlen - 1] == '%') {
                double pct = strtod(kv->value, NULL);
                int count = count_tag_segments(ctx, tag);
                if (count <= 0) return card->i[field_idx];
                int seg = (int)round(pct / 100.0 * (double)count);
                if (seg < 1) seg = 1;
                if (seg > count) seg = count;
                return seg;
            }
            break;
        }
        kv = kv->next;
    }
    return card->i[field_idx];
}

/******************************************************************************
 * nec_run_simulation()
 *
 * Complete wrapper function for running an NEC simulation from a parsed deck.
 * This is the main entry point for library usage (e.g., from Swift).
 *
 * Performs all steps in order:
 * 1. Calculate geometry (wires, patches)
 * 2. Initialize calculation defaults
 * 3. Process control cards (FR, LD, GN, EX, etc.)
 * 4. Execute frequency loop calculations
 *
 * All errors are accumulated in ctx->errors for the caller to handle.
 *
 * @param ctx     The NEC context (must be initialized)
 * @param deck    The deck containing geometry and control cards (must be parsed)
 * @return        0 on success, -1 on error (check ctx->errors for details)
 */
int nec_run_simulation(nec_context_t *ctx, deck_t *deck)
{
    errors_list_t geometry_errors = {0};

    // Step 1: Calculate geometry.
    // Skip if already populated — nec_estimate_time() may have done this as a
    // side effect (e.g. a GUI calling the estimator before launching the run).
    if (ctx->geometry.n == 0 && ctx->geometry.m == 0) {
        calculate_geometry(ctx, deck, &geometry_errors, &ctx->outputs);

        // Check for geometry errors
        if (geometry_errors.num_errors > 0) {
            // Transfer already-logged errors without re-printing them
            transfer_errors(&geometry_errors, &ctx->errors);
            return -1;
        }
    }
    
    // Step 2: Initialize calculation defaults (requires valid geometry)
    if (nec_calculation_defaults(ctx) != 0) {
        add_error(ctx, &ctx->errors, "Failed to initialize calculation defaults (no valid geometry)", FATAL);
        return -1;
    }
    
    // Step 3: Initialize batch processing state
    ctx->current_card_idx = deck->geometry_end + 1;  // Start after GE card
    if (ctx->current_card_idx >= deck->num_cards) {
        // No control cards after GE - deck is complete
        return 0;
    }
    ctx->card_number_offset = 0;  // Card numbering starts at 0
    ctx->iflow = 0;  // Initial state
    
    // Step 4: Process batches in a loop
    bool deck_complete = false;
    while (!deck_complete) {
        int batch_start, batch_end;
        
        // Process next batch of control cards
        int batch_result = process_next_batch(ctx, deck, &batch_start, &batch_end);
        if (batch_result < 0) {
            // Error occurred
            return -1;
        }
        
        // Save batch boundaries in context for output
        ctx->batch_start_card = batch_start;
        ctx->batch_end_card = batch_end;
        
        // Check if this batch is EN or XT (no calculations)
        bool is_termination = false;
        if (batch_start == batch_end) {
            card_t *card = &deck->cards[batch_start];
            if (strcmp(card->card_code, "EN") == 0 || strcmp(card->card_code, "XT") == 0) {
                is_termination = true;
            }
            if (strcmp(card->card_code, "XT") == 0) {
                ctx->xt_terminated = true;
            }
        }
        
        // Execute frequency loop only when an output request card is present in the batch.
        // This matches Fortran/nec2c behavior: the frequency loop (and CALL LOAD) is only
        // entered when RP, NE, NH, XQ, or WG is encountered. FR/LD/GN/EX-only batches
        // (e.g., ending with EN) accumulate state but perform no computation.
        bool has_xq = (!is_termination && batch_end >= 0 &&
                       strcmp(deck->cards[batch_end].card_code, "XQ") == 0);
        bool has_output_request = (ctx->gnd.ifar != -1 ||
                                   ctx->fpat.near != -1 ||
                                   has_xq ||
                                   ctx->wg_after_cmset);
        if (!is_termination && has_output_request) {
            ctx->frequency_loop_ran = true;
            if (execute_frequency_loop(ctx, ctx->save.nfrq, ctx->save.ifrq, ctx->save.delfrq) != 0) {
                return -1;
            }
        }
        
        // Handle NX restart: flush section output, reset state, restart with next section
        if (batch_result == 2) {
            int nx_pos = batch_end;

            /* Step 1: Scan forward from NX+1 for all new section boundaries. */
            int new_comment_start = -1, new_comment_end = -1;
            int new_sym_start = -1,     new_sym_end = -1;
            int new_geom_start = -1,    new_geom_end = -1;
            int new_deck_end   = -1;
            bool in_new_geom   = false;

            for (int i = nx_pos + 1; i < deck->num_cards; i++) {
                card_t *c = &deck->cards[i];
                if (c->ignore) continue;
                if (is_comment(c)) {
                    if (new_comment_start == -1) new_comment_start = i;
                    new_comment_end = i;
                    continue;
                }
                if (strcmp(c->card_code, "SY") == 0 && !in_new_geom) {
                    if (new_sym_start == -1) new_sym_start = i;
                    new_sym_end = i;
                    continue;
                }
                if (is_geometry(c)) {
                    in_new_geom = true;
                    if (new_geom_start == -1) new_geom_start = i;
                    new_geom_end = i;
                    if (strcmp(c->card_code, "GE") == 0) break;
                    continue;
                }
                if (!in_new_geom) continue; /* skip stray pre-geometry control cards */
            }
            if (new_geom_end >= 0) {
                for (int i = new_geom_end + 1; i < deck->num_cards; i++) {
                    card_t *c = &deck->cards[i];
                    if (!c->ignore && strcmp(c->card_code, "EN") == 0)
                        { new_deck_end = i; break; }
                }
            }

            if (new_geom_start == -1 || new_geom_end == -1 ||
                strcmp(deck->cards[new_geom_end].card_code, "GE") != 0) {
                /* No geometry follows NX — treat as terminal (like EN).
                 * This covers:
                 *   - NX as the last card in the deck
                 *   - NX followed only by EN (no new geometry section) */
                if (ctx->output_fp != NULL && ctx->frequency_loop_ran) {
                    deck->deck_end = nx_pos;
                    write_nec_output(ctx, deck, ctx->output_fp);
                    deck->deck_end = -1;
                    ctx->frequency_loop_ran = false; /* prevent double write in main */
                }
                deck_complete = true;
                break;
            }

            /* Step 2: Flush output for the completed section.
             * Temporarily set deck_end to the NX card so write_input_cards
             * prints section 1's control cards (FR/EX/RP/NX range). */
            if (ctx->output_fp != NULL && ctx->frequency_loop_ran) {
                deck->deck_end = nx_pos;
                write_nec_output(ctx, deck, ctx->output_fp);
                deck->deck_end = -1;  /* restore: section 1 has no EN */
            }

            /* Step 3: Reset all per-section simulation state. */
            reset_loading_buffers(ctx);
            reset_network_buffers(ctx);
            reset_coupling_buffers(ctx);
            reset_vsorc_buffers(ctx);

            if (ctx->rpat.points != NULL) { free(ctx->rpat.points); ctx->rpat.points = NULL; }
            ctx->rpat.num_points = 0;

            if (ctx->yparm.coupling_rows != NULL) {
                free(ctx->yparm.coupling_rows);
                ctx->yparm.coupling_rows = NULL;
                ctx->yparm.num_coupling_rows = 0;
                ctx->yparm.coupling_rows_cap = 0;
            }

            if (ctx->ngf_cm != NULL) { free(ctx->ngf_cm); ctx->ngf_cm = NULL; }
            ctx->has_ngf = false; ctx->ngf_n_segs = 0; ctx->ngf_neq = 0; ctx->ngf_fmhz = 0.0;

            /* Step 4: Update deck section pointers to the new section and re-run geometry. */
            deck->comment_start  = new_comment_start;
            deck->comment_end    = new_comment_end;
            deck->symbol_start   = new_sym_start;
            deck->symbol_end     = new_sym_end;
            deck->geometry_start = new_geom_start;
            deck->geometry_end   = new_geom_end;
            deck->deck_end       = new_deck_end;

            errors_list_t nx_geom_errors = {0};
            calculate_geometry(ctx, deck, &nx_geom_errors, &ctx->outputs);
            if (nx_geom_errors.num_errors > 0) {
                for (int i = 0; i < nx_geom_errors.num_errors; i++)
                    add_error(ctx, &ctx->errors, nx_geom_errors.errors[i].message,
                             nx_geom_errors.errors[i].severity);
                return -1;
            }
            if (nec_calculation_defaults(ctx) != 0) {
                add_error(ctx, &ctx->errors,
                    "NX: failed to initialize calculation defaults for new section", FATAL);
                return -1;
            }

            ctx->current_card_idx = new_geom_end + 1;
            ctx->iflow = 0;
            continue;
        }

        // Check if we're done
        if (batch_result == 1) {
            deck_complete = true;
        }
    }
    
    return 0;
}

/******************************************************************************
 * nec_calculation_defaults()
 *
 * Initialize calculation defaults that depend on geometry being calculated first.
 * This should be called after calculate_geometry() and before process_control_cards().
 *
 * Sets geometry-dependent values and resets all calculation parameters to their
 * initial state to support multiple calculation runs.
 *
 * @param ctx     The NEC context to initialize
 * @return        0 on success, -1 on error (no geometry)
 */
static int nec_calculation_defaults(nec_context_t *ctx)
{
    // validate that geometry has been calculated
    if (ctx->geometry.np <= 0 && ctx->geometry.mp <= 0) {
        return -1;
    }
    
    // set geometry-dependent matrix parameters
    ctx->netcx.npeq = ctx->geometry.np + 2 * ctx->geometry.mp;
    
    // matrix parameters (from oldmain.c lines 289-292)
    if (ctx->matpar.imat == 0) {
        ctx->netcx.neq = ctx->geometry.n + 2 * ctx->geometry.m;
        ctx->netcx.neq2 = 0;
    }
    
    // reset all calculation defaults to initial values
    // these are reset for each run to support multiple calculations
    ctx->fpat.ixtyp = 0;
    ctx->fpat.near = -1;
    ctx->zload.nload = 0;
    ctx->loading_outputs.count = 0;
    ctx->loading_outputs.capacity = 0;
    ctx->loading_outputs.entries = NULL;
    ctx->netcx.nonet = 0;
    ctx->plot.iplp1 = 0;
    ctx->plot.iplp2 = 0;
    ctx->plot.iplp3 = 0;
    ctx->plot.iplp4 = 0;
    ctx->yparm.ncoup = 0;
    ctx->yparm.icoup = 0;
    ctx->gnd.iperf = 0;
    ctx->gnd.nradl = 0;
    ctx->dataj.rkh = 1.0;  // Default matrix integration limit
    ctx->dataj.iexk = 0;   // Extended thin-wire kernel off by default
    ctx->gnd.ifar = -1;
    ctx->frequency_loop_ran = false;
    
    // Note: The following old main.c local variables are not stored in ctx
    // as they were only used for local flow control:
    //   igo    - execution flow control flag
    //   nfrq   - frequency loop counter
    //   rkh    - wave number parameter (k*h)
    //   iexk   - extended thin-wire kernel flag
    //   iped   - impedance print flag
    //   iptflg - pattern output control flag
    //   iptflq - pattern output control flag
    //   mpcnt  - command card counter
    
    return 0;
}

/******************************************************************************
 * reset_loading_buffers()
 *
 * Reset and free loading buffers. Called when starting a new batch.
 */
static void reset_loading_buffers(nec_context_t *ctx)
{
    if (ctx->zload.nload > 0) {
        mem_free(ctx, (void **)&ctx->zload.ldtyp);
        mem_free(ctx, (void **)&ctx->zload.ldtag);
        mem_free(ctx, (void **)&ctx->zload.ldtagf);
        mem_free(ctx, (void **)&ctx->zload.ldtagt);
        mem_free(ctx, (void **)&ctx->zload.zlr);
        mem_free(ctx, (void **)&ctx->zload.zli);
        mem_free(ctx, (void **)&ctx->zload.zlc);
        ctx->zload.nload = 0;
    }
    if (ctx->loading_outputs.entries != NULL) {
        mem_free(ctx, (void **)&ctx->loading_outputs.entries);
        ctx->loading_outputs.count = 0;
        ctx->loading_outputs.capacity = 0;
    }
}

/******************************************************************************
 * reset_network_buffers()
 *
 * Reset and free network buffers. Called when starting a new batch.
 */
static void reset_network_buffers(nec_context_t *ctx)
{
    if (ctx->netcx.nonet > 0) {
        mem_free(ctx, (void **)&ctx->netcx.ntyp);
        mem_free(ctx, (void **)&ctx->netcx.iseg1);
        mem_free(ctx, (void **)&ctx->netcx.iseg2);
        mem_free(ctx, (void **)&ctx->netcx.x11r);
        mem_free(ctx, (void **)&ctx->netcx.x11i);
        mem_free(ctx, (void **)&ctx->netcx.x12r);
        mem_free(ctx, (void **)&ctx->netcx.x12i);
        mem_free(ctx, (void **)&ctx->netcx.x22r);
        mem_free(ctx, (void **)&ctx->netcx.x22i);
        ctx->netcx.nonet = 0;
    }
}

/******************************************************************************
 * reset_coupling_buffers()
 *
 * Reset and free coupling buffers. Called when starting a new batch.
 */
static void reset_coupling_buffers(nec_context_t *ctx)
{
    if (ctx->yparm.ncoup > 0) {
        mem_free(ctx, (void **)&ctx->yparm.nctag);
        mem_free(ctx, (void **)&ctx->yparm.ncseg);
        ctx->yparm.ncoup = 0;
    }
}

/******************************************************************************
 * reset_vsorc_buffers()
 *
 * Reset and free excitation source (vsorc) buffers. Called on NX restart.
 */
static void reset_vsorc_buffers(nec_context_t *ctx)
{
    if (ctx->vsorc.nsant > 0) {
        mem_free(ctx, (void **)&ctx->vsorc.isant);
        mem_free(ctx, (void **)&ctx->vsorc.vsant);
        ctx->vsorc.nsant = 0;
    }
    if (ctx->vsorc.nvqd > 0) {
        mem_free(ctx, (void **)&ctx->vsorc.ivqd);
        mem_free(ctx, (void **)&ctx->vsorc.iqds);
        mem_free(ctx, (void **)&ctx->vsorc.vqd);
        mem_free(ctx, (void **)&ctx->vsorc.vqds);
        ctx->vsorc.nvqd = 0;
        ctx->vsorc.nqds = 0;
    }
}

/******************************************************************************
 * process_next_batch()
 *
 * Process control cards from current position up to next XQ, EN, XT, or NX
 * card. Updates batch boundaries in context and handles iflow state transitions.
 *
 * @param ctx          The NEC context
 * @param deck         The deck containing control cards
 * @param batch_start  Output: first card index of this batch (inclusive)
 * @param batch_end    Output: last card index of this batch (inclusive)
 * @return             0 on success, -1 on error, 1 if EN/XT reached (end of deck), 2 if NX restart
 */
static int process_next_batch(nec_context_t *ctx, deck_t *deck, int *batch_start, int *batch_end)
{
    // Validate inputs
    if (ctx == NULL || deck == NULL || batch_start == NULL || batch_end == NULL) {
        return -1;
    }
    
    // Check if we've reached the end of the deck
    if (ctx->current_card_idx >= deck->num_cards) {
        return 1;  // End of deck reached
    }
    
    // Set batch start from current position
    *batch_start = ctx->current_card_idx;
    
    // Find end of batch (next XQ, EN, or XT card)
    *batch_end = ctx->current_card_idx;
    bool found_batch_end = false;
    
    for (int card_idx = ctx->current_card_idx; card_idx < deck->num_cards; card_idx++) {
        card_t *card = &deck->cards[card_idx];
        
        // Skip ignored or comment cards
        if (card->ignore || is_comment(card)) {
            continue;
        }
        
        char *code = card->card_code;
        
        // Check for batch termination cards
        if (strcmp(code, "XQ") == 0) {
            *batch_end = card_idx;  // Include XQ in this batch
            ctx->current_card_idx = card_idx + 1;  // Next batch starts after XQ
            found_batch_end = true;
            break;
        }
        else if (strcmp(code, "EN") == 0 || strcmp(code, "XT") == 0 || strcmp(code, "NX") == 0) {
            // EN/XT/NX should be a separate batch by itself
            if (card_idx > ctx->current_card_idx) {
                // There are cards before EN/XT, end batch before EN/XT
                *batch_end = card_idx - 1;
                ctx->current_card_idx = card_idx;  // Next batch starts at EN/XT
                found_batch_end = true;
                break;
            } else {
                // EN/XT is the first/only card in this batch
                *batch_end = card_idx;
                ctx->current_card_idx = card_idx + 1;
                found_batch_end = true;
                break;
            }
        }
        
        *batch_end = card_idx;
    }
    
    if (!found_batch_end) {
        // Reached end of deck without XQ/EN/XT - treat as implicit EN
        ctx->current_card_idx = deck->num_cards;
    }
    
    // Determine if this is the final batch (EN or XT)
    bool is_final_batch = false;
    if (*batch_end < deck->num_cards) {
        card_t *last_card = &deck->cards[*batch_end];
        if (!is_comment(last_card) && !last_card->ignore) {
            if (strcmp(last_card->card_code, "EN") == 0 || strcmp(last_card->card_code, "XT") == 0) {
                is_final_batch = true;
            }
        }
    }
    if (!found_batch_end) {
        is_final_batch = true;  // Implicit EN at end of deck
    }
    
    // Now process the control cards in this batch to configure ctx
    for (int card_idx = *batch_start; card_idx <= *batch_end; card_idx++) {
        card_t *card = &deck->cards[card_idx];
        
        // Skip ignored, comment, or empty cards
        if (card->ignore || is_comment(card)) {
            continue;
        }
        
        char *code = card->card_code;
        
        // Check if this is a SY card - if so, evaluate its formulas and continue to next card
        if (strcmp(code, "SY") == 0) {
            if (card->formulas) {
                key_value_t *kv = card->formulas;
                while (kv) {
                    evaluate_formula(ctx, kv, deck, &ctx->errors);
                    kv = kv->next;
                }
            }
            continue; // Skip to next card, SY cards don't configure anything
        }
        
        // Skip XQ, EN, XT, NX cards (they don't configure anything)
        if (strcmp(code, "XQ") == 0 || strcmp(code, "EN") == 0 || strcmp(code, "XT") == 0 || strcmp(code, "NX") == 0) {
            continue;
        }
        
        // Get field values for convenience
        int i1 = card->i[1], i2 = card->i[2], i3 = card->i[3], i4 = card->i[4];
        double f1 = card->f[1], f2 = card->f[2], f3 = card->f[3];
        double f4 = card->f[4], f5 = card->f[5], f6 = card->f[6];
        
        // Process based on card type
        if (strcmp(code, "FR") == 0) {
            // FR card - Frequency specification
            if (ctx->iflow != 1) {
                ctx->iflow = 1;
            }
            ctx->save.ifrq = i1;
            ctx->save.nfrq = (i2 == 0) ? 1 : i2;
            ctx->save.fmhz = f1;
            ctx->save.delfrq = f2;
        }
        else if (strcmp(code, "LD") == 0) {
            // LD card - Loading
            if (i1 == -1) {
                continue;
            }

            if (i1 > 5) {
                char msg[MAX_ERROR_LEN];
                snprintf(msg, sizeof(msg), "Card %d is an LD card with type %d, which is not supported.", card_idx + 1, i1);
                add_error(ctx, &ctx->errors, msg, FATAL);
                return -1;
            }
            
            // First LD in batch resets loading (iflow transition to 3)
            if (ctx->iflow != 3 && ctx->zload.nload == 0) {
                reset_loading_buffers(ctx);
                ctx->iflow = 3;
            }
            
            // Reallocate loading buffers
            ctx->zload.nload++;
            size_t mreq = (size_t)ctx->zload.nload * sizeof(int);
            mem_realloc(ctx, (void **)&ctx->zload.ldtyp, mreq);
            mem_realloc(ctx, (void **)&ctx->zload.ldtag, mreq);
            mem_realloc(ctx, (void **)&ctx->zload.ldtagf, mreq);
            mem_realloc(ctx, (void **)&ctx->zload.ldtagt, mreq);
            
            mreq = (size_t)ctx->zload.nload * sizeof(double);
            mem_realloc(ctx, (void **)&ctx->zload.zlr, mreq);
            mem_realloc(ctx, (void **)&ctx->zload.zli, mreq);
            mem_realloc(ctx, (void **)&ctx->zload.zlc, mreq);
            
            int idx = ctx->zload.nload - 1;
            ctx->zload.ldtyp[idx] = i1;
            ctx->zload.ldtag[idx] = i2;
            ctx->zload.ldtagf[idx] = (i4 == 0) ? i3 : i3;
            ctx->zload.ldtagt[idx] = (i4 == 0) ? i3 : i4;
            
            if (ctx->zload.ldtagt[idx] < ctx->zload.ldtagf[idx]) {
                char msg[MAX_ERROR_LEN];
                snprintf(msg, sizeof(msg),
                    "DATA FAULT ON LOADING CARD No: %d: ITAG "
                    "STEP1: %d IS GREATER THAN ITAG STEP2: %d",
                    ctx->zload.nload, i3, i4);
                add_error(ctx, &ctx->errors, msg, FATAL);
                return -1;
            }
            
            ctx->zload.zlr[idx] = f1;
            ctx->zload.zli[idx] = f2;
            ctx->zload.zlc[idx] = f3;
        }
        else if (strcmp(code, "GN") == 0) {
            // GN card - Ground parameters  
            if (i1 == -1) {
                ctx->gnd.ksymp = 1;
                ctx->gnd.nradl = 0;
                ctx->gnd.iperf = 0;
                continue;
            }
            
            ctx->gnd.iperf = i1;
            ctx->gnd.nradl = i2;
            ctx->gnd.ksymp = 2;
            ctx->save.epsr = f1;
            ctx->save.sig = f2;
            
            if (ctx->gnd.nradl != 0) {
                if (ctx->gnd.iperf == 2) {
                    add_error(ctx, &ctx->errors,
                        "RADIAL WIRE G.S. APPROXIMATION MAY "
                        "NOT BE USED WITH SOMMERFELD GROUND OPTION", FATAL);
                    return -1;
                }
                if (f3 >= 1.0e-20 || f4 >= 1.0e-20) {
                    ctx->save.scrwlt = f3;
                    ctx->save.scrwrt = f4;
                }
            }
        }
        // Continue processing other cards...
        else if (strcmp(code, "EX") == 0) {
            // EX card - Excitation
            ctx->fpat.ixtyp = i1;
            ctx->netcx.masym = i4 / 10;
            
            // warn about unsupported EX types
            if (i1 == 6 || i1 == 7) {
                char msg[MAX_ERROR_LEN];
                snprintf(msg, sizeof(msg), "Card %d is an EX card with type %d, which is not supported.", card_idx + 1, i1);
                add_error(ctx, &ctx->errors, msg, WARNING);
            }
            
            // For voltage source types (0 and 5)
            if (i1 == 0 || i1 == 5) {
                ctx->netcx.ntsol = 0;
                
                if (i1 == 5) {
                    // Incident plane wave or elementary current source
                    ctx->vsorc.nvqd++;
                    size_t mreq = (size_t)ctx->vsorc.nvqd * sizeof(int);
                    mem_realloc(ctx, (void **)&ctx->vsorc.ivqd, mreq);
                    mem_realloc(ctx, (void **)&ctx->vsorc.iqds, mreq);
                    
                    mreq = (size_t)ctx->vsorc.nvqd * sizeof(complex double);
                    mem_realloc(ctx, (void **)&ctx->vsorc.vqd, mreq);
                    mem_realloc(ctx, (void **)&ctx->vsorc.vqds, mreq);
                    
                    int idx = ctx->vsorc.nvqd - 1;
                    int i3_resolved = resolve_pct_segment(ctx, card, 3, i2);
                    int seg_num = segment_number(ctx, i2, i3_resolved);
                    if (seg_num == 0) {
                        char msg[MAX_ERROR_LEN];
                        snprintf(msg, sizeof(msg), "Card %d is an EX that references invalid tag %d, segment %d", card_idx + 1, i2, i3_resolved);
                        add_error(ctx, &ctx->errors, msg, FATAL);
                        return -1;
                    }
                    ctx->vsorc.ivqd[idx] = seg_num;
                    ctx->vsorc.vqd[idx] = f1 + I * f2;
                    if (cabs(ctx->vsorc.vqd[idx]) < 1.e-20) {
                        ctx->vsorc.vqd[idx] = CPLX_10;
                    }
                } else {
                    // Applied voltage source
                    ctx->vsorc.nsant++;
                    size_t mreq = (size_t)ctx->vsorc.nsant * sizeof(int);
                    mem_realloc(ctx, (void **)&ctx->vsorc.isant, mreq);
                    
                    mreq = (size_t)ctx->vsorc.nsant * sizeof(complex double);
                    mem_realloc(ctx, (void **)&ctx->vsorc.vsant, mreq);
                    
                    int idx = ctx->vsorc.nsant - 1;
                    int i3_resolved = resolve_pct_segment(ctx, card, 3, i2);
                    int seg_num = segment_number(ctx, i2, i3_resolved);
                    if (seg_num == 0) {
                        char msg[MAX_ERROR_LEN];
                        snprintf(msg, sizeof(msg), "Card %d is an EX that references invalid tag %d, segment %d", card_idx + 1, i2, i3_resolved);
                        add_error(ctx, &ctx->errors, msg, FATAL);
                        return -1;
                    }
                    ctx->vsorc.isant[idx] = seg_num;
                    ctx->vsorc.vsant[idx] = f1 + I * f2;
                    if (cabs(ctx->vsorc.vsant[idx]) < 1.e-20) {
                        ctx->vsorc.vsant[idx] = CPLX_10;
                    }
                }
            } else {
                // Far field pattern for receiving antenna
                ctx->fpat.xpr6 = f6;
                ctx->vsorc.nsant = 0;
                ctx->vsorc.nvqd = 0;
            }
        }
        else if (strcmp(code, "NT") == 0 || strcmp(code, "TL") == 0) {
            // NT/TL cards - Network parameters
            if (i2 == -1) {
                continue;
            }
            
            // First NT/TL in batch resets network (iflow transition to 6)
            if (ctx->iflow != 6 && ctx->netcx.nonet == 0) {
                reset_network_buffers(ctx);
                ctx->iflow = 6;
            }
            
            // Reallocate network buffers
            ctx->netcx.nonet++;
            size_t mreq = (size_t)ctx->netcx.nonet * sizeof(int);
            mem_realloc(ctx, (void **)&ctx->netcx.ntyp, mreq);
            mem_realloc(ctx, (void **)&ctx->netcx.iseg1, mreq);
            mem_realloc(ctx, (void **)&ctx->netcx.iseg2, mreq);
            
            mreq = (size_t)ctx->netcx.nonet * sizeof(double);
            mem_realloc(ctx, (void **)&ctx->netcx.x11r, mreq);
            mem_realloc(ctx, (void **)&ctx->netcx.x11i, mreq);
            mem_realloc(ctx, (void **)&ctx->netcx.x12r, mreq);
            mem_realloc(ctx, (void **)&ctx->netcx.x12i, mreq);
            mem_realloc(ctx, (void **)&ctx->netcx.x22r, mreq);
            mem_realloc(ctx, (void **)&ctx->netcx.x22i, mreq);
            
            int idx = ctx->netcx.nonet - 1;
            if (strcmp(code, "NT") == 0) {
                ctx->netcx.ntyp[idx] = 1;
            } else {
                ctx->netcx.ntyp[idx] = 2;
            }
            
            ctx->netcx.iseg1[idx] = segment_number(ctx, i1, i2);
            if (ctx->netcx.iseg1[idx] == 0) {
                char msg[MAX_ERROR_LEN];
                snprintf(msg, sizeof(msg), "Card %d is a %s that references invalid tag %d, segment %d", card_idx + 1, code, i1, i2);
                add_error(ctx, &ctx->errors, msg, FATAL);
                return -1;
            }
            ctx->netcx.iseg2[idx] = segment_number(ctx, i3, i4);
            if (ctx->netcx.iseg2[idx] == 0) {
                char msg[MAX_ERROR_LEN];
                snprintf(msg, sizeof(msg), "Card %d is a %s that references invalid tag %d, segment %d", card_idx + 1, code, i3, i4);
                add_error(ctx, &ctx->errors, msg, FATAL);
                return -1;
            }
            ctx->netcx.x11r[idx] = f1;
            ctx->netcx.x11i[idx] = f2;
            ctx->netcx.x12r[idx] = f3;
            ctx->netcx.x12i[idx] = f4;
            ctx->netcx.x22r[idx] = f5;
            ctx->netcx.x22i[idx] = f6;
            
            // Check for transmission line with impedance
            if ((ctx->netcx.ntyp[idx] == 2) && (f1 <= 0.0)) {
                ctx->netcx.ntyp[idx] = 3;
                ctx->netcx.x11r[idx] = -f1;
            }
        }
        else if (strcmp(code, "CP") == 0) {
            // CP card - Maximum coupling between antennas
            if (i2 == 0) {
                continue;
            }
            
            // First CP in batch resets coupling (iflow transition to 2)
            if (ctx->iflow != 2 && ctx->yparm.ncoup == 0) {
                reset_coupling_buffers(ctx);
                ctx->iflow = 2;
            }
            
            ctx->yparm.icoup = 0;
            
            // First antenna
            ctx->yparm.ncoup++;
            size_t mreq = (size_t)ctx->yparm.ncoup * sizeof(int);
            mem_realloc(ctx, (void **)&ctx->yparm.nctag, mreq);
            mem_realloc(ctx, (void **)&ctx->yparm.ncseg, mreq);
            ctx->yparm.nctag[ctx->yparm.ncoup - 1] = i1;
            ctx->yparm.ncseg[ctx->yparm.ncoup - 1] = i2;
            
            // Second antenna (if specified)
            if (i4 != 0) {
                ctx->yparm.ncoup++;
                mreq = (size_t)ctx->yparm.ncoup * sizeof(int);
                mem_realloc(ctx, (void **)&ctx->yparm.nctag, mreq);
                mem_realloc(ctx, (void **)&ctx->yparm.ncseg, mreq);
                ctx->yparm.nctag[ctx->yparm.ncoup - 1] = i3;
                ctx->yparm.ncseg[ctx->yparm.ncoup - 1] = i4;
            }
        }
        else if (strcmp(code, "GD") == 0) {
            // GD card - Ground representation (for patterns)
            ctx->fpat.epsr2 = f1;
            ctx->fpat.sig2 = f2;
            ctx->fpat.clt = f3;
            ctx->fpat.cht = f4;
        }
        else if (strcmp(code, "PT") == 0 || strcmp(code, "PQ") == 0 || strcmp(code, "PL") == 0) {
            // These cards are print control - skip in batch processing
            continue;
        }
        else if (strcmp(code, "RP") == 0) {
            // RP card - Radiation pattern parameters
            ctx->gnd.ifar = i1;
            ctx->fpat.nth = (i2 == 0) ? 1 : i2;
            ctx->fpat.nph = (i3 == 0) ? 1 : i3;
            
            ctx->fpat.ipd = i4 / 10;
            ctx->fpat.iavp = i4 - ctx->fpat.ipd * 10;
            ctx->fpat.inor = ctx->fpat.ipd / 10;
            ctx->fpat.ipd = ctx->fpat.ipd - ctx->fpat.inor * 10;
            ctx->fpat.iax = ctx->fpat.inor / 10;
            ctx->fpat.inor = ctx->fpat.inor - ctx->fpat.iax * 10;
            
            if (ctx->fpat.iax != 0) ctx->fpat.iax = 1;
            if (ctx->fpat.ipd != 0) ctx->fpat.ipd = 1;
            if ((ctx->fpat.nth < 2) || (ctx->fpat.nph < 2) || (ctx->gnd.ifar == 1)) {
                ctx->fpat.iavp = 0;
            }
            
            ctx->fpat.thets = f1;
            ctx->fpat.phis = f2;
            ctx->fpat.dth = f3;
            ctx->fpat.dph = f4;
            ctx->fpat.rfld = f5;
            ctx->fpat.gnor = f6;
        }
        else if (strcmp(code, "NE") == 0 || strcmp(code, "NH") == 0) {
            // NE/NH cards - Near field calculation
            ctx->fpat.nfeh = (strcmp(code, "NH") == 0) ? 1 : 0;
            ctx->fpat.near = i1;
            ctx->fpat.nrx = i2;
            ctx->fpat.nry = i3;
            ctx->fpat.nrz = i4;
            ctx->fpat.xnr = f1;
            ctx->fpat.ynr = f2;
            ctx->fpat.znr = f3;
            ctx->fpat.dxnr = f4;
            ctx->fpat.dynr = f5;
            ctx->fpat.dznr = f6;
        }
        else if (strcmp(code, "EK") == 0) {
            // Extended thin-wire kernel
            ctx->dataj.iexk = i1;
        }
        else if (strcmp(code, "KH") == 0) {
            // Matrix integration limit
            ctx->dataj.rkh = f1;
        }
        else if (strcmp(code, "WG") == 0) {
            /* WG FILENAME: write Numerical Green's Function file after cmset.
             * Open the output file now; write_greens_binary() is called in
             * execute_frequency_loop() after the matrix is filled, then the
             * frequency loop exits without factorizing or solving.
             * If no filename is given on the card, derive one from the input
             * deck path by replacing the extension with .ngf (same directory). */
            const char *wg_filename = card->comment;
            char wg_default[MAX_PATH_LEN + 1];
            char wg_resolved[MAX_PATH_LEN + 1];
            if (!wg_filename || *wg_filename == '\0') {
                if (ctx->source_filename) {
                    strncpy(wg_default, ctx->source_filename, MAX_PATH_LEN);
                    wg_default[MAX_PATH_LEN] = '\0';
                    char *dot   = strrchr(wg_default, '.');
                    char *slash = strrchr(wg_default, '/');
                    if (dot && (!slash || dot > slash))
                        *dot = '\0';
                    strncat(wg_default, ".ngf", MAX_PATH_LEN - strlen(wg_default));
                    wg_filename = wg_default;
                } else {
                    char msg[MAX_ERROR_LEN];
                    snprintf(msg, sizeof(msg), "WG card %d has no filename and no input file to derive one from.", card_idx + 1);
                    add_error(ctx, &ctx->errors, msg, FATAL);
                    return -1;
                }
            } else {
                /* Explicit filename: resolve relative to input file's directory */
                resolve_path_relative_to_input(wg_filename, ctx->source_filename,
                                               wg_resolved, sizeof(wg_resolved));
                wg_filename = wg_resolved;
            }
            if (ctx->green_fp != NULL) {
                fclose(ctx->green_fp);
                ctx->green_fp = NULL;
            }
            ctx->green_fp = fopen(wg_filename, "wb");
            if (!ctx->green_fp) {
                char msg[MAX_ERROR_LEN];
                snprintf(msg, sizeof(msg),
                         "WG card %d: cannot open '%s' for writing.", card_idx + 1, wg_filename);
                add_error(ctx, &ctx->errors, msg, FATAL);
                return -1;
            }
            ctx->wg_after_cmset = true;
        }
    }
    
    // Return 2 for NX restart, 1 for final batch (EN/XT), 0 to continue
    if (*batch_end >= 0 && *batch_end < deck->num_cards) {
        card_t *term_card = &deck->cards[*batch_end];
        if (!is_comment(term_card) && !term_card->ignore &&
            strcmp(term_card->card_code, "NX") == 0)
            return 2;  /* NX: start next section */
    }
    return is_final_batch ? 1 : 0;
}

/******************************************************************************
 * execute_frequency_loop()
 *
 * Execute the main frequency loop calculations. This is the core computation
 * that performs matrix fill/factor, network calculations, and field calculations
 * for each frequency point.
 * 
 * This replaces the old main.c frequency do loop (lines 945-1862).
 * Output formatting has been factored out to output.c functions.
 *
 * @param ctx     The NEC context with all calculation parameters
 * @param nfrq    Number of frequency points to calculate
 * @param ifrq    Frequency step type (0=linear, 1=multiplicative)
 * @param delfrq  Frequency step size
 * @return        0 on success, -1 on error
 */
static int execute_frequency_loop(nec_context_t *ctx, int nfrq, int ifrq, double delfrq)
{
    if (ctx == NULL) {
        return -1;
    }
    
    // If no FR card was processed, default to a single frequency run at
    // the context default frequency (CVEL MHz => wavelength = 1 m), matching
    // NEC-2 behaviour when FR is absent.
    if (nfrq == 0) {
        nfrq = 1;
    }
    // Validate geometry exists
    if (ctx->netcx.neq == 0 || ctx->netcx.npeq == 0) {
        add_error(ctx, &ctx->errors, "Geometry not initialized before frequency loop", FATAL);
        return -1;
    }
    
    if (ctx->geometry.n > 0 && (ctx->geometry.icon1 == NULL || ctx->geometry.icon2 == NULL)) {
        add_error(ctx, &ctx->errors, "Geometry connection data not allocated", FATAL);
        return -1;
    }
    
    // Allocate memory for interaction matrix and IP array
    size_t iresrv = ctx->netcx.neq * (ctx->netcx.neq + 2);
    size_t mreq = iresrv * sizeof(complex double);
    complex double *cm = NULL;
    mem_alloc(ctx, (void **)&cm, mreq);
    
    mreq = ctx->netcx.neq * sizeof(int);
    mem_alloc(ctx, (void **)&ctx->save.ip, mreq);
    
    // Allocate symmetry array
    ctx->smat.nop = ctx->netcx.neq / ctx->netcx.npeq;
    mreq = (size_t)(ctx->smat.nop * ctx->smat.nop) * sizeof(complex double);
    mem_alloc(ctx, (void **)&ctx->smat.ssx, mreq);
    
    // Allocate current array
    mreq = (size_t)ctx->geometry.np3m * sizeof(complex double);
    mem_alloc(ctx, (void **)&ctx->crnt.cur, mreq);
    
    // Allocate current basis function coefficient arrays
    mreq = (size_t)ctx->geometry.npm * sizeof(double);
    mem_alloc(ctx, (void **)&ctx->crnt.air, mreq);
    mem_alloc(ctx, (void **)&ctx->crnt.aii, mreq);
    mem_alloc(ctx, (void **)&ctx->crnt.bir, mreq);
    mem_alloc(ctx, (void **)&ctx->crnt.bii, mreq);
    mem_alloc(ctx, (void **)&ctx->crnt.cir, mreq);
    mem_alloc(ctx, (void **)&ctx->crnt.cii, mreq);
    
    // Save unscaled geometry for frequency scaling
    double *xtemp = NULL, *ytemp = NULL, *ztemp = NULL;
    double *sitemp = NULL, *bitemp = NULL;
    
    if (ctx->geometry.n > 0 || ctx->geometry.m > 0) {
        mreq = (ctx->geometry.n + ctx->geometry.m) * sizeof(double);
        mem_alloc(ctx, (void **)&xtemp, mreq);
        mem_alloc(ctx, (void **)&ytemp, mreq);
        mem_alloc(ctx, (void **)&ztemp, mreq);
        mem_alloc(ctx, (void **)&sitemp, mreq);
        mem_alloc(ctx, (void **)&bitemp, mreq);
        
        // Save wire geometry
        for (int i = 0; i < ctx->geometry.n; i++) {
            xtemp[i] = ctx->geometry.x[i];
            ytemp[i] = ctx->geometry.y[i];
            ztemp[i] = ctx->geometry.z[i];
            sitemp[i] = ctx->geometry.si[i];
            bitemp[i] = ctx->geometry.bi[i];
        }
        
        // Save patch geometry (patch-only decks have n==0 but m>0)
        if (ctx->geometry.m > 0) {
            for (int i = 0; i < ctx->geometry.m; i++) {
                int j = i + ctx->geometry.n;
                xtemp[j] = ctx->geometry.px[i];
                ytemp[j] = ctx->geometry.py[i];
                ztemp[j] = ctx->geometry.pz[i];
                bitemp[j] = ctx->geometry.pbi[i];
            }
        }
    }
    
    // Perform fblock matrix setup if needed
    if (ctx->matpar.imat == 0) {
        fblock(ctx, ctx->netcx.npeq, ctx->netcx.neq, iresrv, ctx->geometry.ipsym);
    }
    
    // Frequency loop
    for (int mhz = 1; mhz <= nfrq; mhz++) {
        // Update frequency
        if (mhz > 1) {
            if (ifrq == 1) {
                ctx->save.fmhz *= delfrq;
            } else {
                ctx->save.fmhz += delfrq;
            }
        }
        
        // Calculate wavelength and frequency ratio
        double fr = ctx->save.fmhz / CVEL;
        ctx->geometry.wlam = CVEL / ctx->save.fmhz;
        
        // Scale geometry to current frequency
        if (ctx->geometry.n > 0) {
            for (int i = 0; i < ctx->geometry.n; i++) {
                ctx->geometry.x[i] = xtemp[i] * fr;
                ctx->geometry.y[i] = ytemp[i] * fr;
                ctx->geometry.z[i] = ztemp[i] * fr;
                ctx->geometry.si[i] = sitemp[i] * fr;
                ctx->geometry.bi[i] = bitemp[i] * fr;
            }
        }
        
        if (ctx->geometry.m > 0) {
            double fr2 = fr * fr;
            for (int i = 0; i < ctx->geometry.m; i++) {
                int j = i + ctx->geometry.n;
                ctx->geometry.px[i] = xtemp[j] * fr;
                ctx->geometry.py[i] = ytemp[j] * fr;
                ctx->geometry.pz[i] = ztemp[j] * fr;
                ctx->geometry.pbi[i] = bitemp[j] * fr2;
            }
        }
        
        // Apply loading to structure
        if (ctx->zload.nload > 0) {
            int *ldtyp = ctx->zload.ldtyp;
            int *ldtag = ctx->zload.ldtag;
            int *ldtagf = ctx->zload.ldtagf;
            int *ldtagt = ctx->zload.ldtagt;
            double *zlr = ctx->zload.zlr;
            double *zli = ctx->zload.zli;
            double *zlc = ctx->zload.zlc;
            
            if (load(ctx, ldtyp, ldtag, ldtagf, ldtagt, zlr, zli, zlc) != 0)
                return -1;
        }
        
        // Set up ground parameters
        if (ctx->gnd.ksymp != 1) {
            ctx->gnd.frati = CPLX_10;
            
            if (ctx->gnd.iperf != 1) {
                double sig = ctx->save.sig;
                if (sig < 0.0) {
                    sig = -sig / (59.96 * ctx->geometry.wlam);
                    ctx->save.sig = sig;
                }
                
                complex double epsc = ctx->save.epsr - I * sig * ctx->geometry.wlam * 59.96;
                ctx->gnd.zrati = 1.0 / csqrt(epsc);
                ctx->gwav.u = ctx->gnd.zrati;
                ctx->gwav.u2 = ctx->gwav.u * ctx->gwav.u;
                
                // Handle radial wire ground screen
                if (ctx->gnd.nradl != 0) {
                    ctx->gnd.scrwl = ctx->save.scrwlt / ctx->geometry.wlam;
                    ctx->gnd.scrwr = ctx->save.scrwrt / ctx->geometry.wlam;
                    ctx->gnd.t1 = CPLX_01 * 2367.067 / (double)ctx->gnd.nradl;
                    ctx->gnd.t2 = ctx->gnd.scrwr * (double)ctx->gnd.nradl;
                }
                
                // Use Sommerfeld ground solution if requested
                if (ctx->gnd.iperf == 2) {
                    somnec(ctx, ctx->save.epsr, ctx->save.sig, ctx->save.fmhz);
                    ctx->gnd.frati = (epsc - 1.0) / (epsc + 1.0);
                }
            }
        }
        
        // Fill and factor primary interaction matrix
        double tim1, tim2;
        nec_get_time_ms(ctx, &tim1);
        if (cmset(ctx, ctx->netcx.neq, cm, ctx->dataj.rkh, ctx->dataj.iexk) != 0) {
            mem_free(ctx, (void *)&cm);
            return -1;
        }

        /* If NGF segments were loaded via a GF card, inject the cached matrix
         * block into the upper-left ngf_neq × ngf_neq corner of cm.
         * cmset just filled the entire matrix; overwriting the old-vs-old
         * block with the stored values restores the higher-accuracy matrix
         * from the prior WG run (which may have used a finer integration
         * or different solver options). */
        if (ctx->has_ngf && ctx->ngf_cm != NULL) {
            int nn  = ctx->ngf_neq;
            int neq = ctx->netcx.neq;
            for (int col = 0; col < nn && col < neq; col++)
                for (int row = 0; row < nn && row < neq; row++)
                    cm[row + col * neq] = ctx->ngf_cm[row + col * nn];
        }

        /* Export the (possibly NGF-injected) matrix if green_fp is open */
        if (ctx->green_fp != NULL) {
            write_greens_binary(ctx->green_fp, ctx, ctx->netcx.neq, cm);
            fclose(ctx->green_fp);
            ctx->green_fp = NULL;
            if (ctx->wg_after_cmset) {
                /* WG mode: write NGF file then stop — do not factorise or solve */
                ctx->wg_after_cmset = false;
                nec_get_time_ms(ctx, &tim2);
                ctx->mat_fill_time = tim2 - tim1;
                break;  /* exit frequency loop without solving */
            }
        }

        nec_get_time_ms(ctx, &tim2);
        ctx->mat_fill_time = tim2 - tim1;

        factrs(ctx, ctx->netcx.npeq, ctx->netcx.neq, cm, ctx->save.ip);
        nec_get_time_ms(ctx, &tim1);
        ctx->mat_factor_time = tim1 - tim2;
        
        // Reset solution counter
        ctx->netcx.ntsol = 0;
        ctx->netcx.nprint = 0;
        
        // Set up excitation and solve
        // For voltage source excitation (most common case)
        if (ctx->fpat.ixtyp == 0 || ctx->fpat.ixtyp == 5) {
            // Fill right-hand side matrix (excitation)
            etmns(ctx, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, ctx->fpat.ixtyp, ctx->crnt.cur);
            
            // Solve with network
            network(ctx, cm, ctx->save.ip, ctx->crnt.cur);
            ctx->netcx.ntsol = 1;
            
            // Calculate power loss in structure
            ctx->fpat.ploss = 0.0;
            if (ctx->geometry.n > 0) {
                for (int i = 0; i < ctx->geometry.n; i++) {
                    complex double curi = ctx->crnt.cur[i] * ctx->geometry.wlam;
                    double cmag = cabs(curi);
                    
                    if (ctx->zload.nload > 0 && fabs(creal(ctx->zload.zarray[i])) >= 1.e-20) {
                        ctx->fpat.ploss += 0.5 * cmag * cmag * 
                                          creal(ctx->zload.zarray[i]) * ctx->geometry.si[i];
                    }
                }
            }
            
            // Handle coupling calculations if requested
            if (ctx->yparm.ncoup > 0) {
                couple(ctx, ctx->crnt.cur, ctx->geometry.wlam);
            }
            
            // Near field calculation if requested
            // Note: do NOT reset near to -1 here after the last frequency;
            // the output guard in main.c reads it after execute_frequency_loop
            // returns, and nec_calculation_defaults resets it per-batch.
            if (ctx->fpat.near != -1) {
                nfpat(ctx);
            }
            
            // Store data for radiation pattern output (calculation happens in output.c)
            if (ctx->gnd.ifar != -1) {
                ctx->fpat.pinr = ctx->netcx.pin;
                ctx->fpat.pnlr = ctx->netcx.pnls;
                rdpat(ctx);
            }
        }
    }
    
    // Free temporary arrays
    mem_free(ctx, (void **)&cm);
    if (xtemp != NULL) {
        mem_free(ctx, (void **)&xtemp);
        mem_free(ctx, (void **)&ytemp);
        mem_free(ctx, (void **)&ztemp);
        mem_free(ctx, (void **)&sitemp);
        mem_free(ctx, (void **)&bitemp);
    }
    
    return 0;
}

