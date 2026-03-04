#include <stdio.h>
#include "maa-support.h"
#include "deck.h"
#include "misc.h"

int main(void) {
    deck_t deck = {0};

    /* Write a minimal Variant B .maa file to test both import and export */
    FILE *tmp = fopen("/tmp/mini.maa", "w");
    fprintf(tmp,
        "Test antenna\n"
        "144.28\n"
        "***Wires***\n"
        "1\n"
        "0.0, 0.0, 0.0, 0.52, 0.0, 0.0, 0.001, 5\n"
        "***Source***\n"
        "1, 0\n"
        "w1c, 0.00, 1.000000\n"
        "***Load***\n"
        "0, 0\n"
    );
    fclose(tmp);

    tmp = fopen("/tmp/mini.maa", "r");
    int rc = read_deck_maa(&deck, tmp);
    fclose(tmp);

    printf("read rc=%d, cards=%d\n\n", rc, deck.num_cards);
    for (int i = 0; i < deck.num_cards; i++)
        printf("  [%s] %s\n", deck.cards[i].card_code,
               deck.cards[i].card_str ? deck.cards[i].card_str : "(null)");

    printf("\n--- write_deck_maa output (should be Variant B) ---\n");
    write_deck_maa(&deck, stdout);
    return 0;
}
