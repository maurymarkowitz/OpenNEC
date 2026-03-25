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
/* Formerly nec2c: nefld */
void near_e_field(context_t *restrict ctx, double xob, double yob, double zob, complex double *restrict ex, complex double *restrict ey, complex double *restrict ez);
/* Formerly nec2c: nhfld */
void near_h_field(context_t *restrict ctx, double xob, double yob, double zob, complex double *restrict hx, complex double *restrict hy, complex double *restrict hz);
/* Formerly nec2c: nfpat */
void compute_near_field(context_t *ctx);

/* Cross-module field functions - called from matrix.c, ground.c, radiation.c */
/* Formerly nec2c: efld */
void e_field_segment(context_t *ctx, double xi, double yi, double zi, double ai, int ij);
/* Formerly nec2c: gwave */
void ground_wave_field(context_t *restrict ctx, complex double *restrict erv, complex double *restrict ezv, complex double *restrict erh, complex double *restrict ezh, complex double *restrict eph);
/* Formerly nec2c: hintg */
void h_field_patch(context_t *ctx, double xi, double yi, double zi);
/* Formerly nec2c: hsfld */
void h_field_segment(context_t *ctx, double xi, double yi, double zi, double ai);
/* Formerly nec2c: pcint */
void integrate_patch_at_junction(context_t *restrict ctx, double xi, double yi, double zi, double cabi, double sabi, double salpi, complex double *restrict e);
/* Formerly nec2c: unere */
void e_field_unit_patch_current(context_t *ctx, double xob, double yob, double zob);

#endif /* FIELDS_H */
