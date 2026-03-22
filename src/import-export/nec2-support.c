/**
 * @file nec2-support.c
 *
 * Minimal placeholder implementations for the routines declared in
 * `nec2-support.h`.  The module currently provides thin wrappers around the
 * core parser/writer in `deck.c` but exists so that any NEC-2–specific
 * helpers can be added without polluting the general parser.
 */

#include "nec2-support.h"
#include "deck.h"    /* for the real implementations and card structures */
#include "input.h"   /* for read_deck declaration */
#include <stdio.h>
#include <string.h>   /* for strcmp, snprintf, etc. */

/**
 * @copydoc read_deck_nec2
 */
int read_deck_nec2(deck_t *deck, FILE *fp)
{
    if (!deck || !fp) return -1;
    nec_context_t *ctx = nec_create_context();
    if (!ctx) return -1;
    read_deck(ctx, deck, fp);
    nec_destroy_context(ctx);
    return 0;
}

/**
 * @copydoc write_deck_nec2
 */
int write_deck_nec2(const deck_t *deck, FILE *file)
{
    if (!deck || !file) return -1;
    card_t *card;
    int MAX_INTS, MAX_FLTS;

    for (int i = 0; i < deck->num_cards; i++)
    {
        card = &deck->cards[i];

        // if we are past the end of the deck, just write out the whole string
        if (i > deck->deck_end)
        {
            fputs(card->card_str, file);
            fputc('\n', file);
            continue;
        }

        // skip extension cards in pure NEC files
        if (is_extension(card))
            continue;

        // for comment cards with the CM or CE *in the header*, simply export the card
        if (i <= deck->geometry_start &&
            (strcmp(card->card_code, "CM") == 0 ||
             strcmp(card->card_code, "CE") == 0))
        {
            fprintf(file, "%s%s", deck->cards[i].card_code,
                    deck->cards[i].comment);
            fputc('\n', file);
        }
        // for comment cards with other headers, only export if the option is on
        if (is_comment(card))
        {
            fprintf(file, "%s%s", deck->cards[i].card_code,
                    deck->cards[i].comment);
            fputc('\n', file);
        }

        // for geometry and command cards, start with the code
        fputs(card->card_code, file);

        // get the number of fields for this sort of card
        MAX_INTS = max_int_fields(card);
        MAX_FLTS = max_flt_fields(card);

        // int and float fields
        if (is_control(card) || is_geometry(card))
        {
            for (int j = 0; j <= card->ints_used && j <= MAX_INTS; j++)
            {
                char key[8];
                snprintf(key, sizeof(key), "I%d", j);
                const char *formula = lookup_formula(card, key);

                if (formula != NULL)
                {
                    fprintf(file, " %s", formula);
                }
                else
                {
                    fprintf(file, " %d", card->i[j]);
                }
            }
            for (int j = 0; j <= card->flts_used && j <= MAX_FLTS; j++)
            {
                char key[8];
                snprintf(key, sizeof(key), "F%d", j);
                const char *formula = lookup_formula(card, key);

                if (formula != NULL)
                {
                    fprintf(file, " %s", formula);
                }
                else
                {
                    fprintf(file, " %G", card->f[j]);
                }
            }
            // close the line
            fputc('\n', file);
        } /* if command or geometry */
    } /* for over cards */
    return 0;
}
