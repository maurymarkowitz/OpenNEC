/*
 * radiation.h - Radiation pattern computation for OpenNEC
 * 
 * Radiation pattern calculations.
 * Used by control.c.
 */

#ifndef RADIATION_H
#define RADIATION_H

#include "types.h"

/* Radiation pattern - called from control.c */
/* Formerly nec2c: rdpat */
void compute_radiation_pattern(nec_context_t *ctx);

#endif /* RADIATION_H */
