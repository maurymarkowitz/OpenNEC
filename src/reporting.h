/*
 * reporting.h - produces NEC-2 style output reports for OpenNEC
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


#ifndef REPORTING_H
#define REPORTING_H

#include "types.h"
#include "opennec.h"

/**
 * @struct card_state_t
 * @brief State variables maintained during card-by-card sequential processing.
 *
 * Corresponds to Fortran NEC-2 state variables:
 * - processing_stage (formerly IGO): Processing stage (1-5)
 * - card_sequence_state (formerly IFLOW): Card sequence flow (1-12)
 * - num_frequencies (formerly NFRQ): Number of frequencies
 * - current_frequency_mhz (formerly MHZ): Current frequency index
 * - etc.
 *
 * This structure persists across the entire deck (EN card), with some fields
 * resetting when specific card types are encountered.
 */
typedef struct {
    /* Card counting (for output reporting) */
    int total_cards_processed;  /**< mpcnt - Total cards processed (for echoing cards) */
    
    /* Processing stage - Fortran IGO equivalent */
    int processing_stage;       /**< igo - Stage: 1=need_matrix, 2=have_matrix,
                                    3=excitation_ready, 4=solved, 5=complete */
    
    /* Card sequence flow - Fortran IFLOW equivalent */
    int card_sequence_state;    /**< iflow - Flow: 1=FR, 2=CP, 3=LD, 4=GN, 
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
                        card_state_t *state);

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
                                            card_state_t *state);

/* Card-specific processor functions (Fortran labels 16-39, nec2c cases 0-17) */

/**
 * @brief Process FR (frequency) card.
 * @param ctx The simulation context.
 * @param card The FR card.
 * @param state Card state (updated).
 * @return 0 on success, -1 on error.
 */
static int process_fr_card(context_t *ctx, const card_t *card, card_state_t *state);

/**
 * @brief Process LD (loading) card.
 * @param ctx The simulation context.
 * @param card The LD card.
 * @param state Card state (updated).
 * @return 0 on success, -1 on error.
 */
static int process_ld_card(context_t *ctx, const card_t *card, card_state_t *state);

/**
 * @brief Process GN (ground) card.
 * @param ctx The simulation context.
 * @param card The GN card.
 * @param state Card state (updated).
 * @return 0 on success, -1 on error.
 */
static int process_gn_card(context_t *ctx, const card_t *card, card_state_t *state);

/**
 * @brief Process EX (excitation) card.
 * @param ctx The simulation context.
 * @param card The EX card.
 * @param state Card state (updated).
 * @return 0 on success, -1 on error.
 */
static int process_ex_card(context_t *ctx, const card_t *card, card_state_t *state);

/**
 * @brief Process NT/TL (network) card.
 * @param ctx The simulation context.
 * @param card The NT or TL card.
 * @param state Card state (updated).
 * @return 0 on success, -1 on error.
 */
static int process_nt_tl_card(context_t *ctx, const card_t *card, card_state_t *state);

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
                          card_state_t *state);

/**
 * @brief Process RP (radiation pattern) card.
 * @param ctx The simulation context.
 * @param card The RP card.
 * @param state Card state (updated).
 * @return 0 on success, -1 on error.
 */
static int process_rp_card(context_t *ctx, const card_t *card, card_state_t *state);

/**
 * @brief Process NE (near-field equatorial) card.
 * @param ctx The simulation context.
 * @param card The NE card.
 * @param state Card state (updated).
 * @return 0 on success, -1 on error.
 */
static int process_ne_card(context_t *ctx, const card_t *card, card_state_t *state);

/**
 * @brief Process NH (near-field horizontal) card.
 * @param ctx The simulation context.
 * @param card The NH card.
 * @param state Card state (updated).
 * @return 0 on success, -1 on error.
 */
static int process_nh_card(context_t *ctx, const card_t *card, card_state_t *state);

/**
 * @brief Process PT (print control - currents) card.
 * @param ctx The simulation context.
 * @param card The PT card.
 * @param state Card state (updated).
 * @return 0 on success, -1 on error.
 */
static int process_pt_card(context_t *ctx, const card_t *card, card_state_t *state);

/**
 * @brief Process PQ (print control - charges) card.
 * @param ctx The simulation context.
 * @param card The PQ card.
 * @param state Card state (updated).
 * @return 0 on success, -1 on error.
 */
static int process_pq_card(context_t *ctx, const card_t *card, card_state_t *state);

/**
 * @brief Process KH (matrix limit) card.
 * @param ctx The simulation context.
 * @param card The KH card.
 * @param state Card state (updated).
 * @return 0 on success, -1 on error.
 */
static int process_kh_card(context_t *ctx, const card_t *card, card_state_t *state);

/**
 * @brief Process EK (extended kernel) card.
 * @param ctx The simulation context.
 * @param card The EK card.
 * @param state Card state (updated).
 * @return 0 on success, -1 on error.
 */
static int process_ek_card(context_t *ctx, const card_t *card, card_state_t *state);

/**
 * @brief Process CP (coupling) card.
 * @param ctx The simulation context.
 * @param card The CP card.
 * @param state Card state (updated).
 * @return 0 on success, -1 on error.
 */
static int process_cp_card(context_t *ctx, const card_t *card, card_state_t *state);

/**
 * @brief Process GD (ground detail) card.
 * @param ctx The simulation context.
 * @param card The GD card.
 * @param state Card state (updated).
 * @return 0 on success, -1 on error.
 */
static int process_gd_card(context_t *ctx, const card_t *card, card_state_t *state);

/**
 * @brief Process NX (next structure) card.
 * @param ctx The simulation context.
 * @param deck The parsed NEC input deck.
 * @param card_idx Index of the NX card.
 * @param state Card state (updated).
 * @return 0 on success, -1 on error.
 */
static int process_nx_card(context_t *ctx, deck_t *deck, int card_idx,
                          card_state_t *state);

/* Utility functions for geometry scaling during frequency loop */

/**
 * @brief Save geometry for frequency scaling (called once before frequency loop).
 * @param ctx The simulation context.
 * @param state Card state (geometry saved into state).
 * @return 0 on success, -1 on memory allocation failure.
 */
static int save_geometry_for_scaling(context_t *ctx, card_state_t *state);

/**
 * @brief Scale geometry to wavelength units for current frequency.
 * @param ctx The simulation context (geometry modified in place).
 * @param state Card state (contains original unscaled geometry).
 * @param fr Frequency scaling factor (frequency / speed of light).
 */
static void scale_geometry_for_frequency(context_t *ctx, const card_state_t *state,
                                        double fr);

/**
 * @brief Restore geometry to original unscaled values.
 * @param ctx The simulation context (geometry restored from state).
 * @param state Card state (contains original unscaled geometry).
 */
static void restore_geometry(context_t *ctx, const card_state_t *state);

#endif /* REPORTING_H */
