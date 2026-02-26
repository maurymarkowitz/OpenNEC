
#include "internals.h"
#include "geometry.h"
#include <stdio.h>

static void null_log(void *ud, int level, const char *msg) { (void)ud;(void)level;(void)msg; }

int main(int argc, char *argv[]) {
    const char *path = (argc > 1) ? argv[1] : "test/4nec2 examples/Example3.nec";
    nec_context_t *ctx = nec_create_context();
    nec_set_log_callback(ctx, null_log, NULL);

    deck_t deck; memset(&deck, 0, sizeof(deck));
    FILE *f = fopen(path, "r");
    if (!f) { fprintf(stderr, "Cannot open %s\n", path); return 1; }
    read_deck(ctx, &deck, f);
    fclose(f);

    errors_list_t errs = {0};
    parse_deck(ctx, &deck, &errs);
    initialize_symbol_table(&deck, &errs);
    update_deck_values(ctx, &deck);

    printf("num_cards     = %d\n", deck.num_cards);
    printf("comment_start = %d\n", deck.comment_start);
    printf("comment_end   = %d\n", deck.comment_end);
    printf("symbol_start  = %d\n", deck.symbol_start);
    printf("symbol_end    = %d\n", deck.symbol_end);
    printf("geometry_start= %d\n", deck.geometry_start);
    printf("geometry_end  = %d\n", deck.geometry_end);
    printf("deck_end      = %d\n", deck.deck_end);
    printf("\n");
    printf("idx  marker  code  ign  cmt   raw\n");
    printf("---  ------  ----  ---  ---   ---\n");

    const char *marker_names[] = {
        "comment_start", "comment_end", "symbol_start", "symbol_end",
        "geometry_start", "geometry_end", "deck_end"
    };
    int marker_vals[] = {
        deck.comment_start, deck.comment_end, deck.symbol_start, deck.symbol_end,
        deck.geometry_start, deck.geometry_end, deck.deck_end
    };
    int n_markers = 7;

    for (int i = 0; i < deck.num_cards; i++) {
        card_t *c = &deck.cards[i];
        const char *marker = "";
        for (int m = 0; m < n_markers; m++) {
            if (marker_vals[m] == i) { marker = marker_names[m]; break; }
        }
        char raw[61] = "";
        if (c->orig_str) { strncpy(raw, c->orig_str, 60); raw[60] = 0; }
        /* strip trailing newline for display */
        int len = strlen(raw);
        while (len > 0 && (raw[len-1] == '\n' || raw[len-1] == '\r')) raw[--len] = 0;
        printf("[%2d]  %-14s  %-4s  %d  '%c'   %s\n",
            i, marker, c->card_code, c->ignore,
            c->cmt_code[0] ? c->cmt_code[0] : ' ', raw);
    }
    for (int i = 0; i < errs.num_errors; i++) free(errs.errors[i].message);
    free(errs.errors);
    free_deck(&deck);
    nec_destroy_context(ctx);
    return 0;
}
