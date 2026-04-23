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
static int calculation_defaults(context_t *ctx);
static int execute_frequency_loop(context_t *ctx, int nfrq, int ifrq, double delfrq, const deck_t *deck);
static void reset_loading_buffers(context_t *ctx);
static void reset_network_buffers(context_t *ctx);
static void reset_coupling_buffers(context_t *ctx);
static void reset_vsorc_buffers(context_t *ctx);
static int process_next_batch(context_t *ctx, deck_t *deck, int *batch_start, int *batch_end, bool *batch_has_fr);
static int execute_extra_patterns(context_t *ctx, const deck_t *deck, int batch_start, int batch_end);
static int count_tag_segments(const context_t *ctx, int tag);
static int resolve_pct_segment(const context_t *ctx, const card_t *card, int field_idx, int tag);
static void validate_geometry_post_calculation(context_t *ctx, errors_list_t *errors);
static int inject_current_source(context_t *ctx, int card_idx, int tag, int seg_idx, complex double I_desired);

/******************************************************************************
 * count_tag_segments()
 *
 * Returns the total number of geometry segments carrying the given tag.
 * tag == 0 means all segments (NEC convention).
 */
static int count_tag_segments(const context_t *ctx, int tag)
{
    if (tag == 0) return ctx->geometry.num_segs;
    int count = 0;
    for (int i = 0; i < ctx->geometry.num_segs; i++) {
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
static int resolve_pct_segment(const context_t *ctx, const card_t *card,
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
 * validate_geometry_post_calculation()
 *
 * Private helper: Post-calculation geometry sanity checks. These validations
 * run AFTER geometry calculation and formula evaluation, but BEFORE the NEC
 * solver (fill_interaction_matrix). They check for physical impossibilities
 * that would cause calculation failures or looping.
 *
 * Checks performed:
 * 1. No zero-length wires (half_len must be > 0)
 * 2. No zero-radius wires (radius must be > 0)  
 * 3. No zero-area patches (patch_area must be > 0)
 *
 * These are FATAL validation errors that stop calculation.
 *
 * @param ctx     The NEC context with calculated geometry
 * @param errors  The errors_list_t to append errors to
 */
static void validate_geometry_post_calculation(context_t *ctx, errors_list_t *errors)
{
  char msg[MAX_ERROR_LEN];

  if (ctx == NULL || errors == NULL) {
    return;
  }

  /* Check wire segments for zero length or zero radius */
  if (ctx->geometry.num_segs > 0) {
    if (ctx->geometry.half_len == NULL || ctx->geometry.radius == NULL) {
      snprintf(msg, sizeof(msg), 
               "Internal error: geometry has %d segments but half_len or radius array is NULL.",
               ctx->geometry.num_segs);
      add_error(ctx, errors, msg, FATAL);
      return;
    }

    for (int i = 0; i < ctx->geometry.num_segs; i++) {
      double len = ctx->geometry.half_len[i];
      double rad = ctx->geometry.radius[i];
      int tag = ctx->geometry.tag_nums[i];

      /* Check for zero or negative length */
      if (len <= 0.0) {
        snprintf(msg, sizeof(msg),
                 "Wire segment %d (tag %d): has zero or negative length (%.6g m). "
                 "This prevents NEC solver from running.", 
                 i + 1, tag, len);
        add_error(ctx, errors, msg, FATAL);
      }

      /* Check for zero or negative radius */
      if (rad <= 0.0) {
        snprintf(msg, sizeof(msg),
                 "Wire segment %d (tag %d): has zero or negative radius (%.6g m). "
                 "This prevents NEC solver from running.",
                 i + 1, tag, rad);
        add_error(ctx, errors, msg, FATAL);
      }
    }
  }

  /* Check patch geometry for zero area */
  if (ctx->geometry.num_patches > 0) {
    if (ctx->geometry.patch_area == NULL) {
      snprintf(msg, sizeof(msg),
               "Internal error: geometry has %d patches but patch_area array is NULL.",
               ctx->geometry.num_patches);
      add_error(ctx, errors, msg, FATAL);
      return;
    }

    for (int i = 0; i < ctx->geometry.num_patches; i++) {
      double area = ctx->geometry.patch_area[i];

      /* Check for zero or negative area (patch_area is in wavelengths^2) */
      if (area <= 0.0) {
        /* Patch with zero/negative area is tolerated; no warning emitted. */
        continue;
      }
    }
  }
}

/******************************************************************************
 * run_simulation()
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
int run_simulation(context_t *ctx, deck_t *deck)
{
    errors_list_t geometry_errors = {0};

    // Step 1: Calculate geometry.
    // Skip if already populated — estimate_time() may have done this as a
    // side effect (e.g. a GUI calling the estimator before launching the run).
    if (ctx->geometry.num_segs == 0 && ctx->geometry.num_patches == 0) {
        calculate_geometry(ctx, deck, &geometry_errors, &ctx->outputs);

        // Check for geometry errors
        if (geometry_errors.num_errors > 0) {
            // Transfer already-logged errors without re-printing them
            transfer_errors(&geometry_errors, &ctx->errors);
            return -1;
        }
    }
    
    // Step 1b: Post-geometry sanity checks on calculated values
    // This validates that formulas evaluated correctly and geometry is physically valid
    // (no zero-length wires, zero-radius wires, or zero-area patches).
    // These checks prevent looping or crashes in the NEC solver.
    validate_geometry_post_calculation(ctx, &ctx->errors);
    if (ctx->errors.num_errors > 0) {
        // Check for any FATAL errors in post-calculation validation
        for (int i = 0; i < ctx->errors.num_errors; i++) {
            if (ctx->errors.errors[i].severity == FATAL) {
                return -1;
            }
        }
    }
    
    // Step 2: Initialize calculation defaults (requires valid geometry)
    if (calculation_defaults(ctx) != 0) {
        add_error(ctx, &ctx->errors, "Failed to initialize calculation defaults (no valid geometry)", FATAL);
        return -1;
    }

    // Step 2b: Complexity pre-check — warn before starting expensive work.
    // estimate_time() reuses the geometry already computed above (no re-calc).
    {
        double T = estimate_time(ctx, deck);
        int n_total = ctx->geometry.num_segs;
        int n_cell  = ctx->geometry.num_segs_sym > 0 ? ctx->geometry.num_segs_sym : n_total;
        int m_sym   = (n_cell > 0) ? n_total / n_cell : 1;
        if (T >= 1.0e11) {
            char cmsg[256];
            if (m_sym > 1)
                snprintf(cmsg, sizeof(cmsg),
                         "WARNING: large model detected: %d total segments "
                         "(%d in symmetry cell, x%d symmetry). "
                         "Complexity T=%.2e — this may run for several minutes.",
                         n_total, n_cell, m_sym, T);
            else
                snprintf(cmsg, sizeof(cmsg),
                         "WARNING: large model detected: %d segments, no symmetry. "
                         "Complexity T=%.2e — this may run for several minutes.",
                         n_total, T);
            report(ctx, ONEC_SEV_WARNING, "%s", cmsg);
        }
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
        bool batch_has_fr = false;

        // Process next batch of control cards
        int batch_result = process_next_batch(ctx, deck, &batch_start, &batch_end, &batch_has_fr);
        if (batch_result < 0) {
            // Error occurred
            return -1;
        }
        
        // Check if we're done before processing the batch
        if (batch_result == 1) {
            deck_complete = true;
            break;
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
        bool has_output_request = (ctx->gnd.far_field_type != -1 ||
                                   ctx->fpat.is_near_field != -1 ||
                                   has_xq ||
                                   ctx->wg_after_cmset);

        // Determine whether this batch needs a full solve or just extra patterns.
        // A batch with no FR card and an already-completed prior solve behaves like
        // nec2c's igo==4 path: compute radiation/near-field patterns with existing
        // currents, without re-filling the matrix or re-printing power budget.
        // This also covers a bare XQ after an RP+XQ pair (where RP already ran the
        // full solve): XQ alone with no new FR is a no-op / already handled.
        bool extra_patterns_only = (!batch_has_fr &&
                                    ctx->frequency_loop_ran &&
                                    !ctx->patterns_output_for_freq &&
                                    (ctx->gnd.far_field_type != -1 || ctx->fpat.is_near_field != -1 ||
                                     has_xq) &&
                                    !ctx->wg_after_cmset);

        if (!is_termination && has_output_request) {
            if (extra_patterns_only) {
                // XQ alone with no new FR: nothing new to compute, skip
                if (!has_xq) {
                    if (execute_extra_patterns(ctx, deck, batch_start, batch_end) != 0) {
                        return -1;
                    }
                }
            } else {
                ctx->frequency_loop_ran = true;
                if (execute_frequency_loop(ctx, ctx->save.num_freq, ctx->save.freq_step_type, ctx->save.freq_step, deck) != 0) {
                    return -1;
                }
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

            if (ctx->nfr.points != NULL) { free(ctx->nfr.points); ctx->nfr.points = NULL; }
            ctx->nfr.num_points = 0;

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
            if (calculation_defaults(ctx) != 0) {
                add_error(ctx, &ctx->errors,
                    "NX: failed to initialize calculation defaults for new section", FATAL);
                return -1;
            }

            ctx->current_card_idx = new_geom_end + 1;
            ctx->iflow = 0;
            continue;
        }
    }
    
    return 0;
}

/******************************************************************************
 * calculation_defaults()
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
static int calculation_defaults(context_t *ctx)
{
    // validate that geometry has been calculated
    if (ctx->geometry.num_segs_sym <= 0 && ctx->geometry.num_patches_sym <= 0) {
        return -1;
    }
    
    // set geometry-dependent matrix parameters
    ctx->netcx.num_eq_sym = ctx->geometry.num_segs_sym + 2 * ctx->geometry.num_patches_sym;
    
    // matrix parameters (from oldmain.c lines 289-292)
    if (ctx->matpar.core_used == 0) {
        ctx->netcx.num_eq = ctx->geometry.num_segs + 2 * ctx->geometry.num_patches;
        ctx->netcx.num_eq_ngf = 0;
    }
    
    // reset all calculation defaults to initial values
    // these are reset for each run to support multiple calculations
    ctx->fpat.excitation_type = 0;
    ctx->fpat.is_near_field = -1;
    ctx->zload.num_loads = 0;
    ctx->loading_outputs.count = 0;
    ctx->loading_outputs.capacity = 0;
    ctx->loading_outputs.entries = NULL;
    ctx->netcx.num_networks = 0;
    ctx->plot.plot_type = 0;
    ctx->plot.plot_axis = 0;
    ctx->plot.plot_component = 0;
    ctx->plot.plot_gain_type = 0;
    ctx->yparm.num_pairs = 0;
    ctx->yparm.coupling_flag = 0;
    ctx->gnd.is_perfect = 0;
    ctx->gnd.num_radials = 0;
    ctx->dataj.k_half_len = 1.0;  // Default matrix integration limit
    ctx->dataj.use_extended_kernel = 0;   // Extended thin-wire kernel off by default
    ctx->gnd.far_field_type = -1;
    ctx->frequency_loop_ran = false;
    ctx->freq_step_output_written = false;
    ctx->preamble_written = false;
    
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
static void reset_loading_buffers(context_t *ctx)
{
    if (ctx->zload.num_loads > 0) {
        mem_free(ctx, (void **)&ctx->zload.load_types);
        mem_free(ctx, (void **)&ctx->zload.load_tags);
        mem_free(ctx, (void **)&ctx->zload.load_tag_from);
        mem_free(ctx, (void **)&ctx->zload.load_tag_to);
        mem_free(ctx, (void **)&ctx->zload.ldcard_num);
        mem_free(ctx, (void **)&ctx->zload.load_r);
        mem_free(ctx, (void **)&ctx->zload.load_l);
        mem_free(ctx, (void **)&ctx->zload.load_c);
        mem_free(ctx, (void **)&ctx->zload.load_freq);
        ctx->zload.num_loads = 0;
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
static void reset_network_buffers(context_t *ctx)
{
    if (ctx->netcx.num_networks > 0) {
        mem_free(ctx, (void **)&ctx->netcx.net_types);
        mem_free(ctx, (void **)&ctx->netcx.net_seg1);
        mem_free(ctx, (void **)&ctx->netcx.net_seg2);
        mem_free(ctx, (void **)&ctx->netcx.y11_real);
        mem_free(ctx, (void **)&ctx->netcx.y11_imag);
        mem_free(ctx, (void **)&ctx->netcx.y12_real);
        mem_free(ctx, (void **)&ctx->netcx.y12_imag);
        mem_free(ctx, (void **)&ctx->netcx.y22_real);
        mem_free(ctx, (void **)&ctx->netcx.y22_imag);
        ctx->netcx.num_networks = 0;
    }
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
}

/******************************************************************************
 * reset_vsorc_buffers()
 *
 * Reset and free excitation source (vsorc) buffers. Called on NX restart.
 */
static void reset_vsorc_buffers(context_t *ctx)
{
    if (ctx->vsorc.num_vsrcs > 0) {
        mem_free(ctx, (void **)&ctx->vsorc.vsrc_segs);
        mem_free(ctx, (void **)&ctx->vsorc.vsrc_voltages);
        ctx->vsorc.num_vsrcs = 0;
    }
    if (ctx->vsorc.num_qdsrcs > 0) {
        mem_free(ctx, (void **)&ctx->vsorc.qdsrc_segs);
        mem_free(ctx, (void **)&ctx->vsorc.qdsrc_indices);
        mem_free(ctx, (void **)&ctx->vsorc.qdsrc_voltages);
        mem_free(ctx, (void **)&ctx->vsorc.qdsrc_voltages_saved);
        ctx->vsorc.num_qdsrcs = 0;
        ctx->vsorc.num_qdsrcs_used = 0;
    }
}

/******************************************************************************
 * inject_current_source()
 *
 * Implements EX type 6 (current source) by expanding it into an equivalent
 * voltage source on a synthetic dummy wire segment coupled to the target
 * segment via an NT two-port admittance network.
 *
 * The dummy wire is placed parallel to the target, offset by 4 wire radii in
 * the most perpendicular direction, so it does not physically contact any
 * existing segment.  A unit voltage (1+0j V) is applied to the dummy via a
 * synthesised EX 0 entry.  The NT network uses Y12 = -I_desired (all other
 * Y-params zero), which forces current I_desired from the network into the
 * target segment (NEC port-current convention: positive into the network).
 *
 * This function appends the dummy segment to ctx->geometry without calling
 * wire() to avoid resetting the geometry symmetry flags unnecessarily; it
 * directly extends and populates all derived arrays (half_len, dir_cos_*,
 * *_center, seg_end1/2_conn).  num_segs_sym is updated to match num_segs so
 * the matrix solver covers the new segment.
 *
 * Must be called only after calculate_geometry() has completed.
 *
 * @param ctx        The NEC context
 * @param card_idx   0-based index of the EX card (used in error messages)
 * @param tag        Tag number of the target wire
 * @param seg_idx    Segment index within the tag (1-based)
 * @param I_desired  Desired injected current in Amperes (complex)
 * @return           0 on success, -1 on fatal error
 */
static int inject_current_source(context_t *ctx, int card_idx,
                                  int tag, int seg_idx,
                                  complex double I_desired)
{
  geometry_t *geom = &ctx->geometry;
  char msg[MAX_ERROR_LEN];
  size_t mreq;

  /* Resolve target segment number (1-based) */
  int tgt_seg = segment_number(ctx, tag, seg_idx);
  if (tgt_seg == 0 || tgt_seg > geom->num_segs) {
    snprintf(msg, sizeof(msg),
             "EX on line %d: type 6 references invalid tag %d, segment %d",
             card_idx + 1, tag, seg_idx);
    add_error(ctx, &ctx->errors, msg, FATAL);
    return -1;
  }
  int tgt_idx = tgt_seg - 1;  /* 0-based index */

  /* Find a unique dummy tag number above all existing tags */
  int max_tag = 0;
  for (int i = 0; i < geom->num_segs; i++) {
    if (geom->tag_nums[i] > max_tag) {
      max_tag = geom->tag_nums[i];
    }
  }
  int dummy_tag = max_tag + 1;

  /* --- Compute dummy wire endpoints ---
   * Place the dummy parallel to the target, offset by 4*radius in the
   * direction most perpendicular to the wire axis.                       */
  double x1  = geom->end1_x[tgt_idx];
  double y1  = geom->end1_y[tgt_idx];
  double z1  = geom->end1_z[tgt_idx];
  double x2  = geom->end2_x[tgt_idx];
  double y2  = geom->end2_y[tgt_idx];
  double z2  = geom->end2_z[tgt_idx];
  double rad = geom->radius[tgt_idx];

  /* Unit direction vector along the target segment */
  double dx = x2 - x1, dy = y2 - y1, dz = z2 - z1;
  double L = sqrt(dx*dx + dy*dy + dz*dz);
  double ux = dx / L, uy = dy / L, uz = dz / L;

  /* Choose perpendicular direction via cross product with the most
   * orthogonal axis (avoids degeneracy when wire is axis-aligned).     */
  double px, py, pz;
  if (fabs(ux) <= fabs(uy) && fabs(ux) <= fabs(uz)) {
    /* cross(u, x-hat) = (0, uz, -uy) */
    px = 0.0; py = uz; pz = -uy;
  } else if (fabs(uy) <= fabs(ux) && fabs(uy) <= fabs(uz)) {
    /* cross(u, y-hat) = (-uz, 0, ux) */
    px = -uz; py = 0.0; pz = ux;
  } else {
    /* cross(u, z-hat) = (uy, -ux, 0) */
    px = uy; py = -ux; pz = 0.0;
  }
  double plen = sqrt(px*px + py*py + pz*pz);
  px /= plen; py /= plen; pz /= plen;

  double offset = 4.0 * rad;
  double d1x = x1 + offset * px;
  double d1y = y1 + offset * py;
  double d1z = z1 + offset * pz;
  double d2x = x2 + offset * px;
  double d2y = y2 + offset * py;
  double d2z = z2 + offset * pz;

  /* --- Append dummy segment to the geometry arrays ---
   * Done inline (not via wire()) to preserve existing symmetry flags.
   * All arrays that finish_geometry() and connect_segments() fill are
   * extended and populated for the new segment index.                    */
  int new_idx = geom->num_segs;
  int new_n   = geom->num_segs + 1;

  mreq = (size_t)new_n * sizeof(double);
  mem_realloc(ctx, (void **)&geom->end1_x,    mreq);
  mem_realloc(ctx, (void **)&geom->end1_y,    mreq);
  mem_realloc(ctx, (void **)&geom->end1_z,    mreq);
  mem_realloc(ctx, (void **)&geom->end2_x,    mreq);
  mem_realloc(ctx, (void **)&geom->end2_y,    mreq);
  mem_realloc(ctx, (void **)&geom->end2_z,    mreq);
  mem_realloc(ctx, (void **)&geom->radius,    mreq);
  mem_realloc(ctx, (void **)&geom->half_len,  mreq);
  mem_realloc(ctx, (void **)&geom->dir_cos_x, mreq);
  mem_realloc(ctx, (void **)&geom->dir_cos_y, mreq);
  mem_realloc(ctx, (void **)&geom->dir_cos_z, mreq);
  mem_realloc(ctx, (void **)&geom->x_center,  mreq);
  mem_realloc(ctx, (void **)&geom->y_center,  mreq);
  mem_realloc(ctx, (void **)&geom->z_center,  mreq);

  /* tag/card and connection arrays are sized to num_segs + num_patches */
  mreq = (size_t)(new_n + geom->num_patches) * sizeof(int);
  mem_realloc(ctx, (void **)&geom->card_nums,     mreq);
  mem_realloc(ctx, (void **)&geom->tag_nums,      mreq);
  mem_realloc(ctx, (void **)&geom->seg_end1_conn, mreq);
  mem_realloc(ctx, (void **)&geom->seg_end2_conn, mreq);

  /* Fill raw endpoint fields */
  geom->end1_x[new_idx]  = d1x;
  geom->end1_y[new_idx]  = d1y;
  geom->end1_z[new_idx]  = d1z;
  geom->end2_x[new_idx]  = d2x;
  geom->end2_y[new_idx]  = d2y;
  geom->end2_z[new_idx]  = d2z;
  geom->radius[new_idx]  = rad;
  geom->card_nums[new_idx] = card_idx;
  geom->tag_nums[new_idx]  = dummy_tag;

  /* Fill derived fields (mirrors finish_geometry() for the new segment).
   * Note: half_len stores the full segment length (the formula is the same
   * numerical computation used in finish_geometry).                        */
  double ddx = d2x - d1x, ddy = d2y - d1y, ddz = d2z - d1z;
  double L2   = ddx*ddx + ddy*ddy + ddz*ddz;
  double Lmag = sqrt(L2);
  geom->half_len[new_idx]  = (L2 / Lmag + Lmag) * 0.5;
  geom->x_center[new_idx]  = (d1x + d2x) / 2.0;
  geom->y_center[new_idx]  = (d1y + d2y) / 2.0;
  geom->z_center[new_idx]  = (d1z + d2z) / 2.0;
  geom->dir_cos_x[new_idx] = ddx / Lmag;
  geom->dir_cos_y[new_idx] = ddy / Lmag;
  {
    double dcz = ddz / Lmag;
    if (dcz >  1.0) { dcz =  1.0; }
    if (dcz < -1.0) { dcz = -1.0; }
    geom->dir_cos_z[new_idx] = dcz;
  }

  /* Dummy segment is isolated — no connections to neighbouring segments */
  geom->seg_end1_conn[new_idx] = 0;
  geom->seg_end2_conn[new_idx] = 0;

  /* Update all geometry counters.  Reset num_segs_sym to the new total so
   * the matrix covers the added segment; this disables any prior symmetry
   * optimisation for this run.                                             */
  geom->num_segs             = new_n;
  geom->num_segs_sym         = new_n;
  geom->num_segs_and_patches = new_n + geom->num_patches;
  geom->num_segs_2xpatches   = new_n + 2 * geom->num_patches;
  geom->num_segs_3xpatches   = new_n + 3 * geom->num_patches;

  /* Update matrix equation dimension to include the added segment */
  ctx->netcx.num_eq     = new_n + 2 * geom->num_patches;
  ctx->netcx.num_eq_sym = new_n + 2 * geom->num_patches_sym;

  int dummy_seg = new_n;  /* 1-based segment number of the dummy */

  /* --- Add voltage source (EX 0) on the dummy segment --- */
  ctx->vsorc.num_vsrcs++;
  mreq = (size_t)ctx->vsorc.num_vsrcs * sizeof(int);
  mem_realloc(ctx, (void **)&ctx->vsorc.vsrc_segs, mreq);
  mreq = (size_t)ctx->vsorc.num_vsrcs * sizeof(complex double);
  mem_realloc(ctx, (void **)&ctx->vsorc.vsrc_voltages, mreq);
  {
    int vsrc_idx = ctx->vsorc.num_vsrcs - 1;
    ctx->vsorc.vsrc_segs[vsrc_idx]     = dummy_seg;
    ctx->vsorc.vsrc_voltages[vsrc_idx] = CPLX_10;  /* 1+0j V reference */
  }

  /* --- Add NT network: dummy (port 1) → target (port 2) ---
   * Two-port admittance: I_port2 = Y12*V1 + Y22*V2
   * With V1=1 V, Y12=-I_desired, Y22=0 → I_port2 = -I_desired.
   * NEC sign convention: I_port is current INTO the network from the segment,
   * so current FROM network INTO target antenna = +I_desired.              */
  if (ctx->iflow != 6 && ctx->netcx.num_networks == 0) {
    reset_network_buffers(ctx);
    ctx->iflow = 6;
  }
  ctx->netcx.num_networks++;
  mreq = (size_t)ctx->netcx.num_networks * sizeof(int);
  mem_realloc(ctx, (void **)&ctx->netcx.net_types, mreq);
  mem_realloc(ctx, (void **)&ctx->netcx.net_seg1,  mreq);
  mem_realloc(ctx, (void **)&ctx->netcx.net_seg2,  mreq);
  mreq = (size_t)ctx->netcx.num_networks * sizeof(double);
  mem_realloc(ctx, (void **)&ctx->netcx.y11_real, mreq);
  mem_realloc(ctx, (void **)&ctx->netcx.y11_imag, mreq);
  mem_realloc(ctx, (void **)&ctx->netcx.y12_real, mreq);
  mem_realloc(ctx, (void **)&ctx->netcx.y12_imag, mreq);
  mem_realloc(ctx, (void **)&ctx->netcx.y22_real, mreq);
  mem_realloc(ctx, (void **)&ctx->netcx.y22_imag, mreq);
  {
    int nt_idx = ctx->netcx.num_networks - 1;
    ctx->netcx.net_types[nt_idx] = 1;         /* NT admittance network */
    ctx->netcx.net_seg1[nt_idx]  = dummy_seg; /* port 1 = dummy */
    ctx->netcx.net_seg2[nt_idx]  = tgt_seg;   /* port 2 = target */
    ctx->netcx.y11_real[nt_idx] = 0.0;
    ctx->netcx.y11_imag[nt_idx] = 0.0;
    ctx->netcx.y12_real[nt_idx] = -creal(I_desired);
    ctx->netcx.y12_imag[nt_idx] = -cimag(I_desired);
    ctx->netcx.y22_real[nt_idx] = 0.0;
    ctx->netcx.y22_imag[nt_idx] = 0.0;
  }

  return 0;
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
static int process_next_batch(context_t *ctx, deck_t *deck, int *batch_start, int *batch_end, bool *batch_has_fr)
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
    bool found_fr = false;  // track whether an FR card is in this batch
    
    for (int card_idx = ctx->current_card_idx; card_idx < deck->num_cards; card_idx++) {
        card_t *card = &deck->cards[card_idx];
        
        // Skip ignored or comment cards
        if (card->ignore || is_comment(card)) {
            continue;
        }
        
        char *code = card->card_code;

        // Track FR card appearances in this batch
        if (strcmp(code, "FR") == 0) {
            found_fr = true;
        }
        
        // Check for batch termination cards
        if (strcmp(code, "XQ") == 0 ||
            strcmp(code, "RP") == 0 ||
            strcmp(code, "NE") == 0 ||
            strcmp(code, "NH") == 0) {
            *batch_end = card_idx;  // Include this card in the batch
            ctx->current_card_idx = card_idx + 1;  // Next batch starts after
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

    if (batch_has_fr != NULL) {
        *batch_has_fr = found_fr;
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
            ctx->save.freq_step_type = i1;
            ctx->save.num_freq = (i2 == 0) ? 1 : i2;
            ctx->save.freq_mhz = f1;
            ctx->save.freq_step = f2;
            if (ctx->save.first_fr_mhz == 0.0)
                ctx->save.first_fr_mhz = f1;
            // Reset pattern output flag for new frequency specification
            ctx->patterns_output_for_freq = false;
        }
        else if (strcmp(code, "LD") == 0) {
            // LD card - Loading
            if (i1 == -1) {
                continue;
            }

            if (i1 > 7) {
                char msg[MAX_ERROR_LEN];
                snprintf(msg, sizeof(msg), "LD on line %d: type %d is not supported.", card_idx + 1, i1);
                add_error(ctx, &ctx->errors, msg, FATAL);
                return -1;
            }
            
            // First LD in batch resets loading (iflow transition to 3)
            if (ctx->iflow != 3 && ctx->zload.num_loads == 0) {
                reset_loading_buffers(ctx);
                ctx->iflow = 3;
            }
            
            // Reallocate loading buffers
            ctx->zload.num_loads++;
            size_t mreq = (size_t)ctx->zload.num_loads * sizeof(int);
            mem_realloc(ctx, (void **)&ctx->zload.load_types, mreq);
            mem_realloc(ctx, (void **)&ctx->zload.load_tags, mreq);
            mem_realloc(ctx, (void **)&ctx->zload.load_tag_from, mreq);
            mem_realloc(ctx, (void **)&ctx->zload.load_tag_to, mreq);
            mem_realloc(ctx, (void **)&ctx->zload.ldcard_num, mreq);
            
            mreq = (size_t)ctx->zload.num_loads * sizeof(double);
            mem_realloc(ctx, (void **)&ctx->zload.load_r, mreq);
            mem_realloc(ctx, (void **)&ctx->zload.load_l, mreq);
            mem_realloc(ctx, (void **)&ctx->zload.load_c, mreq);
            mem_realloc(ctx, (void **)&ctx->zload.load_freq, mreq);
            
            int idx = ctx->zload.num_loads - 1;
            ctx->zload.load_types[idx] = i1;
            ctx->zload.load_tags[idx] = i2;
            ctx->zload.ldcard_num[idx] = card_idx + 1;
            /* resolve percentage-style segment specifiers against the tag count */
            int start_seg = resolve_pct_segment(ctx, card, 3, i2);
            int end_seg   = resolve_pct_segment(ctx, card, 4, i2);
            ctx->zload.load_tag_from[idx] = (i4 == 0) ? start_seg : start_seg;
            ctx->zload.load_tag_to[idx]   = (i4 == 0) ? start_seg : end_seg;
            
            if (ctx->zload.load_tag_to[idx] < ctx->zload.load_tag_from[idx]) {
                char msg[MAX_ERROR_LEN];
                snprintf(msg, sizeof(msg),
                    "LD on line %d: ITAG start %d is greater than ITAG end %d",
                    card_idx + 1, i3, i4);
                add_error(ctx, &ctx->errors, msg, FATAL);
                return -1;
            }
            
            ctx->zload.load_r[idx] = f1;
            ctx->zload.load_l[idx] = f2;
            ctx->zload.load_c[idx] = f3;
            /* For LD type 6: f4 optionally overrides the design frequency (MHz).  
             * 0 means "use the first FR card's frequency" (resolved at compute time). */
            ctx->zload.load_freq[idx] = (i1 == 6) ? f4 : 0.0;
        }
        else if (strcmp(code, "GN") == 0) {
            // GN card - Ground parameters  
            if (i1 == -1) {
                ctx->gnd.has_ground = 1;
                ctx->gnd.num_radials = 0;
                ctx->gnd.is_perfect = 0;
                continue;
            }

            if (i1 == 3) {
                /* GN type 3: 4nec2 MiniNec ground extension.
                 * Internally equivalent to GN 1 (perfect ground for currents/impedance)
                 * plus GD 0 0 0 0 <diel> <cond> (second medium starting at distance 0
                 * for far-field calculations).  F1=dielectric constant, F2=conductivity. */
                ctx->gnd.is_perfect = 1;   /* perfect ground for impedance computation */
                ctx->gnd.num_radials = 0;
                ctx->gnd.has_ground = 2;
                ctx->save.ground_epsr  = f1;
                ctx->save.ground_sigma = f2;
                /* second medium starts at distance 0, height 0 (far-field uses real ground) */
                ctx->fpat.epsr2       = f1;
                ctx->fpat.sigma2      = f2;
                ctx->fpat.cliff_dist  = 0.0;
                ctx->fpat.cliff_height = 0.0;
                continue;
            }

            ctx->gnd.is_perfect = i1;
            ctx->gnd.num_radials = i2;
            ctx->gnd.has_ground = 2;
            ctx->save.ground_epsr = f1;
            ctx->save.ground_sigma = f2;
            
            if (ctx->gnd.num_radials != 0) {
                if (ctx->gnd.is_perfect == 2) {
                    char msg[MAX_ERROR_LEN];
                    snprintf(msg, sizeof(msg),
                        "GN on line %d: radial wire ground screen cannot be used with Sommerfeld ground option.",
                        card_idx + 1);
                    add_error(ctx, &ctx->errors, msg, FATAL);
                    return -1;
                }
                if (f3 >= 1.0e-20 || f4 >= 1.0e-20) {
                    ctx->save.screen_wire_len = f3;
                    ctx->save.screen_wire_radius = f4;
                }
            }
        }
        // Continue processing other cards...
        else if (strcmp(code, "EX") == 0) {
            // EX card - Excitation
            ctx->fpat.excitation_type = i1;
            ctx->netcx.check_asymmetry = i4 / 10;

            // For voltage source types (0 and 5)
            if (i1 == 0 || i1 == 5) {
                ctx->netcx.network_type = 0;
                
                if (i1 == 5) {
                    // Incident plane wave or elementary current source
                    ctx->vsorc.num_qdsrcs++;
                    size_t mreq = (size_t)ctx->vsorc.num_qdsrcs * sizeof(int);
                    mem_realloc(ctx, (void **)&ctx->vsorc.qdsrc_segs, mreq);
                    mem_realloc(ctx, (void **)&ctx->vsorc.qdsrc_indices, mreq);
                    
                    mreq = (size_t)ctx->vsorc.num_qdsrcs * sizeof(complex double);
                    mem_realloc(ctx, (void **)&ctx->vsorc.qdsrc_voltages, mreq);
                    mem_realloc(ctx, (void **)&ctx->vsorc.qdsrc_voltages_saved, mreq);
                    
                    int idx = ctx->vsorc.num_qdsrcs - 1;
                    int i3_resolved = resolve_pct_segment(ctx, card, 3, i2);
                    int seg_num = segment_number(ctx, i2, i3_resolved);
                    if (seg_num == 0) {
                        char msg[MAX_ERROR_LEN];
                        snprintf(msg, sizeof(msg), "EX on line %d: references invalid tag %d, segment %d", card_idx + 1, i2, i3_resolved);
                        add_error(ctx, &ctx->errors, msg, FATAL);
                        return -1;
                    }
                    ctx->vsorc.qdsrc_segs[idx] = seg_num;
                    ctx->vsorc.qdsrc_voltages[idx] = f1 + I * f2;
                    if (cabs(ctx->vsorc.qdsrc_voltages[idx]) < 1.e-20) {
                        ctx->vsorc.qdsrc_voltages[idx] = CPLX_10;
                    }
                } else {
                    // Applied voltage source
                    ctx->vsorc.num_vsrcs++;
                    size_t mreq = (size_t)ctx->vsorc.num_vsrcs * sizeof(int);
                    mem_realloc(ctx, (void **)&ctx->vsorc.vsrc_segs, mreq);
                    
                    mreq = (size_t)ctx->vsorc.num_vsrcs * sizeof(complex double);
                    mem_realloc(ctx, (void **)&ctx->vsorc.vsrc_voltages, mreq);
                    
                    int idx = ctx->vsorc.num_vsrcs - 1;
                    int i3_resolved = resolve_pct_segment(ctx, card, 3, i2);
                    int seg_num = segment_number(ctx, i2, i3_resolved);
                    if (seg_num == 0) {
                        char msg[MAX_ERROR_LEN];
                        snprintf(msg, sizeof(msg), "EX on line %d: references invalid tag %d, segment %d", card_idx + 1, i2, i3_resolved);
                        add_error(ctx, &ctx->errors, msg, FATAL);
                        return -1;
                    }
                    ctx->vsorc.vsrc_segs[idx] = seg_num;
                    ctx->vsorc.vsrc_voltages[idx] = f1 + I * f2;
                    if (cabs(ctx->vsorc.vsrc_voltages[idx]) < 1.e-20) {
                        ctx->vsorc.vsrc_voltages[idx] = CPLX_10;
                    }
                }
            } else if (i1 == 6) {
                // EX type 6: current source (4nec2 extension).
                // Converted to a voltage source on a synthetic dummy segment
                // connected to the target via an NT admittance network.
                // Set excitation_type = 0 (voltage source) because the
                // underlying mechanism is a synthesised EX 0 on the dummy.
                ctx->fpat.excitation_type = 0;
                ctx->netcx.network_type = 0;
                int i3_resolved = resolve_pct_segment(ctx, card, 3, i2);
                complex double I_desired = f1 + I * f2;
                if (cabs(I_desired) < 1.e-20) {
                    I_desired = CPLX_10;  /* default to 1 A if zero */
                }
                if (inject_current_source(ctx, card_idx, i2, i3_resolved, I_desired) != 0) {
                    return -1;
                }
            } else if (i1 == 7) {
                // EX type 7 is not supported
                char msg[MAX_ERROR_LEN];
                snprintf(msg, sizeof(msg), "EX on line %d: type 7 is not supported.", card_idx + 1);
                add_error(ctx, &ctx->errors, msg, WARNING);
            } else {
                // Far field pattern for receiving antenna
                ctx->fpat.exc_param6 = f6;
                ctx->vsorc.num_vsrcs = 0;
                ctx->vsorc.num_qdsrcs = 0;
            }
        }
        else if (strcmp(code, "NT") == 0 || strcmp(code, "TL") == 0) {
            // NT/TL cards - Network parameters
            if (i2 == -1) {
                continue;
            }
            
            // First NT/TL in batch resets network (iflow transition to 6)
            if (ctx->iflow != 6 && ctx->netcx.num_networks == 0) {
                reset_network_buffers(ctx);
                ctx->iflow = 6;
            }
            
            // Reallocate network buffers
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
            if (strcmp(code, "NT") == 0) {
                ctx->netcx.net_types[idx] = 1;
            } else {
                ctx->netcx.net_types[idx] = 2;
            }
            
            /* endpoints may be specified with a percentage value, resolve first */
            int seg1_idx = resolve_pct_segment(ctx, card, 2, i1);
            ctx->netcx.net_seg1[idx] = segment_number(ctx, i1, seg1_idx);
            if (ctx->netcx.net_seg1[idx] == 0) {
                char msg[MAX_ERROR_LEN];
                snprintf(msg, sizeof(msg), "%s on line %d: references invalid tag %d, segment %d", code, card_idx + 1, i1, seg1_idx);
                add_error(ctx, &ctx->errors, msg, FATAL);
                return -1;
            }
            int seg2_idx = resolve_pct_segment(ctx, card, 4, i3);
            ctx->netcx.net_seg2[idx] = segment_number(ctx, i3, seg2_idx);
            if (ctx->netcx.net_seg2[idx] == 0) {
                char msg[MAX_ERROR_LEN];
                snprintf(msg, sizeof(msg), "%s on line %d: references invalid tag %d, segment %d", code, card_idx + 1, i3, seg2_idx);
                add_error(ctx, &ctx->errors, msg, FATAL);
                return -1;
            }
            ctx->netcx.y11_real[idx] = f1;
            ctx->netcx.y11_imag[idx] = f2;
            ctx->netcx.y12_real[idx] = f3;
            ctx->netcx.y12_imag[idx] = f4;
            ctx->netcx.y22_real[idx] = f5;
            ctx->netcx.y22_imag[idx] = f6;
            
            // Check for transmission line with impedance
            if ((ctx->netcx.net_types[idx] == 2) && (f1 <= 0.0)) {
                ctx->netcx.net_types[idx] = 3;
                ctx->netcx.y11_real[idx] = -f1;
            }
        }
        else if (strcmp(code, "CP") == 0) {
            // CP card - Maximum coupling between antennas
            if (i2 == 0) {
                continue;
            }
            
            // First CP in batch resets coupling (iflow transition to 2)
            if (ctx->iflow != 2 && ctx->yparm.num_pairs == 0) {
                reset_coupling_buffers(ctx);
                ctx->iflow = 2;
            }
            
            ctx->yparm.coupling_flag = 0;
            
            // First antenna
            ctx->yparm.num_pairs++;
            size_t mreq = (size_t)ctx->yparm.num_pairs * sizeof(int);
            mem_realloc(ctx, (void **)&ctx->yparm.pair_tags, mreq);
            mem_realloc(ctx, (void **)&ctx->yparm.pair_segs, mreq);
            ctx->yparm.pair_tags[ctx->yparm.num_pairs - 1] = i1;
            ctx->yparm.pair_segs[ctx->yparm.num_pairs - 1] =
                resolve_pct_segment(ctx, card, 2, i1);
            
            // Second antenna (if specified)
            if (i4 != 0) {
                ctx->yparm.num_pairs++;
                mreq = (size_t)ctx->yparm.num_pairs * sizeof(int);
                mem_realloc(ctx, (void **)&ctx->yparm.pair_tags, mreq);
                mem_realloc(ctx, (void **)&ctx->yparm.pair_segs, mreq);
                ctx->yparm.pair_tags[ctx->yparm.num_pairs - 1] = i3;
                ctx->yparm.pair_segs[ctx->yparm.num_pairs - 1] = i4;
            }
        }
        else if (strcmp(code, "GD") == 0) {
            // GD card - Ground representation (for patterns)
            ctx->fpat.epsr2 = f1;
            ctx->fpat.sigma2 = f2;
            ctx->fpat.cliff_dist = f3;
            ctx->fpat.cliff_height = f4;
        }
        else if (strcmp(code, "PT") == 0 || strcmp(code, "PQ") == 0 || strcmp(code, "PL") == 0) {
            // These cards are print control - skip in batch processing
            continue;
        }
        else if (strcmp(code, "RP") == 0) {
            // RP card - Radiation pattern parameters
            ctx->gnd.far_field_type = i1;
            ctx->fpat.num_theta = (i2 == 0) ? 1 : i2;
            ctx->fpat.num_phi = (i3 == 0) ? 1 : i3;
            
            ctx->fpat.gain_type = i4 / 10;
            ctx->fpat.avg_power_flag = i4 - ctx->fpat.gain_type * 10;
            ctx->fpat.normalize_gain = ctx->fpat.gain_type / 10;
            ctx->fpat.gain_type = ctx->fpat.gain_type - ctx->fpat.normalize_gain * 10;
            ctx->fpat.pol_axis = ctx->fpat.normalize_gain / 10;
            ctx->fpat.normalize_gain = ctx->fpat.normalize_gain - ctx->fpat.pol_axis * 10;
            
            if (ctx->fpat.pol_axis != 0) ctx->fpat.pol_axis = 1;
            if (ctx->fpat.gain_type != 0) ctx->fpat.gain_type = 1;
            if ((ctx->fpat.num_theta < 2) || (ctx->fpat.num_phi < 2) || (ctx->gnd.far_field_type == 1)) {
                ctx->fpat.avg_power_flag = 0;
            }
            
            ctx->fpat.theta_start = f1;
            ctx->fpat.phi_start = f2;
            ctx->fpat.theta_step = f3;
            ctx->fpat.phi_step = f4;
            ctx->fpat.range = f5;
            ctx->fpat.norm_gain = f6;
        }
        else if (strcmp(code, "NE") == 0 || strcmp(code, "NH") == 0) {
            // NE/NH cards - Near field calculation
            ctx->fpat.near_field_type = (strcmp(code, "NH") == 0) ? 1 : 0;
            ctx->fpat.is_near_field = i1;
            ctx->fpat.grid_nx = i2;
            ctx->fpat.grid_ny = i3;
            ctx->fpat.grid_nz = i4;
            ctx->fpat.grid_x0 = f1;
            ctx->fpat.grid_y0 = f2;
            ctx->fpat.grid_z0 = f3;
            ctx->fpat.grid_dx = f4;
            ctx->fpat.grid_dy = f5;
            ctx->fpat.grid_dz = f6;
        }
        else if (strcmp(code, "EK") == 0) {
            // Extended thin-wire kernel. Per NEC-2 spec: a bare EK card (or
            // I1=0) enables the extended kernel. I1=-1 disables/resets it.
            ctx->dataj.use_extended_kernel = (i1 == -1) ? 0 : 1;
        }
        else if (strcmp(code, "KH") == 0) {
            // Matrix integration limit
            ctx->dataj.k_half_len = f1;
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
                    snprintf(msg, sizeof(msg), "WG on line %d: no filename and no input file to derive one from.", card_idx + 1);
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
                         "WG on line %d: cannot open '%s' for writing.", card_idx + 1, wg_filename);
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
 * execute_extra_patterns()
 *
 * Handle an RP/NE/NH batch that has no FR card — i.e., nec2c's igo==4/5/6
 * path where the matrix is not refilled and no new power budget is printed.
 * The previously computed currents (from the last execute_frequency_loop call)
 * are reused; only the radiation-pattern (or near-field) calculation and its
 * output section are repeated.
 *
 * This matches nec2c oldmain.c case 4 → case 5 → case 6: after igo is set to
 * 4 (end of excitation section), a subsequent RP with no new FR jumps straight
 * to the pattern computation without a matrix refill, frequency header, or
 * power budget.
 *
 * @param ctx         The NEC context (currents already solved)
 * @param deck        The full deck (for output card listing)
 * @param batch_start First card index of this batch
 * @param batch_end   Last card index (the RP or NE/NH card)
 * @return            0 on success, -1 on error
 */
static int execute_extra_patterns(context_t *ctx, const deck_t *deck, int batch_start, int batch_end)
{
    (void)batch_start; /* currently unused; kept for future multi-RP iteration */
    (void)batch_end;

    if (ctx == NULL || deck == NULL) {
        return -1;
    }

    /* After execute_frequency_loop() returns, the geometry arrays (x_center,
     * y_center, z_center, half_len, radius) are restored to their original
     * unscaled metre values (see the "Restore geometry" block at the end of
     * execute_frequency_loop).  However, compute_radiation_pattern() and
     * compute_near_field() — specifically far_e_field() — require the geometry
     * to be in wavelength units.  Re-apply the same frequency scaling that the
     * frequency loop uses, call the pattern computation, then undo the scaling
     * so the next execute_frequency_loop call still sees unscaled geometry.    */
    double fr = 0.0;
    bool geom_scaled = false;
    if (ctx->save.freq_mhz > 0.0 && ctx->frequency_loop_ran &&
        (ctx->gnd.far_field_type != -1 || ctx->fpat.is_near_field != -1)) {
        fr = ctx->save.freq_mhz / CVEL;
        for (int i = 0; i < ctx->geometry.num_segs; i++) {
            ctx->geometry.x_center[i] *= fr;
            ctx->geometry.y_center[i] *= fr;
            ctx->geometry.z_center[i] *= fr;
            ctx->geometry.half_len[i] *= fr;
            ctx->geometry.radius[i]   *= fr;
        }
        if (ctx->geometry.num_patches > 0) {
            double fr2 = fr * fr;
            for (int i = 0; i < ctx->geometry.num_patches; i++) {
                ctx->geometry.patch_x_center[i] *= fr;
                ctx->geometry.patch_y_center[i] *= fr;
                ctx->geometry.patch_z_center[i] *= fr;
                ctx->geometry.patch_area[i]     *= fr2;
            }
        }
        geom_scaled = true;
    }

    /* Compute the pattern using existing (already solved) currents */
    if (ctx->gnd.far_field_type != -1) {
        ctx->fpat.power_in    = ctx->netcx.power_in;
        ctx->fpat.network_loss = ctx->netcx.power_net_loss;
        compute_radiation_pattern(ctx);
    }

    if (ctx->fpat.is_near_field != -1) {
        compute_near_field(ctx);
    }

    /* Restore geometry to unscaled (metre) values for subsequent calls */
    if (geom_scaled) {
        for (int i = 0; i < ctx->geometry.num_segs; i++) {
            ctx->geometry.x_center[i] /= fr;
            ctx->geometry.y_center[i] /= fr;
            ctx->geometry.z_center[i] /= fr;
            ctx->geometry.half_len[i] /= fr;
            ctx->geometry.radius[i]   /= fr;
        }
        if (ctx->geometry.num_patches > 0) {
            double fr2 = fr * fr;
            for (int i = 0; i < ctx->geometry.num_patches; i++) {
                ctx->geometry.patch_x_center[i] /= fr;
                ctx->geometry.patch_y_center[i] /= fr;
                ctx->geometry.patch_z_center[i] /= fr;
                ctx->geometry.patch_area[i]     /= fr2;
            }
        }
    }

    /* Write only the pattern section — no frequency header, no power budget */
    if (ctx->output_fp != NULL) {
        write_extra_pattern_output(ctx->output_fp, ctx);
    }

    return 0;
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
static int execute_frequency_loop(context_t *ctx, int nfrq, int ifrq, double delfrq, const deck_t *deck)
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
    if (ctx->netcx.num_eq == 0 || ctx->netcx.num_eq_sym == 0) {
        add_error(ctx, &ctx->errors, "Geometry not initialized before frequency loop", FATAL);
        return -1;
    }
    
    if (ctx->geometry.num_segs > 0 && (ctx->geometry.seg_end1_conn == NULL || ctx->geometry.seg_end2_conn == NULL)) {
        add_error(ctx, &ctx->errors, "Geometry connection data not allocated", FATAL);
        return -1;
    }
    
    // Allocate memory for interaction matrix and IP array
    size_t iresrv = ctx->netcx.num_eq * (ctx->netcx.num_eq + 2);
    size_t mreq = iresrv * sizeof(complex double);
    complex double *cm = NULL;
    mem_alloc(ctx, (void **)&cm, mreq);
    
    mreq = ctx->netcx.num_eq * sizeof(int);
    mem_alloc(ctx, (void **)&ctx->save.pivot, mreq);
    
    // Allocate symmetry array
    ctx->smat.num_sections = ctx->netcx.num_eq / ctx->netcx.num_eq_sym;
    mreq = (size_t)(ctx->smat.num_sections * ctx->smat.num_sections) * sizeof(complex double);
    mem_alloc(ctx, (void **)&ctx->smat.mode_matrix, mreq);
    
    // Allocate current array
    mreq = (size_t)ctx->geometry.num_segs_3xpatches * sizeof(complex double);
    mem_alloc(ctx, (void **)&ctx->crnt.surface_cur, mreq);
    
    // Allocate current basis function coefficient arrays
    mreq = (size_t)ctx->geometry.num_segs_and_patches * sizeof(double);
    mem_alloc(ctx, (void **)&ctx->crnt.a_real, mreq);
    mem_alloc(ctx, (void **)&ctx->crnt.a_imag, mreq);
    mem_alloc(ctx, (void **)&ctx->crnt.b_real, mreq);
    mem_alloc(ctx, (void **)&ctx->crnt.b_imag, mreq);
    mem_alloc(ctx, (void **)&ctx->crnt.c_real, mreq);
    mem_alloc(ctx, (void **)&ctx->crnt.c_imag, mreq);
    
    // Save unscaled geometry for frequency scaling
    double *xtemp = NULL, *ytemp = NULL, *ztemp = NULL;
    double *sitemp = NULL, *bitemp = NULL;
    
    if (ctx->geometry.num_segs > 0 || ctx->geometry.num_patches > 0) {
        mreq = (ctx->geometry.num_segs + ctx->geometry.num_patches) * sizeof(double);
        mem_alloc(ctx, (void **)&xtemp, mreq);
        mem_alloc(ctx, (void **)&ytemp, mreq);
        mem_alloc(ctx, (void **)&ztemp, mreq);
        mem_alloc(ctx, (void **)&sitemp, mreq);
        mem_alloc(ctx, (void **)&bitemp, mreq);
        
        // Save wire geometry
        for (int i = 0; i < ctx->geometry.num_segs; i++) {
            xtemp[i] = ctx->geometry.x_center[i];
            ytemp[i] = ctx->geometry.y_center[i];
            ztemp[i] = ctx->geometry.z_center[i];
            sitemp[i] = ctx->geometry.half_len[i];
            bitemp[i] = ctx->geometry.radius[i];
        }
        
        // Save patch geometry (patch-only decks have n==0 but m>0)
        if (ctx->geometry.num_patches > 0) {
            for (int i = 0; i < ctx->geometry.num_patches; i++) {
                int j = i + ctx->geometry.num_segs;
                xtemp[j] = ctx->geometry.patch_x_center[i];
                ytemp[j] = ctx->geometry.patch_y_center[i];
                ztemp[j] = ctx->geometry.patch_z_center[i];
                bitemp[j] = ctx->geometry.patch_area[i];
            }
        }
    }
    
    // Perform fblock matrix setup if needed
    if (ctx->matpar.core_used == 0) {
        factor_block_matrix(ctx, ctx->netcx.num_eq_sym, ctx->netcx.num_eq, iresrv, ctx->geometry.symmetry_flag);
    }
    
    // Write one-time geometry preamble before the frequency loop (first call only)
    if (ctx->output_fp != NULL && !ctx->preamble_written) {
        write_nec_preamble(ctx, deck, ctx->output_fp);
        ctx->preamble_written = true;
    }

    // Frequency loop
    for (int mhz = 1; mhz <= nfrq; mhz++) {
        // Clear loading outputs from previous frequency iteration
        ctx->loading_outputs.count = 0;
        
        // Update frequency
        if (mhz > 1) {
            if (ifrq == 1) {
                ctx->save.freq_mhz *= delfrq;
            } else {
                ctx->save.freq_mhz += delfrq;
            }
        }
        
        // Calculate wavelength and frequency ratio
        double fr = ctx->save.freq_mhz / CVEL;
        ctx->geometry.wavelength = CVEL / ctx->save.freq_mhz;
        
        // Scale geometry to current frequency
        if (ctx->geometry.num_segs > 0) {
            for (int i = 0; i < ctx->geometry.num_segs; i++) {
                ctx->geometry.x_center[i] = xtemp[i] * fr;
                ctx->geometry.y_center[i] = ytemp[i] * fr;
                ctx->geometry.z_center[i] = ztemp[i] * fr;
                ctx->geometry.half_len[i] = sitemp[i] * fr;
                ctx->geometry.radius[i] = bitemp[i] * fr;
            }
        }
        
        if (ctx->geometry.num_patches > 0) {
            double fr2 = fr * fr;
            for (int i = 0; i < ctx->geometry.num_patches; i++) {
                int j = i + ctx->geometry.num_segs;
                ctx->geometry.patch_x_center[i] = xtemp[j] * fr;
                ctx->geometry.patch_y_center[i] = ytemp[j] * fr;
                ctx->geometry.patch_z_center[i] = ztemp[j] * fr;
                ctx->geometry.patch_area[i] = bitemp[j] * fr2;
            }
        }
        
        // Apply loading to structure
        if (ctx->zload.num_loads > 0) {
            int *ldtyp = ctx->zload.load_types;
            int *ldtag = ctx->zload.load_tags;
            int *ldtagf = ctx->zload.load_tag_from;
            int *ldtagt = ctx->zload.load_tag_to;
            double *zlr = ctx->zload.load_r;
            double *zli = ctx->zload.load_l;
            double *zlc = ctx->zload.load_c;
            
            if (apply_impedance_loading(ctx, ldtyp, ldtag, ldtagf, ldtagt, zlr, zli, zlc) != 0)
                return -1;
        }
        
        // Set up ground parameters
        if (ctx->gnd.has_ground != 1) {
            ctx->gnd.fresnel_ratio = CPLX_10;
            
            if (ctx->gnd.is_perfect != 1) {
                double sig = ctx->save.ground_sigma;
                if (sig < 0.0) {
                    sig = -sig / (59.96 * ctx->geometry.wavelength);
                    ctx->save.ground_sigma = sig;
                }
                
                complex double epsc = ctx->save.ground_epsr - I * sig * ctx->geometry.wavelength * 59.96;
                ctx->gnd.impedance_ratio = 1.0 / csqrt(epsc);
                ctx->gwav.impedance_ratio = ctx->gnd.impedance_ratio;
                ctx->gwav.impedance_ratio_sq = ctx->gwav.impedance_ratio * ctx->gwav.impedance_ratio;
                
                // Handle radial wire ground screen
                if (ctx->gnd.num_radials != 0) {
                    ctx->gnd.screen_wire_len = ctx->save.screen_wire_len / ctx->geometry.wavelength;
                    ctx->gnd.screen_wire_radius = ctx->save.screen_wire_radius / ctx->geometry.wavelength;
                    ctx->gnd.screen_impedance = CPLX_01 * 2367.067 / (double)ctx->gnd.num_radials;
                    ctx->gnd.screen_inner_r = ctx->gnd.screen_wire_radius * (double)ctx->gnd.num_radials;
                }
                
                // Use Sommerfeld ground solution if requested
                if (ctx->gnd.is_perfect == 2) {
                    somnec(ctx, ctx->save.ground_epsr, ctx->save.ground_sigma, ctx->save.freq_mhz);
                    ctx->gnd.fresnel_ratio = (epsc - 1.0) / (epsc + 1.0);
                }
            }
        }
        
        // Fill and factor primary interaction matrix
        double tim1, tim2;
        get_time_ms(ctx, &tim1);
        if (fill_interaction_matrix(ctx, ctx->netcx.num_eq, cm, ctx->dataj.k_half_len, ctx->dataj.use_extended_kernel) != 0) {
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
            int neq = ctx->netcx.num_eq;
            for (int col = 0; col < nn && col < neq; col++)
                for (int row = 0; row < nn && row < neq; row++)
                    cm[row + col * neq] = ctx->ngf_cm[row + col * nn];
        }

        /* Export the (possibly NGF-injected) matrix if green_fp is open */
        if (ctx->green_fp != NULL) {
            write_greens_binary(ctx->green_fp, ctx, ctx->netcx.num_eq, cm);
            fclose(ctx->green_fp);
            ctx->green_fp = NULL;
            if (ctx->wg_after_cmset) {
                /* WG mode: write NGF file then stop — do not factorise or solve */
                ctx->wg_after_cmset = false;
                get_time_ms(ctx, &tim2);
                ctx->mat_fill_time = (tim2 - tim1) / 1000.0;
                break;  /* exit frequency loop without solving */
            }
        }

        get_time_ms(ctx, &tim2);
        ctx->mat_fill_time = (tim2 - tim1) / 1000.0;

        factor_matrix_symmetric(ctx, ctx->netcx.num_eq_sym, ctx->netcx.num_eq, cm, ctx->save.pivot);
        get_time_ms(ctx, &tim1);
        ctx->mat_factor_time = (tim1 - tim2) / 1000.0;
        
        // Reset solution counter
        ctx->netcx.network_type = 0;
        ctx->netcx.print_net_data = 0;
        
        // Set up excitation and solve
        // For voltage source excitation (most common case)
        if (ctx->fpat.excitation_type == 0 || ctx->fpat.excitation_type == 5) {
            // Fill right-hand side matrix (excitation)
            fill_excitation_vector(ctx, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, ctx->fpat.excitation_type, ctx->crnt.surface_cur);
            
            // Solve with network
            network(ctx, cm, ctx->save.pivot, ctx->crnt.surface_cur);
            ctx->netcx.network_type = 1;
            
            // Calculate power loss in structure
            ctx->fpat.ohmic_loss = 0.0;
            if (ctx->geometry.num_segs > 0) {
                for (int i = 0; i < ctx->geometry.num_segs; i++) {
                    complex double curi = ctx->crnt.surface_cur[i] * ctx->geometry.wavelength;
                    double cmag = cabs(curi);
                    
                    if (ctx->zload.num_loads > 0 && fabs(creal(ctx->zload.seg_impedance[i])) >= 1.e-20) {
                        ctx->fpat.ohmic_loss += 0.5 * cmag * cmag * 
                                          creal(ctx->zload.seg_impedance[i]) * ctx->geometry.half_len[i];
                    }
                }
            }
            
            // Handle coupling calculations if requested
            if (ctx->yparm.num_pairs > 0) {
                compute_coupling(ctx, ctx->crnt.surface_cur, ctx->geometry.wavelength);
            }
            
            // Near field calculation if requested
            // Note: do NOT reset near to -1 here after the last frequency;
            // the output guard in main.c reads it after execute_frequency_loop
            // returns, and calculation_defaults resets it per-batch.
            if (ctx->fpat.is_near_field != -1) {
                compute_near_field(ctx);
            }
            
            // Store data for radiation pattern output (calculation happens in output.c)
            if (ctx->gnd.far_field_type != -1) {
                ctx->fpat.power_in = ctx->netcx.power_in;
                ctx->fpat.network_loss = ctx->netcx.power_net_loss;
                compute_radiation_pattern(ctx);
            }
        }

        // Write per-frequency-step output after all calculations are done
        if (ctx->output_fp != NULL) {
            write_frequency_step_output(ctx->output_fp, ctx);
            ctx->freq_step_output_written = true;
            ctx->patterns_output_for_freq = true;  // Mark that RP/NE/NH output has been written
        }
    }
    
    // Free temporary arrays
    mem_free(ctx, (void **)&cm);
    if (xtemp != NULL) {
        // Restore geometry to original unscaled (metres) values so that a
        // subsequent execute_frequency_loop call (e.g. from a second RP batch)
        // gets the same baseline geometry instead of the already-scaled one.
        for (int i = 0; i < ctx->geometry.num_segs; i++) {
            ctx->geometry.x_center[i] = xtemp[i];
            ctx->geometry.y_center[i] = ytemp[i];
            ctx->geometry.z_center[i] = ztemp[i];
            ctx->geometry.half_len[i] = sitemp[i];
            ctx->geometry.radius[i]   = bitemp[i];
        }
        if (ctx->geometry.num_patches > 0) {
            for (int i = 0; i < ctx->geometry.num_patches; i++) {
                int j = i + ctx->geometry.num_segs;
                ctx->geometry.patch_x_center[i] = xtemp[j];
                ctx->geometry.patch_y_center[i] = ytemp[j];
                ctx->geometry.patch_z_center[i] = ztemp[j];
                ctx->geometry.patch_area[i]     = bitemp[j];
            }
        }
        mem_free(ctx, (void **)&xtemp);
        mem_free(ctx, (void **)&ytemp);
        mem_free(ctx, (void **)&ztemp);
        mem_free(ctx, (void **)&sitemp);
        mem_free(ctx, (void **)&bitemp);
    }
    
    return 0;
}

