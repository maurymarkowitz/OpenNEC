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

#endif /* CONTROL_H */
