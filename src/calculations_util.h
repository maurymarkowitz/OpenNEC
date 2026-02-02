/*
 * calculations_util.h - Math utility functions for OpenNEC
 * 
 * Widely-used mathematical utilities for dB conversions and
 * complex number operations.
 */

#ifndef CALCULATIONS_UTIL_H
#define CALCULATIONS_UTIL_H

#include "types.h"

/* dB conversion utilities */
double db10(nec_context_t *ctx, double x);
double db20(nec_context_t *ctx, double x);

/* Complex number utilities */
double cang(nec_context_t *ctx, complex double z);

/* Simple utilities */
int min(nec_context_t *ctx, int a, int b);

#endif /* CALCULATIONS_UTIL_H */
