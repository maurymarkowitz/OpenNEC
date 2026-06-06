
/******************************************************************************
 * types.c
 *
 * types.c defines and initializes the global arrays, enums, and lookup tables
 * used throughout OpenNEC for card parsing, unit handling, and symbolic field
 * access. It provides the string-to-index mappings for card mnemonics, field
 * names, control/geometry/extension codes, and measurement units, as well as
 * the corresponding conversion factors and variable name arrays for formula
 * evaluation.
 *
 * Major responsibilities include:
 * - Defining the string arrays for field names (I1..I4, F1..F7), card codes
 *   (comment, control, geometry, extension), and measurement units.
 * - Providing the unit conversion multipliers for SI normalization, including
 *   special handling for feet+inches and AWG wire gauge.
 * - Declaring and initializing the variable name arrays (fnames/inames) for
 *   formula evaluation with tinyexpr, using 1-based indexing.
 * - Implementing context initialization and cleanup routines for context_t,
 *   including allocation of ground grid arrays and error lists.
 *
 * These definitions are shared across the parser, deck, and calculation
 * modules, ensuring consistent interpretation of card fields and units.
 *****************************************************************************/

#include "types.h"
#include "internals.h"
#include "misc.h"

// NOTE: ordering of these lists is important! they are used in
//       various places to convert the code back to a number for
//       a switch statement

char *field_names[NUM_FIELD_NAMES] = {
  "I1", "I2", "I3", "I4", "F1", "F2", "F3", "F4", "F5", "F6", "F7"
};

char *comment_codes[NUM_COMMENT_CODES] = {
  "CM", "CE", "!", "'", "#"
};

char *control_codes[NUM_CONTROL_CODES] = {
  "FR", "LD", "GN", "EX", "NT", "TL", \
  "XQ", "GD", "RP", "NX", "PT", "KH", \
  "NE", "NH", "PQ", "EK", "CP", "PL", \
  "EN", "WG"
};

// this list is from the original nec2c but has been expanded with
// some that nec2c left out. The original code didn't need something
// like GC in this list because it only follows a GW, so instead of
// decoding the GC code into a number and handling it, it triggered the
// decode of the GC right in the GW handler. In onec, reading and
// decoding are separate sections of the system, so we need every
// code in here somewhere.
char *geometry_codes[NUM_GEOMETRY_CODES] = {
  "GW", "GX", "GR", "GS", "GE", "GM", \
  "SP", "SM", "GA", "SC", "GH", "GF", "GC"
};

// this is the list of extensions that onec directly supports
// XT = "eXiT"     - from nec2c
// SY = "SY"mbol   - from 4nec2
// IT = "ITerate"  - new code, runs the output several times after changing SYs by a step
// OP = "OPtimize" - attempts to maximize or minimize a selected output value, like gain
char *onec_codes[NUM_ONEC_CODES] = {
  "XT", "SY", "IT", "OP"
};


/*
 * tinyexpr variable names for NEC field bindings.
 * The first element is a blank string so these arrays are 1-based,
 * matching how field indices are used throughout (F1..F7, I1..I4).
 * Index 0 is intentionally unused.
 */
const char *fnames[MAX_FLT_FIELDS + 1] = {
  "", "F1", "F2", "F3", "F4", "F5", "F6", "F7"
};
const char *inames[MAX_INT_FIELDS + 1] = {
  "", "I1", "I2", "I3", "I4"
};

context_t* create_context(void)
{
    context_t *ctx = (context_t*)calloc(1, sizeof(context_t));
    if (ctx) {
        context_init(ctx);
    }
    return ctx;
}

void destroy_context(context_t *ctx)
{
    if (ctx) {
        context_cleanup(ctx);
        free(ctx);
    }
}

void set_log_callback(context_t *ctx, log_callback_t callback, void *user_data)
{
    if (ctx) {
        ctx->log_callback = callback;
        ctx->log_user_data = user_data;
    }
}

