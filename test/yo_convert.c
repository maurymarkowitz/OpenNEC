/*
 * yo_convert.c  —  smoke-test / command-line converter for Yagi Optimizer files
 *
 * Usage:
 *   ./yo_convert <file.yo> [<file2.yo> ...]
 *
 * For each input file, reads the YO-format antenna description and writes
 * the resulting NEC cards to <file>.nec in the same directory.  Each output
 * card is taken directly from the card_str field of the generated deck.
 *
 * With no arguments the tool falls back to a hard-coded example path so it can
 * be run quickly during development.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "import-export/yo-support.h"
#include "deck.h"
#include "misc.h"

static int convert_file(const char *inpath)
{
    deck_t deck; init_deck(&deck);
    FILE *inf = fopen(inpath, "r");
    if (!inf) {
        perror(inpath);
        return -1;
    }
    if (read_deck_yo(&deck, inf) != 0) {
        fprintf(stderr, "failed to parse %s\n", inpath);
        fclose(inf);
        return -1;
    }
    fclose(inf);

    /* build output name by replacing extension with .nec */
    char outpath[1024];
    strncpy(outpath, inpath, sizeof(outpath) - 1);
    outpath[sizeof(outpath) - 1] = '\0';
    char *dot = strrchr(outpath, '.');
    if (dot)
        strcpy(dot, ".nec");
    else
        strncat(outpath, ".nec", sizeof(outpath) - strlen(outpath) - 1);

    FILE *outf = fopen(outpath, "w");
    if (!outf) {
        perror(outpath);
        return -1;
    }

    /* write deck: use card_str (built verbatim by read_deck_yo) */
    for (int i = 0; i < deck.num_cards; i++) {
        const char *s = deck.cards[i].card_str;
        if (s && *s) {
            fputs(s, outf);
            fputc('\n', outf);
        }
    }
    fclose(outf);
    printf("converted %s -> %s  (%d cards)\n", inpath, outpath, deck.num_cards);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: yo_convert <file.yo> [<file2.yo> ...]\n");
        return 1;
    }
    int rc = 0;
    for (int i = 1; i < argc; i++) {
        if (convert_file(argv[i]) != 0)
            rc = 1;
    }
    return rc;
}
