/*
 * network.h - Network analysis for OpenNEC
 * 
 * Network analysis functions.
 * Used by control.c.
 */

#ifndef NETWORK_H
#define NETWORK_H

#include "types.h"

/* Network analysis - called from control.c */
void network(nec_context_t *restrict ctx, complex double *restrict cm, int *restrict ip, complex double *restrict einc);

#endif /* NETWORK_H */
