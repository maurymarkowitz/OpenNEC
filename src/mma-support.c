/******************************************************************************
 * mma-support.c
 *
 * Functions to convert between OpenNEC deck structures and the
 * MMANA-GAL ".maa" file format.  The format is a simple, line-oriented
 * representation used by MMANA-GAL and compatible tools.  Exporting writes a
 * minimal subset of the deck (wires, loads, sources, frequency) to the
 * provided file pointer.  Importing parses a .maa file and appends the
 * corresponding GW/LD/EX/FR cards to an existing deck.
 *
 * These utilities are deliberately lightweight and do not attempt to
 * perfectly reproduce every MMANA feature; they mirror the behavior of the
 * earlier Python converters located in the AntennaSim project.
 *
 *****************************************************************************/

#include "mma-support.h"
#include "deck.h"    // for insert_card
#include "misc.h"    // for add_error
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>

/**
 * @brief Create a new card from a raw text line and append to deck.
 *
 * This helper allocates and populates a temporary card_t with the provided
 * string, then uses insert_card() to place it at the end of the deck.  It
 * is used by both the importer and exporter to materialize NEC cards.
 *
 * @param deck Target deck (must be non-NULL).
 * @param text Null-terminated string containing the entire card line.
 * @return 0 on success, -1 on allocation or insertion failure.
 */
static int append_card_from_text(deck_t *deck, const char *text)
{
    card_t card = {0};
    size_t len = strlen(text);
    card.edited = false;
    card.ignore = false;
    card.card_num = deck->num_cards + 1;
    card.orig_str = calloc(len + 1, 1);
    card.card_str = calloc(len + 1, 1);
    if (!card.orig_str || !card.card_str) {
        free(card.orig_str);
        free(card.card_str);
        return -1;
    }
    memcpy(card.orig_str, text, len);
    memcpy(card.card_str, text, len);
    /* mnemonic is first two non-space chars */
    const char *p = text;
    while (*p && isspace((unsigned char)*p)) p++;
    card.card_code[0] = *p ? *p : '\0';
    card.card_code[1] = *p && *(p+1) ? *(p+1) : '\0';
    card.card_code[2] = '\0';

    if (insert_card(deck, &card, deck->num_cards) < 0) {
        return -1;
    }
    return 0;
}

/**
 * @copydoc write_deck_maa
 */
int write_deck_maa(const deck_t *deck, FILE *fp)
{
    if (!deck || !fp) return -1;

    /* title */
    fprintf(fp, "OpenNEC export\n");

    /* frequency: look for FR card first */
    double freq = 14.0; /* default */
    for (int i = 0; i < deck->num_cards; i++) {
        card_t *c = &deck->cards[i];
        if (strcmp(c->card_code, "FR") == 0) {
            freq = c->f[1];
            break;
        }
    }
    fprintf(fp, "%.6f\n", freq);

    /* count wires, loads, sources */
    int nw = 0, nl = 0, ns = 0;
    for (int i = 0; i < deck->num_cards; i++) {
        card_t *c = &deck->cards[i];
        if (strcmp(c->card_code, "GW") == 0) nw++;
        else if (strcmp(c->card_code, "LD") == 0) nl++;
        else if (strcmp(c->card_code, "EX") == 0) ns++;
    }
    fprintf(fp, "%d %d %d\n", nw, nl, ns);

    /* wires */
    for (int i = 0, count = 0; i < deck->num_cards && count < nw; i++) {
        card_t *c = &deck->cards[i];
        if (strcmp(c->card_code, "GW") != 0) continue;
        count++;
        /* NEC: GW tag, segments, x1,y1,z1,x2,y2,z2,radius
           fields are i[1], i[2], f1..f7; some decks use commas */
        double x1 = c->f[1];
        double y1 = c->f[2];
        double z1 = c->f[3];
        double x2 = c->f[4];
        double y2 = c->f[5];
        double z2 = c->f[6];
        double rad = c->f[7];
        int segs = c->i[2];
        fprintf(fp, "%.6f, %.6f, %.6f, %.6f, %.6f, %.6f, %.6f, %d\n",
                x1, y1, z1, x2, y2, z2, rad, segs);
    }

    /* loads */
    for (int i = 0, count = 0; i < deck->num_cards && count < nl; i++) {
        card_t *c = &deck->cards[i];
        if (strcmp(c->card_code, "LD") != 0) continue;
        count++;
        int wire = c->i[2];
        int seg  = c->i[3];
        double R = c->f[1];
        double X = c->f[2];
        double L = c->f[3];
        double C = c->f[4];
        fprintf(fp, "%d, %d, %.6g, %.6g, %.6g, %.6g\n",
                wire, seg, R, X, L, C);
    }

    /* sources */
    for (int i = 0, count = 0; i < deck->num_cards && count < ns; i++) {
        card_t *c = &deck->cards[i];
        if (strcmp(c->card_code, "EX") != 0) continue;
        count++;
        int wire = c->i[2];
        int seg  = c->i[3];
        double re = c->f[1];
        double im = c->f[2];
        double mag = hypot(re, im);
        double phase = atan2(im, re) * 180.0 / M_PI;
        fprintf(fp, "%d, %d, %.6f, %.2f\n", wire, seg, mag, phase);
    }

    /* ground/other info: simply write defaults */
    fprintf(fp, "1\n");          /* real ground */
    fprintf(fp, "13.0, 0.005\n"); /* average parameters */
    fprintf(fp, "\n");
    return 0;
}

