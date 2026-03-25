/*
 * matrix.h - Matrix operations for OpenNEC
 * 
 * Matrix setup, factorization, and linear system solvers.
 * Used by control.c and network.c.
 */

#ifndef MATRIX_H
#define MATRIX_H

#include "types.h"

/* Matrix setup - called from control.c */
/* Formerly nec2c: cmset */
int fill_interaction_matrix(context_t *restrict ctx, int nrow, complex double *restrict cm, double rkhx, int iexkx);
/* Formerly nec2c: factrs */
void factor_matrix_symmetric(context_t *restrict ctx, int np, int nrow, complex double *restrict a, int *restrict ip);
/* Formerly nec2c: fblock */
int factor_block_matrix(context_t *ctx, int nrow, int ncol, int imax, int ipsym);
/* Formerly nec2c: etmns */
void fill_excitation_vector(context_t *restrict ctx, double p1, double p2, double p3, double p4, double p5, double p6, int ipr, complex double *restrict e);

/* Linear system solvers - called from control.c and network.c */
/* Formerly nec2c: factr */
void factor_matrix(const context_t *restrict ctx, int n, complex double *restrict a, int *restrict ip, int ndim);
void solve(const context_t *restrict ctx, int n, complex double *restrict a, int *restrict ip, complex double *restrict b, int ndim);
/* Formerly nec2c: solves */
void solve_symmetric(context_t *restrict ctx, complex double *restrict a, int *restrict ip, complex double *restrict b, int neq, int nrh, int np, int n, int mp, int m);

#endif /* MATRIX_H */
