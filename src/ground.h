/*
 * ground.h - Ground-related field functions for OpenNEC
 * 
 * Integration and field functions for ground effects.
 * Used by fields.c.
 */

#ifndef GROUND_H
#define GROUND_H

#include "types.h"

/* Ground field functions - called from fields.c */
int rom2(nec_context_t *restrict ctx, double a, double b, complex double *restrict sum, double dmin);
void sflds(nec_context_t *restrict ctx, double t, complex double *restrict e);

#endif /* GROUND_H */
