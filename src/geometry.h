/*
 * geometry.h - Geometry computation for OpenNEC
 * 
 * Public interface for geometry calculations and segment resolution.
 */

#ifndef GEOMETRY_H
#define GEOMETRY_H

#include "types.h"

/* Geometry calculation - called from control.c */
void calculate_geometry(nec_context_t *ctx, deck_t *deck, errors_list_t *errors, outputs_list_t *outputs);

/* Segment number resolution - called from control.c for tag->segment mapping */
int segment_number(nec_context_t *ctx, int tag, int m);

#endif /* GEOMETRY_H */
