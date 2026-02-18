
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
 * - Implementing context initialization and cleanup routines for nec_context_t,
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

nec_context_t* nec_create_context(void)
{
    nec_context_t *ctx = (nec_context_t*)calloc(1, sizeof(nec_context_t));
    if (ctx) {
        nec_context_init(ctx);
    }
    return ctx;
}

void nec_destroy_context(nec_context_t *ctx)
{
    if (ctx) {
        nec_context_cleanup(ctx);
        free(ctx);
    }
}

void nec_set_log_callback(nec_context_t *ctx, nec_log_callback_t callback, void *user_data)
{
    if (ctx) {
        ctx->log_callback = callback;
        ctx->log_user_data = user_data;
    }
}

void nec_context_init(nec_context_t *ctx)
{
    memset(ctx, 0, sizeof(nec_context_t));
    
    // Initialize error list
    ctx->errors.num_errors = 0;
    ctx->errors.errors = NULL;
    
    // Initialize output message list
    ctx->outputs.num_messages = 0;
    ctx->outputs.messages = NULL;
    
    // Initialize default values (matching original NEC2 initialization)
    ctx->gnd.ksymp = 1;  // Default to free space
    ctx->gnd.ifar = -1;
    ctx->fpat.near = -1;  // -1 = sentinel "no NE/NH card" (0 is a valid near-field mode)
    ctx->gnd.zrati = CPLX_10;
    ctx->save.fmhz = CVEL;
    
    // Start timing for total runtime
    nec_get_time_ms(ctx, &ctx->start_time);
    
    // Initialize ground grid parameters for somnec (from old main.c lines 145-175)
    ctx->ggrid = (ggrid_t){
        .nxa = {11, 17, 9},
        .nya = {10, 5, 8},
        .dxa = {0.02, 0.05, 0.1},
        .dya = {0.1745329252, 0.0872664626, 0.1745329252},
        .xsa = {0.0, 0.2, 0.2},
        .ysa = {0.0, 0.0, 0.3490658504}
    };
    
    // Allocate ggrid arrays for SOMNEC ground calculations
    size_t mreq;
    mreq = sizeof(complex double) * ctx->ggrid.nxa[0] * ctx->ggrid.nya[0] * 4;
    ctx->ggrid.ar1 = malloc(mreq);
    mreq = sizeof(complex double) * ctx->ggrid.nxa[1] * ctx->ggrid.nya[1] * 4;
    ctx->ggrid.ar2 = malloc(mreq);
    mreq = sizeof(complex double) * ctx->ggrid.nxa[2] * ctx->ggrid.nya[2] * 4;
    ctx->ggrid.ar3 = malloc(mreq);

    // Initialize interpolation state for thread-safety
    ctx->intrp = (intrp_t){
        .ixs = -10,
        .iys = -10,
        .igrs = -10,
        .dx = 1.0,
        .dy = 1.0
    };
}

void nec_context_cleanup(nec_context_t *ctx)
{
    // Free ggrid arrays
    if (ctx->ggrid.ar1 != NULL) {
        free(ctx->ggrid.ar1);
        ctx->ggrid.ar1 = NULL;
    }
    if (ctx->ggrid.ar2 != NULL) {
        free(ctx->ggrid.ar2);
        ctx->ggrid.ar2 = NULL;
    }
    if (ctx->ggrid.ar3 != NULL) {
        free(ctx->ggrid.ar3);
        ctx->ggrid.ar3 = NULL;
    }
    
    // Note: File pointers are managed by the caller (main.c) and should not be closed here
    // They are opened in main.c and should be closed there after this cleanup
    
    // Free radiation pattern data
    if (ctx->rpat.points != NULL) {
        free(ctx->rpat.points);
        ctx->rpat.points = NULL;
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
}

/* end of types.c */