/**
 * @copydoc read_deck_maa
 */
int read_deck_maa(deck_t *deck, FILE *fp)
{
    if (!deck || !fp) return -1;
    char line[512];
    int n_wires = 0, n_loads = 0, n_src = 0;
    double freq = 0.0;

    /* read title */
    if (!fgets(line, sizeof line, fp)) return -1;
    /* next line maybe freq */
    if (fgets(line, sizeof line, fp)) {
        freq = atof(line);
    }
    /* counts */
    if (fgets(line, sizeof line, fp)) {
        sscanf(line, "%d %d %d", &n_wires, &n_loads, &n_src);
    }

    /* append an FR card for frequency */
    if (freq > 0) {
        char buf[128];
        snprintf(buf, sizeof buf, "FR 0,0,%.6f,0,0,0,0,0", freq);
        append_card_from_text(deck, buf);
    }

    /* wires */
    for (int i = 0; i < n_wires; i++) {
        if (!fgets(line, sizeof line, fp)) break;
        double x1,y1,z1,x2,y2,z2,rad; int segs;
        if (sscanf(line, "%lf, %lf, %lf, %lf, %lf, %lf, %lf, %d",
                   &x1,&y1,&z1,&x2,&y2,&z2,&rad,&segs) >= 8) {
            char buf[256];
            /* tag is 1-based order */
            snprintf(buf, sizeof buf,
                     "GW %d, %d, %.6f, %.6f, %.6f, %.6f, %.6f, %.6f, %.6f",
                     i+1, segs, x1,y1,z1,x2,y2,z2,rad);
            append_card_from_text(deck, buf);
        }
    }

    /* loads */
    for (int i = 0; i < n_loads; i++) {
        if (!fgets(line, sizeof line, fp)) break;
        int wire, seg;
        double R,X,L,C;
        char *p = line;
        /* allow commas or spaces */
        sscanf(p, "%d %d %lf %lf %lf %lf", &wire,&seg,&R,&X,&L,&C);
        char buf[256];
        /* simple mapping to LD card; use R,X,L,C in f1..f4 */
        snprintf(buf, sizeof buf,
                 "LD 0, %d, %d, %.6g, %.6g, %.6g, %.6g",
                 wire, seg, R, X, L, C);
        append_card_from_text(deck, buf);
    }

    /* sources */
    for (int i = 0; i < n_src; i++) {
        if (!fgets(line, sizeof line, fp)) break;
        int wire, seg;
        double mag, phase;
        sscanf(line, "%d %d %lf %lf", &wire,&seg,&mag,&phase);
        double phrad = phase * M_PI / 180.0;
        double re = mag * cos(phrad);
        double im = mag * sin(phrad);
        char buf[256];
        snprintf(buf, sizeof buf,
                 "EX 0, %d, %d, %.6f, %.6f, 0,0,0",
                 wire, seg, re, im);
        append_card_from_text(deck, buf);
    }

    /* ignore remaining ground/freq lines */
    return 0;
}
