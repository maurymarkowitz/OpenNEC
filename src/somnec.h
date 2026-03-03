/*
 * somnec.h - Sommerfeld-Norton ground computation for OpenNEC
 * 
 * Ground impedance calculations using Sommerfeld-Norton method.
 * Used by control.c.
 */

#ifndef SOMNEC_H
#define SOMNEC_H

#include "types.h"

/* Sommerfeld-Norton ground computation - called from control.c */
void somnec(nec_context_t *ctx, double epr, double sig, double fmhz);

/* Cross-module somnec functions */
/* Formerly nec2c: sflds */
void sommerfeld_field(nec_context_t *ctx, double t, complex double *e);
/* Formerly nec2c: fbar */
void norton_attenuation_factor(nec_context_t *ctx, complex double p, complex double *r);

#endif /* SOMNEC_H */
