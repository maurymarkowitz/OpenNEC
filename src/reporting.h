/*
 * reporting.h - Card-by-Card Sequential Processing for OpenNEC
 *
 * Implements sequential (one-at-a-time) card processing following the
 * original Fortran NEC-2 / nec2c design pattern. This replaces the batch-based
 * approach in control.c with a simpler state machine that processes control
 * cards in order until EN (end of deck).
 *
 * Primary References:
 * - Fortran NEC: ~/Downloads/Nec2dXS_src/nec2dxs.f
 * - C port nec2c: ~/Developer/nec2c-1.3/main.c
 *
 * Key Differences from Batch Mode:
 * - Card handlers only set state, no output
 * - Frequency loop contains all fprintf inline
 * - Simpler state management driven by card order
 * - Output format uses Fortran style (OUTPUT_FORMAT_ORIGINAL)
 */

#ifndef REPORTING_H
#define REPORTING_H

#include "types.h"
#include "opennec.h"

/**
 * @struct card_state_t
 * @brief State variables maintained during card-by-card sequential processing.
 *
 * Corresponds to Fortran NEC-2 state variables:
 * - IGO: Processing stage (1-5)
 * - IFLOW: Card sequence flow (1-12)
 * - NFRQ: Number of frequencies
 * - MHZ: Current frequency index
 * - etc.
 *
 * This structure persists across the entire deck (EN card), with some fields
 * resetting when specific card types are encountered.
 */
typedef struct {
    int total_cards_processed;  /**< mpcnt - Total cards processed (for echoing cards) */
    
    int processing_stage;       /**< formerly igo - Stage: 1=need_matrix, 2=have_matrix,
                                    3=excitation_ready, 4=solved, 5=complete */
    
    int card_sequence_state;    /**< formerlyiflow - Flow: 1=FR, 2=CP, 3=LD, 4=GN, 
                                    5=EX, 6=NT/TL, 7=XQ/execute, etc. */
    
    /* Frequency loop control */
    int num_frequencies;        /**< nfrq - Number of frequencies to process */
    int freq_stepping_mode;     /**< ifrq - Mode: 0=linear, 1=multiplicative */
    double current_frequency_mhz; /**< fmhz - Current/base frequency in MHz */
    double frequency_delta;     /**< delfrq - Frequency step or multiplier */
    int freq_iteration;         /**< mhz - Current frequency index (1..num_frequencies) */
    
    /* Print control flags */
    int currents_print_control; /**< iptflg - Control flag for current output */
    int charges_print_control;  /**< iptflq - Control flag for charge output */
    int impedance_norm_type;    /**< iped - Norm type: 0=unity, 1=impedance, 2=admittance */
    double impedance_norm_value;/**< zpnorm - Impedance normalization value */
    
    /* Pattern control */
    int excitation_type;        /**< ixtyp - Excitation type for plane waves */
    int num_theta_angles;       /**< nthi - Number of theta angles */
    int num_phi_angles;         /**< nphi - Number of phi angles */
    
    /* Matrix parameters */
    double matrix_integration_limit; /**< rkh - Integration limit (wavelengths) */
    int use_extended_kernel;    /**< iexk - Extended kernel flag */
    
    /* Geometry scaling storage for frequency loop */
    double *wire_x_saved, *wire_y_saved, *wire_z_saved;  /**< xtemp, ytemp, ztemp - Saved wire center coordinates */
    double *wire_half_length_saved, *wire_radius_saved;  /**< sitemp, bitemp - Saved wire half-length and radius */
    double *patch_x_saved, *patch_y_saved, *patch_z_saved;  /**< patch_xtemp, patch_ytemp, patch_ztemp - Saved patch centers */
    double *patch_area_saved;   /**< patch_atemp - Saved patch areas */
    int wire_geometry_saved;    /**< ifrtmw - Flag: wire geometry saved (1=yes, 0=no) */
    int patch_geometry_saved;   /**< ifrtmp - Flag: patch geometry saved (1=yes, 0=no) */
    
    /* Pattern card processing - track which pattern cards were processed with XQ */
    int last_processed_pattern_idx; /**< Index of last RP/NE/NH card processed as part of current XQ */
    
    /* Multiple RP card handling - store parameters for all RP cards following current XQ */
    #define MAX_RP_CARDS_PER_FREQUENCY 20
    struct {
        int num_theta;
        int num_phi;
        double theta_start;
        double phi_start;
        double theta_step;
        double phi_step;
    } rp_cards[MAX_RP_CARDS_PER_FREQUENCY];
    int num_rp_cards;           /**< Number of RP cards collected from look-ahead */
    
    double last_freq_output_mhz;  /**< Last frequency for which frequency output was written */
    
} card_state_t;

