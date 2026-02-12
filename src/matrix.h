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
void cmset(nec_context_t *restrict ctx, int nrow, complex double *restrict cm, double rkhx, int iexkx);
void factrs(nec_context_t *restrict ctx, int np, int nrow, complex double *restrict a, int *restrict ip);
int fblock(nec_context_t *ctx, int nrow, int ncol, int imax, int ipsym);
void etmns(nec_context_t *restrict ctx, double p1, double p2, double p3, double p4, double p5, double p6, int ipr, complex double *restrict e);

/* Linear system solvers - called from control.c and network.c */
void factr(const nec_context_t *restrict ctx, int n, complex double *restrict a, int *restrict ip, int ndim);
void solve(const nec_context_t *restrict ctx, int n, complex double *restrict a, int *restrict ip, complex double *restrict b, int ndim);
void solves(nec_context_t *restrict ctx, complex double *restrict a, int *restrict ip, complex double *restrict b, int neq, int nrh, int np, int n, int mp, int m);

#endif /* MATRIX_H */
