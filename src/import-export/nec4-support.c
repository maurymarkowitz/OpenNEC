/**
 * @file nec4-support.c
 *
 * Minimal support module for NEC-4 input/output.  The implementation mirrors
 * `nec2-support.c`, providing a central place for NEC-4-specific helpers while
 * still relying on the core parser/writer for most work.
 */

#include "nec4-support.h"
#include "input.h"    /* for read_deck */
#include "deck.h"     /* for card helpers: is_comment, is_geometry, etc. */
#include <ctype.h>     /* for isalpha */
#include <string.h>    /* for strcmp, snprintf */
#include <stdio.h>

/**
 * @copydoc read_deck_nec4
 */
int read_deck_nec4(deck_t *deck, FILE *fp)
{
    if (!deck || !fp) return -1;
    context_t *ctx = create_context();
    if (!ctx) return -1;
    read_deck(ctx, deck, fp);
    destroy_context(ctx);
    return 0;
}

/**
 * Remove onec-style key=value pairs from a comment string.
 *
 * The NC/ONEC exporter appends annotations in the form " key=value" to the
 * end of comment fields.  When emitting NEC-4 comments we want to discard
 * these as they are not part of the original deck.  This helper performs a
 * simple scan that skips any whitespace-delimited token containing an '='.
 */
static void strip_onec(const char *src, char *dst, size_t sz)
{
    /* copy src to dst, but drop any " key=value" substrings (simple heuristic) */
    size_t di = 0;
    const char *p = src;
    while (*p && di + 1 < sz) {
        if (*p == ' ' && isalpha((unsigned char)p[1])) {
            // peek ahead for '=' before next space
            const char *q = p + 1;
            while (*q && *q != ' ' && *q != '\t' && *q != '\n') {
                if (*q == '=') { break; }
                q++;
            }
            if (*q == '=') {
                // skip this key=value segment
                p = q;
                while (*p && *p != ' ' && *p != '\t' && *p != '\n') p++;
                continue;
            }
        }
        dst[di++] = *p++;
    }
    dst[di] = '\0';
}

/**
 * @copydoc write_deck_nec4
 */
int write_deck_nec4(const deck_t *deck, FILE *file)
{
    if (!deck || !file) return -1;

    card_t *card;
    int MAX_INTS, MAX_FLTS;

    for (int i = 0; i < deck->num_cards; i++) {
        card = &deck->cards[i];
        // skip SY cards entirely
        if (strcmp(card->card_code, "SY") == 0)
            continue;

        // if we are past the end of the deck, just write out the whole string
        if (i > deck->deck_end) {
            fputs(card->card_str, file);
            fputc('\n', file);
            continue;
        }

        /* handle standalone comment cards; convert all markers to '!'
         * except CM/CE which are left intact. */
        if (is_comment(card)) {
            /* card_code is always non-null (fixed-size array); only test first char */
    if (card->card_code[0] == 'C') {
                fprintf(file, "%s", card->card_code);
                fputs(card->comment, file);
                fputc('\n', file);
            } else {
                char cleaned[512];
                strip_onec(card->comment, cleaned, sizeof cleaned);
                fprintf(file, " !%s\n", cleaned);
            }
            continue;
        }

        // skip extension cards (non-comment)
        if (is_extension(card))
            continue;

        // geometry/control cards start here
        fputs(card->card_code, file);
        MAX_INTS = max_int_fields(card);
        MAX_FLTS = max_flt_fields(card);

        if (is_control(card) || is_geometry(card)) {
            for (int j = 0; j <= card->ints_used && j <= MAX_INTS; j++) {
                char key[8];
                snprintf(key, sizeof key, "I%d", j);
                const char *formula = lookup_formula(card, key);
                if (formula) fprintf(file, " %s", formula);
                else fprintf(file, " %d", card->i[j]);
            }
            for (int j = 0; j <= card->flts_used && j <= MAX_FLTS; j++) {
                char key[8];
                snprintf(key, sizeof key, "F%d", j);
                const char *formula = lookup_formula(card, key);
                if (formula) fprintf(file, " %s", formula);
                else fprintf(file, " %G", card->f[j]);
            }
            // prepare comment text, if any
            char cmt_text[512] = "";
            if (card->comment && *card->comment) {
                strip_onec(card->comment, cmt_text, sizeof cmt_text);
            } else if (card->extn_str && *card->extn_str) {
                strip_onec(card->extn_str, cmt_text, sizeof cmt_text);
            }
            if (cmt_text[0] != '\0') {
                fprintf(file, " !%s", cmt_text);
            }
            fputc('\n', file);
        }
    }
    return 0;
}