/**
 * @brief Initialize a new card_state_t to default values.
 * @param state State structure to initialize.
 */
void init_card_state(card_state_t *state);

/**
 * @brief Free any allocated memory in a card_state_t.
 * @param state State structure to clean up.
 */
void free_card_state(card_state_t *state);

/**
 * @brief Main entry point: Process entire deck sequentially (one card at a time).
 *
 * Reads cards from deck in order, dispatches to card-specific handlers,
 * and processes frequency loops when XQ cards are encountered. Continues
 * until EN (end of deck) card is reached.
 *
 * Matches Fortran NEC-2 line 14 (main input loop).
 *
 * @param ctx The simulation context.
 * @param deck The parsed NEC input deck.
 * @return 0 on success, -1 on error.
 */
int process_deck_sequential(context_t *ctx, deck_t *deck);

/**
 * @brief Dispatch a single control card to its handler.
 *
 * Matches Fortran NEC-2 computed GOTO dispatch (lines 293-307).
 * Matches nec2c switch statement (lines 306-580).
 *
 * @param ctx The simulation context.
 * @param deck The parsed NEC input deck.
 * @param card_idx Index of the card to process.
 * @param state Current card state (updated by handler).
 * @return 0 on success, -1 on error.
 */
static int dispatch_card(context_t *ctx, deck_t *deck, int card_idx,
                        card_state_t *state) __attribute__((unused));

/**
 * @brief Execute the frequency loop (called when XQ card is encountered).
 *
 * Matches Fortran NEC-2 lines 41-120 (label 41 and frequency loop).
 * Matches nec2c lines 607-2025 (frequency loop do-while).
 *
 * All fprintf output is inline (Fortran format, not nec2c format).
 *
 * @param ctx The simulation context.
 * @param deck The parsed NEC input deck.
 * @param xq_card_idx Index of the XQ card that triggered execution.
 * @param state Current card state (updated by loop).
 * @return 0 on success, -1 on error.
 */
static int execute_frequency_loop_sequential(context_t *ctx, deck_t *deck,
                                            int xq_card_idx,
                                            card_state_t *state) __attribute__((unused));

/* Card-specific processor functions (Fortran labels 16-39, nec2c cases 0-17) */

/**
 * @brief Process FR (frequency) card.
 * @param ctx The simulation context.
 * @param card The FR card.
 * @param state Card state (updated).
 * @return 0 on success, -1 on error.
 */
static int process_fr_card(context_t *ctx, const card_t *card, card_state_t *state) __attribute__((unused));

/**
 * @brief Process LD (loading) card.
 * @param ctx The simulation context.
 * @param card The LD card.
 * @param state Card state (updated).
 * @return 0 on success, -1 on error.
 */
static int process_ld_card(context_t *ctx, const card_t *card, card_state_t *state) __attribute__((unused));

/**
 * @brief Process GN (ground) card.
 * @param ctx The simulation context.
 * @param card The GN card.
 * @param state Card state (updated).
 * @return 0 on success, -1 on error.
 */
static int process_gn_card(context_t *ctx, const card_t *card, card_state_t *state) __attribute__((unused));

/**
 * @brief Process EX (excitation) card.
 * @param ctx The simulation context.
 * @param card The EX card.
 * @param state Card state (updated).
 * @return 0 on success, -1 on error.
 */
static int process_ex_card(context_t *ctx, const card_t *card, card_state_t *state) __attribute__((unused));

/**
 * @brief Process NT/TL (network) card.
 * @param ctx The simulation context.
 * @param card The NT or TL card.
 * @param state Card state (updated).
 * @return 0 on success, -1 on error.
 */
static int process_nt_tl_card(context_t *ctx, const card_t *card, card_state_t *state) __attribute__((unused));

/**
 * @brief Process XQ (execute) card.
 *
 * Triggers the frequency loop execution. This is not a configuration card,
 * but marks the point where accumulated state should be processed.
 *
 * @param ctx The simulation context.
 * @param deck The parsed NEC input deck.
 * @param card_idx Index of the XQ card.
 * @param state Card state (updated).
 * @return 0 on success, -1 on error.
 */
