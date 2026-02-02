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
void rdpat(nec_context_t *ctx);

#endif /* RADIATION_H */
