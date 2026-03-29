/* Simple test: verify that read_deck_maa inserts an SY helper and
 * replaces -1 with the textual token 'segs' in GW card strings.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mma-support.h"
#include "types.h"

int main(void)
{
    const char *tmpfile = "/tmp/maa_sy_test_input.maa";
    FILE *fp = fopen(tmpfile, "w");
    if (!fp) {
        perror("fopen");
        return 2;
    }
    /* Minimal .maa content: title, freq, counts, single wire with -1, empty source/load */
    fprintf(fp, "Test SY insertion\n");
    fprintf(fp, "14.000000\n");
    fprintf(fp, "1 0 0\n");
    fprintf(fp, "0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.001, -1\n");
    fprintf(fp, "***Source***\n0, 0\n");
    fprintf(fp, "***Load***\n0, 0\n");
    fprintf(fp, "***Segmentation***\n800, 80, 2.0, 2\n");
    fclose(fp);

    fp = fopen(tmpfile, "r");
    if (!fp) { perror("fopen r"); return 2; }

    deck_t deck; init_deck(&deck);
    int rc = read_deck_maa(&deck, fp);
    fclose(fp);
    if (rc != 0) {
        fprintf(stderr, "read_deck_maa failed: %d\n", rc);
        return 3;
    }

    int found_sy = 0;
    int found_segs = 0;
    for (int i = 0; i < deck.num_cards; i++) {
        card_t *c = &deck.cards[i];
        if (strcmp(c->card_code, "SY") == 0) found_sy = 1;
        if (c->card_str && strstr(c->card_str, "segs")) found_segs = 1;
    }

    if (found_sy && found_segs) {
        printf("PASS: SY inserted and 'segs' token present in GW lines\n");
        return 0;
    }
    if (!found_sy) fprintf(stderr, "FAIL: SY not found\n");
    if (!found_segs) fprintf(stderr, "FAIL: 'segs' token not found in GW lines\n");
    return 1;
}
