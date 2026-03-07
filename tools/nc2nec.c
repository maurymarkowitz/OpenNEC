#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "import-export/nc-support.h"
#include "input.h"
#include "output.h"
#include "deck.h"
#include <ctype.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s input.nc output.nec\n", argv[0]);
        return 2;
    }
    const char *inpath = argv[1];
    const char *outpath = argv[2];

    nec_context_t *ctx = nec_create_context();
    if (!ctx) {
        fprintf(stderr, "nc2nec: failed to create context\n");
        return 1;
    }

    deck_t deck = {0};
    errors_list_t import_errors = {0};

    FILE *inf = fopen(inpath, "r");
    if (!inf) {
        perror("open input");
        nec_destroy_context(ctx);
        return 1;
    }

    if (read_deck_nc(&deck, inf) != 0) {
        fprintf(stderr, "nc2nec: read_deck_nc failed for %s\n", inpath);
        fclose(inf);
        nec_destroy_context(ctx);
        return 1;
    }
    fclose(inf);

    /* parse and evaluate so we have a proper deck state */
    parse_deck(ctx, &deck, &import_errors);
    initialize_symbol_table(&deck, &import_errors);
    update_deck_values(ctx, &deck);

    /* Write ONEC format directly to the output path (no temp file, no injection).
     * write_deck_onec already emits SY cards in the correct positions:
     *   - SY declarations that precede the first geometry card stay before it
     *   - SY assignments interleaved with geometry stay interleaved
     * Overwrite any existing file without creating backups.
     */
    unlink(outpath); /* remove any prior output, ignore errors */
    FILE *outf = fopen(outpath, "w");
    if (!outf) {
        perror("open output");
        free_deck(&deck);
        nec_destroy_context(ctx);
        return 1;
    }
    write_deck_onec(ctx, &deck, outf);
    fclose(outf);

    free_deck(&deck);
    nec_destroy_context(ctx);
    return 0;
}
