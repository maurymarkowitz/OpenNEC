#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mma-support.h"
#include "deck.h"
#include "misc.h"

static int convert_file(const char *inpath)
{
    deck_t deck = {0};
    FILE *inf = fopen(inpath, "r");
    if (!inf) {
        perror(inpath);
        return -1;
    }
    if (read_deck_maa(&deck, inf) != 0) {
        fprintf(stderr, "failed to parse %s\n", inpath);
        fclose(inf);
        return -1;
    }
    fclose(inf);

    /* build output name by replacing extension with .nec */
    char outpath[1024];
    strncpy(outpath, inpath, sizeof(outpath));
    char *dot = strrchr(outpath, '.');
    if (dot) {
        strcpy(dot, ".nec");
    } else {
        strncat(outpath, ".nec", sizeof(outpath)-strlen(outpath)-1);
    }

    FILE *outf = fopen(outpath, "w");
    if (!outf) {
        perror(outpath);
        return -1;
    }
    /* write deck in NEC format: use card_str directly since cards were built
       by read_deck_maa using append_card_from_text (int/float fields unparsed) */
    for (int i = 0; i < deck.num_cards; i++) {
        const char *s = deck.cards[i].card_str;
        if (s && *s) {
            fputs(s, outf);
            fputc('\n', outf);
        }
    }
    fclose(outf);
    printf("converted %s -> %s\n", inpath, outpath);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        /* default example file */
        const char *sample = "/Volumes/Bigger/Users/maury/Downloads/AntennaFiles-OLD-master/Broadband 80m.5.maa";
        return convert_file(sample);
    }
    int rc = 0;
    for (int i = 1; i < argc; i++) {
        if (convert_file(argv[i]) != 0)
            rc = 1;
    }
    return rc;
}
