/**
 * @file report_logic.h
 * @brief Output and report generation functions.
 *
 * Contains the analog of the card reading/action loop from nec2d and nec2c,
 * calling the new functionality and then printing the results using the code
 * in report_output.c.
 * 
 * The code here corresponds roughly to lines 287 (label 14) through 1048
 * (near label 123) in nec2d, and lines 226 to 1866 in nec2c's main.c.
 * The original Fortran labels are preserved in comments for reference.
 * 
 * The code differs from the original fromec2c mostly in that the cards are
 * read post-parsed from the deck_t structure, rather than being read from a
 * file line-by-line. This makes the logic easier to follow. Additionally,
 * any formatted output is handled by the functions in report_output.c,
 * rather than being interspersed throughout the logic.
 */

#ifndef REPORT_LOGIC_H
#define REPORT_LOGIC_H

#include "opennec.h"
#include "types.h"
#include <stdio.h>

/**
 * @struct run_report
 * @brief Main entry point: Process entire deck sequentially (one card at a time).
 *
 */
int run_report(context_t *ctx, deck_t *deck);

/**
 * @struct report_state_t
 * @brief State variables maintained during report processing.
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
    
    int card_sequence_state;    /**< formerly iflow - Flow: 1=FR, 2=CP, 3=LD, 4=GN, 
                                    5=EX, 6=NT/TL, 7=XQ/execute, etc. */
    
    /* frequency loop control */
    int num_frequencies;        /**< formerly nfrq - number of frequencies to process, often 1 */
    int freq_iteration;         /**< formerly mhz - Current frequency index (1..num_frequencies) */
    int freq_stepping_mode;     /**< formerly ifrq - mode: 0=linear, 1=multiplicative */
    double current_frequency_mhz; /**< formerly fmhz - Current/base frequency in MHz */
    double frequency_delta;     /**< formerly delfrq - Frequency step or multiplier */
     
    /* print control flags */
    int currents_print_control; /**< iptflg - Control flag for current output */
    int charges_print_control;  /**< iptflq - Control flag for charge output */
    int impedance_norm_type;    /**< iped - Norm type: 0=unity, 1=impedance, 2=admittance */
    double impedance_norm_value;/**< zpnorm - Impedance normalization value */
    
    /* pattern control */
    int excitation_type;        /**< ixtyp - Excitation type for plane waves */
    int num_theta_angles;       /**< nthi - Number of theta angles */
    int num_phi_angles;         /**< nphi - Number of phi angles */
    
    /* matrix parameters */
    double matrix_integration_limit; /**< rkh - Integration limit (wavelengths) */
    int use_extended_kernel;    /**< iexk - Extended kernel flag */
    
    /* geometry scaling storage for frequency loop */
    double *wire_x_saved, *wire_y_saved, *wire_z_saved;  /**< xtemp, ytemp, ztemp - Saved wire center coordinates */
    double *wire_half_length_saved, *wire_radius_saved;  /**< sitemp, bitemp - Saved wire half-length and radius */
    double *patch_x_saved, *patch_y_saved, *patch_z_saved;  /**< patch_xtemp, patch_ytemp, patch_ztemp - Saved patch centers */
    double *patch_area_saved;   /**< patch_atemp - Saved patch areas */
    int wire_geometry_saved;    /**< ifrtmw - Flag: wire geometry saved (1=yes, 0=no) */
    int patch_geometry_saved;   /**< ifrtmp - Flag: patch geometry saved (1=yes, 0=no) */
    
    /* pattern card (PT) processing - track which pattern cards were processed with XQ */
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
    
} report_state_t;

/**
 * @brief Initialize a new report_state_t to default values.
 * @param state State structure to initialize.
 */
void init_report_state(report_state_t *state);

/**
 * @brief Free any allocated memory in a report_state_t.
 * @param state State structure to clean up.
 */
void free_report_state(report_state_t *state);

#endif // REPORT_LOGIC_H