void context_init(context_t *ctx)
{
    memset(ctx, 0, sizeof(context_t));
    
    // Initialize error list
    ctx->errors.num_errors = 0;
    ctx->errors.errors = NULL;
    
    // Initialize output message list
    ctx->outputs.num_messages = 0;
    ctx->outputs.messages = NULL;
    
    // Initialize default values (matching original NEC2 initialization)
    ctx->gnd.has_ground = 1;  // Default to free space
    ctx->gnd.far_field_type = -1;
    ctx->fpat.is_near_field = -1;  // -1 = sentinel "no NE/NH card" (0 is a valid near-field mode)
    ctx->gnd.impedance_ratio = CPLX_10;
    ctx->save.freq_mhz = CVEL;
    ctx->currents_print_control = -2;  /* iptflg — Fortran default: -2 (print currents) */
    
    // Start timing for total runtime
    get_time_ms(ctx, &ctx->start_time);
    
    // Initialize output format (set to default, may be overridden by main.c)
    ctx->output_format = DEFAULT_OUTPUT_FORMAT;
    
    // Initialize ground grid parameters for somnec (from old main.c lines 145-175)
    ctx->ggrid = (green_grid_t){
        .grid_nx = {11, 17, 9},
        .grid_ny = {10, 5, 8},
        .grid_dx = {0.02, 0.05, 0.1},
        .grid_dy = {0.1745329252, 0.0872664626, 0.1745329252},
        .grid_x0 = {0.0, 0.2, 0.2},
        .grid_y0 = {0.0, 0.0, 0.3490658504}
    };
    
    // Allocate ggrid arrays for SOMNEC ground calculations
    size_t mreq;
    mreq = sizeof(complex double) * ctx->ggrid.grid_nx[0] * ctx->ggrid.grid_ny[0] * 4;
    ctx->ggrid.table1 = malloc(mreq);
    mreq = sizeof(complex double) * ctx->ggrid.grid_nx[1] * ctx->ggrid.grid_ny[1] * 4;
    ctx->ggrid.table2 = malloc(mreq);
    mreq = sizeof(complex double) * ctx->ggrid.grid_nx[2] * ctx->ggrid.grid_ny[2] * 4;
    ctx->ggrid.table3 = malloc(mreq);

    // Initialize interpolation state for thread-safety
    ctx->intrp = (intrp_t){
        .ixs = -10,
        .iys = -10,
        .igrs = -10,
        .dx = 1.0,
        .dy = 1.0
    };
}

void context_cleanup(context_t *ctx)
{
    // Free ggrid arrays
    if (ctx->ggrid.table1 != NULL) {
        free(ctx->ggrid.table1);
        ctx->ggrid.table1 = NULL;
    }
    if (ctx->ggrid.table2 != NULL) {
        free(ctx->ggrid.table2);
        ctx->ggrid.table2 = NULL;
    }
    if (ctx->ggrid.table3 != NULL) {
        free(ctx->ggrid.table3);
        ctx->ggrid.table3 = NULL;
    }
    
    // Note: File pointers are managed by the caller (main.c) and should not be closed here
    // They are opened in main.c and should be closed there after this cleanup
    
    // Free radiation pattern data
    if (ctx->rpat.points != NULL) {
        free(ctx->rpat.points);
        ctx->rpat.points = NULL;
    }

    // Free near-field data
    if (ctx->nfr.points != NULL) {
        free(ctx->nfr.points);
        ctx->nfr.points = NULL;
    }
    
    // Free error list
    if (ctx->errors.errors != NULL) {
        for (int i = 0; i < ctx->errors.num_errors; i++) {
            if (ctx->errors.errors[i].message != NULL) {
                free(ctx->errors.errors[i].message);
            }
        }
        free(ctx->errors.errors);
        ctx->errors.errors = NULL;
    }
    ctx->errors.num_errors = 0;

    // Free output message list
    if (ctx->outputs.messages != NULL) {
        for (int i = 0; i < ctx->outputs.num_messages; i++) {
            if (ctx->outputs.messages[i] != NULL) {
                free(ctx->outputs.messages[i]);
            }
        }
        free(ctx->outputs.messages);
        ctx->outputs.messages = NULL;
    }
    ctx->outputs.num_messages = 0;

    // Free CP coupling rows
    if (ctx->yparm.coupling_rows != NULL) {
        free(ctx->yparm.coupling_rows);
        ctx->yparm.coupling_rows = NULL;
    }
    ctx->yparm.num_coupling_rows = 0;
    ctx->yparm.coupling_rows_cap = 0;

    /* Free NGF cached matrix */
    if (ctx->ngf_cm != NULL) {
        free(ctx->ngf_cm);
        ctx->ngf_cm = NULL;
    }
}

/* end of types.c */
