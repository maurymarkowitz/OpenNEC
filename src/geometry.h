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

/*---------------------------------------------------------------------------
 * Auto-segmentation (MMANA-GAL tapering method)
 *
 * These utilities implement the MMANA-GAL tapering segmentation method
 * (described in the "Segmentation" section of the MMANA-GAL help).  They
 * are called by the .maa importer (maa-support.c) and may also be called
 * directly by GUI applications that need to know how many NEC segments to
 * assign to a wire, or need to split a wire into equal-segment sub-wires
 * that together reproduce the taper profile.
 *
 * Taper parameters (stored in the ***Segmentation*** block of a .maa file):
 *   DM1  Fineness of the finest (end) segments: each = lambda/DM1.
 *        Typical value: 200.  Must be > DM2.
 *   DM2  Fineness of the coarsest (middle) segments: each = lambda/DM2.
 *        Typical value: 20.
 *   SC   Growth factor: each step away from the wire end is SC times longer
 *        than the previous step.  1 < SC <= 3; typical value: 2.0.
 *   EC   Count of the finest segments placed flat at each wire end before
 *        the geometric growth begins.  Typical value: 1.
 *
 * MMANA accuracy limits (from the help document):
 *   - Segments shorter than 0.001 lambda produce unreliable results.
 *   - Segment length-to-wire-radius ratio should not exceed 4:1.
 *---------------------------------------------------------------------------*/

/**
 * Segmentation mode passed to compute_segmentation().
 *
 * These correspond to the SEG column in the MMANA-GAL wire table.  Any
 * positive integer may also be passed directly as a manual segment count;
 * the enum names cover only the special automatic values.
 */
typedef enum {
    SEG_MODE_TAPER_END   = -3, /**< Taper at end (far) end only.                */
    SEG_MODE_TAPER_START = -2, /**< Taper at start (near) end only.             */
    SEG_MODE_TAPER_BOTH  = -1, /**< Taper at both ends (recommended default).   */
    SEG_MODE_UNIFORM     =  0, /**< Uniform coarse segmentation (lambda/DM2).   */
    /* > 0 : manual — use that exact count, all other params ignored */
} seg_mode_t;

/** Maximum number of uniform-segment groups returned in seg_plan_t. */
#define MAX_SEG_GROUPS 64

/**
 * One contiguous span of equal-length segments within a single wire.
 *
 * A NEC GW card requires all segments on a wire to be the same length, so
 * each group maps to exactly one GW card when a wire is sub-divided.
 *
 * @param segs  Number of equal-length segments in this span.
 * @param frac  Fraction of the total wire length covered by this span
 *              (0 < frac <= 1; all fracs in a plan sum to 1).
 */
typedef struct {
    int    segs;
    double frac;
} seg_group_t;

/**
 * Full segmentation plan for one wire.
 *
 * When compute_segmentation() is asked to fill this structure callers can
 * either use @p total_segs directly (as the segment count for a single
 * uniform GW card — an approximation) or iterate over @p groups to build
 * multiple GW sub-wires that accurately reproduce the taper profile.
 */
typedef struct {
    int         total_segs;               /**< Total segments across all groups.   */
    int         n_groups;                 /**< Number of valid entries in groups[]. */
    seg_group_t groups[MAX_SEG_GROUPS];   /**< Per-sub-wire plan; see above.        */
} seg_plan_t;

/**
 * Compute the MMANA-GAL tapering segmentation for a single wire.
 *
 * All length parameters must be in the same unit (metres is conventional).
 * The function is pure math and requires no NEC context.
 *
 * @param wire_len   Physical length of the wire (> 0).
 * @param wavelength Wavelength at the operating frequency (> 0).
 * @param seg_mode   Segmentation mode: use a seg_mode_t constant, or any
 *                   positive integer for a manual count.
 * @param dm1        Finest-segment divisor (λ/DM1 = end-segment length).
 *                   Clamped to a minimum that keeps segments >= 0.001 λ.
 * @param dm2        Coarsest-segment divisor (λ/DM2 = middle-segment length).
 * @param sc         Geometric growth factor between adjacent taper steps (> 1).
 * @param ec         Number of finest segments at each tapered wire end (>= 1).
 * @param plan       If non-NULL, filled with the per-sub-wire breakdown.
 *                   Pass NULL when only the total count is needed.
 * @return Total segment count (>= 1), or -1 if wire_len or wavelength <= 0.
 */
int compute_segmentation(double wire_len, double wavelength,
                         int seg_mode,
                         int dm1, int dm2, double sc, int ec,
                         seg_plan_t *plan);

#endif /* GEOMETRY_H */
