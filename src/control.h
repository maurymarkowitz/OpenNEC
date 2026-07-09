/*
 * control.h - Simulation control for OpenNEC
 * 
 * Main simulation entry point.
 */

#ifndef CONTROL_H
#define CONTROL_H

#include "types.h"

/* Main simulation entry point - called from main.c */
int run_simulation(context_t *ctx, deck_t *deck);

/* Process queued EX cards for excitation (called from sequential processor) */
int process_ex_batch(context_t *ctx);

#endif /* CONTROL_H */
