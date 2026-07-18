/*
 * control.h - Simulation control for OpenNEC
 * 
 * Main simulation entry point.
 */

#ifndef CONTROL_H
#define CONTROL_H

#include "types.h"

/* Main simulation entry point - DEPRECATED - BATCH MODE ONLY
 * 
 * This implements the legacy batch processing system that groups cards into
 * batches bounded by XQ cards. The active sequential processing pathway uses
 * process_deck_sequential() in reporting.c instead.
 * 
 * Called from main.c for backward compatibility but should not be used in new code.
 */
int run_simulation(context_t *ctx, deck_t *deck);

/* Process queued EX cards for excitation (called from sequential processor) */
int process_ex_batch(context_t *ctx);

#endif /* CONTROL_H */
