/*
 * roundtrip_test.c — Read → parse → write round-trip test for OpenNEC decks.
 *
 * For each input file given on the command line, this program:
 *   1. Reads the deck with read_deck()
 *   2. Parses it with parse_deck() / initialize_symbol_table() / update_deck_values()
 *   3. Writes it back out with write_deck_onec() to a sibling file with a
 *      .onec extension (same directory, same basename, extension replaced).
 *
 * No geometry calculations or simulation are performed.  The goal is to
 * verify that the read → parse → write pipeline preserves deck content.
 * Minor whitespace and canonical-formatting differences are expected and OK.
 *
 * Usage:
 *   ./roundtrip_test file.nec [file2.nec ...]
 *   ./roundtrip_test test/example3.nec test/TEST299.nec
 *
 * After running, optionally compare with:
 *   diff test/example3.nec test/example3.onec
 *
 * Build:
 *   make roundtrip_test
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "internals.h"

/* ---- null log callback — suppress nec_report() noise during parse ---- */
static void null_log(void *ud, int level, const char *msg)
{
    (void)ud; (void)level; (void)msg;
}

/* ---- replace extension with .onec (writes into caller-supplied buffer) ---- */
static void make_onec_path(const char *inpath, char *out, size_t outsize)
{
    strncpy(out, inpath, outsize - 1);
    out[outsize - 1] = '\0';

    char *dot   = strrchr(out, '.');
    char *slash = strrchr(out, '/');
    /* Only strip the dot if it belongs to the filename, not a directory component */
    if (dot && (!slash || dot > slash))
        *dot = '\0';

    strncat(out, ".onec", outsize - strlen(out) - 1);
}

/* ---- round-trip one file; returns 0 on success, -1 on error ---------- */
static int roundtrip_file(const char *inpath)
{
    char outpath[1024];
    make_onec_path(inpath, outpath, sizeof(outpath));

    /* --- create context --- */
    nec_context_t *ctx = nec_create_context();
    if (!ctx) {
        fprintf(stderr, "  ERROR: could not allocate NEC context\n");
        return -1;
    }
    nec_set_log_callback(ctx, null_log, NULL);
    ctx->source_filename = (char *)inpath;

    /* --- read --- */
    FILE *ifp = fopen(inpath, "r");
    if (!ifp) {
        perror(inpath);
        nec_destroy_context(ctx);
        return -1;
    }
    deck_t deck = {0};
    read_deck(ctx, &deck, ifp);
    fclose(ifp);

    /* --- parse (no simulation) --- */
    errors_list_t errs = {0};
    parse_deck(ctx, &deck, &errs);
    initialize_symbol_table(&deck, &errs);
    update_deck_values(ctx, &deck);

    int parse_warnings = errs.num_errors;
    for (int i = 0; i < errs.num_errors; i++)
        free(errs.errors[i].message);
    free(errs.errors);

    /* --- write --- */
    FILE *ofp = fopen(outpath, "w");
    if (!ofp) {
        perror(outpath);
        free_deck(&deck);
        nec_destroy_context(ctx);
        return -1;
    }
    write_deck_onec(ctx, &deck, ofp);
    fclose(ofp);

    /* --- report --- */
    printf("  -> %-40s", outpath);
    if (parse_warnings > 0)
        printf("  [%d parse warning(s)]", parse_warnings);
    printf("\n");

    free_deck(&deck);
    nec_destroy_context(ctx);
    return 0;
}

/* ---- main ---------------------------------------------------------------- */
int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file> [file2 ...]\n", argv[0]);
        fprintf(stderr, "  Reads each deck, parses it, and writes it back as <file>.onec\n");
        fprintf(stderr, "  No simulation is run — this tests the read/parse/write pipeline only.\n");
        return 1;
    }

    int ok = 0, fail = 0;
    printf("OpenNEC round-trip test\n");
    printf("=======================\n");

    for (int i = 1; i < argc; i++) {
        printf("%s\n", argv[i]);
        int rc = roundtrip_file(argv[i]);
        if (rc == 0)
            ok++;
        else {
            printf("  FAILED\n");
            fail++;
        }
    }

    printf("\n%d OK, %d FAILED\n", ok, fail);
    return fail > 0 ? 1 : 0;
}