static int process_xq_card(context_t *ctx, deck_t *deck, int card_idx,
                          card_state_t *state) __attribute__((unused));

/**
 * @brief Process RP (radiation pattern) card.
 * @param ctx The simulation context.
 * @param card The RP card.
 * @param state Card state (updated).
 * @return 0 on success, -1 on error.
 */
static int process_rp_card(context_t *ctx, const card_t *card, card_state_t *state) __attribute__((unused));

/**
 * @brief Process NE (near-field equatorial) card.
 * @param ctx The simulation context.
 * @param card The NE card.
 * @param state Card state (updated).
 * @return 0 on success, -1 on error.
 */
static int process_ne_card(context_t *ctx, const card_t *card, card_state_t *state) __attribute__((unused));

/**
 * @brief Process NH (near-field horizontal) card.
 * @param ctx The simulation context.
 * @param card The NH card.
 * @param state Card state (updated).
 * @return 0 on success, -1 on error.
 */
static int process_nh_card(context_t *ctx, const card_t *card, card_state_t *state) __attribute__((unused));

/**
 * @brief Process PT (print control - currents) card.
 * @param ctx The simulation context.
 * @param card The PT card.
 * @param state Card state (updated).
 * @return 0 on success, -1 on error.
 */
static int process_pt_card(context_t *ctx, const card_t *card, card_state_t *state) __attribute__((unused));

/**
 * @brief Process PQ (print control - charges) card.
 * @param ctx The simulation context.
 * @param card The PQ card.
 * @param state Card state (updated).
 * @return 0 on success, -1 on error.
 */
static int process_pq_card(context_t *ctx, const card_t *card, card_state_t *state) __attribute__((unused));

/**
 * @brief Process KH (matrix limit) card.
 * @param ctx The simulation context.
 * @param card The KH card.
 * @param state Card state (updated).
 * @return 0 on success, -1 on error.
 */
static int process_kh_card(context_t *ctx, const card_t *card, card_state_t *state) __attribute__((unused));

/**
 * @brief Process EK (extended kernel) card.
 * @param ctx The simulation context.
 * @param card The EK card.
 * @param state Card state (updated).
 * @return 0 on success, -1 on error.
 */
static int process_ek_card(context_t *ctx, const card_t *card, card_state_t *state) __attribute__((unused));

/**
 * @brief Process CP (coupling) card.
 * @param ctx The simulation context.
 * @param card The CP card.
 * @param state Card state (updated).
 * @return 0 on success, -1 on error.
 */
static int process_cp_card(context_t *ctx, const card_t *card, card_state_t *state) __attribute__((unused));

/**
 * @brief Process GD (ground detail) card.
 * @param ctx The simulation context.
 * @param card The GD card.
 * @param state Card state (updated).
 * @return 0 on success, -1 on error.
 */
static int process_gd_card(context_t *ctx, const card_t *card, card_state_t *state) __attribute__((unused));

/**
 * @brief Process NX (next structure) card.
 * @param ctx The simulation context.
 * @param deck The parsed NEC input deck.
 * @param card_idx Index of the NX card.
 * @param state Card state (updated).
 * @return 0 on success, -1 on error.
 */
static int process_nx_card(context_t *ctx, deck_t *deck, int card_idx,
                          card_state_t *state) __attribute__((unused));

/* Utility functions for geometry scaling during frequency loop */

/**
 * @brief Save geometry for frequency scaling (called once before frequency loop).
 * @param ctx The simulation context.
 * @param state Card state (geometry saved into state).
 * @return 0 on success, -1 on memory allocation failure.
 */
static int save_geometry_for_scaling(context_t *ctx, card_state_t *state) __attribute__((unused));

/**
 * @brief Scale geometry to wavelength units for current frequency.
 * @param ctx The simulation context (geometry modified in place).
 * @param state Card state (contains original unscaled geometry).
 * @param fr Frequency scaling factor (frequency / speed of light).
 */
static void scale_geometry_for_frequency(context_t *ctx, const card_state_t *state,
                                        double fr) __attribute__((unused));

/**
 * @brief Restore geometry to original unscaled values.
 * @param ctx The simulation context (geometry restored from state).
 * @param state Card state (contains original unscaled geometry).
 */
static void restore_geometry(context_t *ctx, const card_state_t *state) __attribute__((unused));

#endif /* REPORTING_H */
