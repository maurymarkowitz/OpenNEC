#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mma-support.h"
#include "deck.h"
#include "output.h"
#include "misc.h"

/* write_deck_nec is defined in output.c but not declared publicly */
extern void write_deck_nec(const nec_context_t *ctx, const deck_t *deck,
                           FILE *file, int remove_inline_comments);

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
    /* write deck in NEC format (no context needed) */
    write_deck_nec(NULL, &deck, outf, 0);
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
