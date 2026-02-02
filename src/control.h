/*
 * control.h - Simulation control for OpenNEC
 * 
 * Main simulation entry point.
 */

#ifndef CONTROL_H
#define CONTROL_H

#include "types.h"

/* Main simulation entry point - called from main.c */
int nec_run_simulation(nec_context_t *ctx, deck_t *deck);

/* Internal control functions */
int nec_calculation_defaults(nec_context_t *ctx);
int process_control_cards(nec_context_t *ctx, deck_t *deck);
int execute_frequency_loop(nec_context_t *ctx, int nfrq, int ifrq, double delfrq);

#endif /* CONTROL_H */
