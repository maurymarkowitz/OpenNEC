/*
 * calculations.h - Calculation functions for OpenNEC
 * 
 * Loading, coupling, and numerical integration functions.
 * Used by control.c and fields.c.
 */

#ifndef CALCULATIONS_H
#define CALCULATIONS_H

#include "types.h"

/* Loading and coupling - called from control.c */
void cabc(nec_context_t *ctx, complex double *curx);
void couple(nec_context_t *ctx, complex double *cur, double wlam);
int load(nec_context_t *ctx, int *ldtyp, int *ldtag, int *ldtagf, int *ldtagt, double *zlr, double *zli, double *zlc);

/* Numerical integration - called from fields.c and radiation.c */
void intrp(nec_context_t *ctx, double x, double y, complex double *f1, complex double *f2, complex double *f3, complex double *f4);
void intx(nec_context_t *ctx, double el1, double el2, double b, int ij, double *sgr, double *sgi);


/* dB conversion utilities */
double db10(nec_context_t *ctx, double x);
double db20(nec_context_t *ctx, double x);

/* Complex number utilities */
double cang(nec_context_t *ctx, complex double z);

/* Simple utilities */
int min(nec_context_t *ctx, int a, int b);

/* Internal calculation functions used within calculations.c and by other modules */
void zint(nec_context_t *ctx, double sigl, double rolam, complex double *zt);
int tbf(nec_context_t *ctx, int i, int icap);
void test(nec_context_t *ctx, double f1r, double f2r, double *tr, double f1i, double f2i, double *ti, double dmin);
int trio(nec_context_t *ctx, int j);

#endif /* CALCULATIONS_H */
