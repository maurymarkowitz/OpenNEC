
/**
 * @file opennec.h
 * @brief Main entry point for the OpenNEC library.
 *
 * OpenNEC is a modernized C implementation of the Numerical Electromagnetics Code (NEC2).
 * 
 * This header aggregates the public API components including context management,
 * deck parsing, calculation engines, and output generation.
 */

#ifndef	OPENNEC_H
#define	OPENNEC_H 1

/** @brief OpenNEC version string */
#define VERSION_STRING "1.1"

#include <math.h>
#include <complex.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <stdbool.h>

/* Public API headers */
#include "types.h"
#include "misc.h"
#include "deck.h"
#include "input.h"
#include "output.h"
#include "geometry.h"
#include "control.h"
#include "fields.h"
#include "ground.h"
#include "calculations.h"
#include "matrix.h"
#include "network.h"
#include "radiation.h"
#include "somnec.h"
#include "tests.h"

/** @name Public Constants
 *  Limits and sizes for various internal buffers.
 *  @{
 */
#define	MAX_LINE_LEN 255   /**< Maximum length of an input line */
#define MAX_PATH_LEN 255   /**< Maximum length of a file path */
#define MAX_ERROR_LEN 255  /**< Maximum length of an error message string */
#define MAX_UNIT_LEN 5     /**< Maximum length of a unit suffix (e.g., "mm") */
/** @} */

/** @name Complex Number Helpers
 *  Macros and constants for working with complex double values.
 *  @{
 */
#define	CPLX_00	(0.0+I*0.0) /**< Complex zero (0+0j) */
#define	CPLX_01	(0.0+I*1.0) /**< Pure imaginary unit (0+1j) */
#define	CPLX_10	(1.0+I*0.0) /**< Real unit (1+0j) */
#define	CPLX_11	(1.0+I*1.0) /**< Complex unit (1+1j) */

/** @brief Helper macro to construct a complex double from real and imaginary parts */
#define cmplx(r, i) ((r)+I*(i))
/** @} */

/** @name Physical Constants
 *  High-precision base and derived constants.
 *  @{
 */
#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288
#endif

#define PI      M_PI            /**< Pi */
#define TP      (2.0 * M_PI)    /**< Two Pi ($2\pi$) */
#define TA      (M_PI / 180.0)  /**< Degrees to Radians conversion factor */
#define TD      (180.0 / M_PI)  /**< Radians to Degrees conversion factor */
#define CVEL    299.792458      /**< Speed of light in $m/\mu s$ (exact) */
/** @} */

#endif
