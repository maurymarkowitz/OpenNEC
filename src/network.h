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
void network(nec_context_t *ctx, complex double *cm, int *ip, complex double *einc);

#endif /* NETWORK_H */
