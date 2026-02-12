/*
 * fields.h - Field computation for OpenNEC
 * 
 * Near-field electric and magnetic field calculations.
 * Used by control.c.
 */

#ifndef FIELDS_H
#define FIELDS_H

#include "types.h"

/* Near-field computations - called from control.c */
void nefld(nec_context_t *restrict ctx, double xob, double yob, double zob, complex double *restrict ex, complex double *restrict ey, complex double *restrict ez);
void nhfld(nec_context_t *restrict ctx, double xob, double yob, double zob, complex double *restrict hx, complex double *restrict hy, complex double *restrict hz);
void nfpat(nec_context_t *ctx);

/* Cross-module field functions - called from matrix.c, ground.c, radiation.c */
void efld(nec_context_t *ctx, double xi, double yi, double zi, double ai, int ij);
void gwave(nec_context_t *restrict ctx, complex double *restrict erv, complex double *restrict ezv, complex double *restrict erh, complex double *restrict ezh, complex double *restrict eph);
void hintg(nec_context_t *ctx, double xi, double yi, double zi);
void hsfld(nec_context_t *ctx, double xi, double yi, double zi, double ai);
void pcint(nec_context_t *restrict ctx, double xi, double yi, double zi, double cabi, double sabi, double salpi, complex double *restrict e);
void unere(nec_context_t *ctx, double xob, double yob, double zob);

#endif /* FIELDS_H */
