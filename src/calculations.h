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
/* Formerly nec2c: cabc */
void compute_current_coefficients(nec_context_t *restrict ctx, complex double *restrict curx);
/* Formerly nec2c: couple */
void compute_coupling(nec_context_t *ctx, complex double *cur, double wlam);
/* Formerly nec2c: load */
int apply_impedance_loading(nec_context_t *ctx, int *ldtyp, int *ldtag, int *ldtagf, int *ldtagt, double *zlr, double *zli, double *zlc);

/* Numerical integration - called from fields.c and radiation.c */
/* Formerly nec2c: intrp */
void interpolate_sommerfeld_grid(nec_context_t *restrict ctx, double x, double y, complex double *restrict f1, complex double *restrict f2, complex double *restrict f3, complex double *restrict f4);
/* Formerly nec2c: intx */
void romberg_integrate_wire_e(nec_context_t *ctx, double el1, double el2, double b, int ij, double *sgr, double *sgi);


/* dB conversion utilities */
double db10(const nec_context_t *ctx, double x);
double db20(const nec_context_t *ctx, double x);

/* Complex number utilities */
/* Formerly nec2c: cang */
double complex_angle_deg(const nec_context_t *ctx, complex double z);

/* Simple utilities */
int min(const nec_context_t *ctx, int a, int b);

/* Internal calculation functions used within calculations.c and by other modules */
/* Formerly nec2c: zint */
void wire_surface_impedance(nec_context_t *restrict ctx, double sigl, double rolam, complex double *restrict zt);
/* Formerly nec2c: tbf */
int compute_basis_func(nec_context_t *ctx, int i, int icap);
/* Formerly nec2c: test */
void test_romberg_convergence(nec_context_t *ctx, double f1r, double f2r, double *tr, double f1i, double f2i, double *ti, double dmin);
/* Formerly nec2c: trio */
int compute_all_basis_funcs_on_seg(nec_context_t *ctx, int j);

#endif /* CALCULATIONS_H */
