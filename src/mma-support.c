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

    /* title: use first comment card if present */
    const char *title = "OpenNEC export";
    for (int i = 0; i < deck->num_cards; i++) {
        card_t *c = &deck->cards[i];
        if (card_is_commented_out(c) || strcmp(c->card_code, "CM") == 0) {
            if (c->comment && c->comment[0] != '\0')
                title = c->comment;
            else if (c->orig_str)
                title = c->orig_str;
            break;
        }
    }
    fprintf(fp, "%s\n", title);

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

    /* append any comment cards as ###Comment### lines */
    for (int i = 0; i < deck->num_cards; i++) {
        card_t *c = &deck->cards[i];
        if (card_is_commented_out(c) || strcmp(c->card_code, "CM") == 0) {
            const char *txt = c->comment ? c->comment : c->orig_str;
            if (txt && txt[0] != '\0') {
                fprintf(fp, "###Comment### %s\n", txt);
            }
        }
    }

    /* end marker */
    fprintf(fp, "\n");
    return 0;
}

/**
 * @copydoc read_deck_maa
 */
/* helper to read next non-header line from the stream */
static int maa_read_data_line(FILE *fp, char *out)
{
    while (fgets(out, 512, fp)) {
        if (strncmp(out, "***", 3) == 0) continue;
        if (out[0] == '*') continue;
        if (out[0] == '\n') continue;
        return 1;
    }
    return 0;
}

int read_deck_maa(deck_t *deck, FILE *fp)
{
    if (!deck || !fp) return -1;
    char line[512];
    int n_wires = 0;
    double freq = 0.0;

    /* read title (first non-empty, non-asterisk line) */
    while (fgets(line, sizeof line, fp)) {
        if (line[0] == '*' || line[0] == '\n') continue;
        break;
    }
    if (line[0] == '\0') return -1;
    {
        char buf[256];
        size_t l = strlen(line);
        if (l && line[l-1] == '\n') line[l-1] = '\0';
        snprintf(buf, sizeof buf, "CM %s", line);
        append_card_from_text(deck, buf);
        append_card_from_text(deck, "CE");
    }
    /* next numeric line is frequency */
    while (fgets(line, sizeof line, fp)) {
        if (line[0] == '*' || line[0] == '\n') continue;
        freq = atof(line);
        break;
    }
    /* now counts line: look for first line containing only integers (no dot)
       this handles the case where the next numeric item is a lone '7' after
       the ***Wires*** header. */
    while (fgets(line, sizeof line, fp)) {
        /* skip headers and comments */
        if (line[0] == '*' || line[0] == '\n') continue;
        /* ignore lines with a decimal point (likely floats) */
        if (strchr(line, '.')) continue;
        char *p = line;
        int vals[3] = {0,0,0};
        int count = 0;
        while (*p && count < 3) {
            /* skip non-digit, non-minus */
            if ((*p >= '0' && *p <= '9') || *p == '-') {
                long v = strtol(p, &p, 10);
                vals[count++] = (int)v;
            } else {
                p++;
            }
        }
        if (count > 0) {
            n_wires = vals[0];
            break;
        }
    }
    /* add FR card */
    if (freq > 0) {
        char buf[128];
        snprintf(buf, sizeof buf, "FR 0,0,%.6f,0,0,0,0,0", freq);
        append_card_from_text(deck, buf);
    }

    /* wires */
    for (int i = 0; i < n_wires; ) {
        if (!maa_read_data_line(fp, line)) break;
        /* handle comment markers that may precede or interrupt the wire list */
        if (strncmp(line, "###Comment###", strlen("###Comment###")) == 0) {
            char *t = line + 13;
            while (*t && isspace((unsigned char)*t)) t++;
            char comment_buf[512];
            if (*t == '\0' || *t == '\n') {
                /* grab next line for the comment text */
                if (fgets(comment_buf, sizeof comment_buf, fp)) {
                    t = comment_buf;
                } else {
                    t = "";
                }
            }
            char *nl = strchr(t, '\n'); if (nl) *nl = '\0';
            while (*t && isspace((unsigned char)*t)) t++;
            char buf[300];
            snprintf(buf, sizeof buf, "CM %s", t);
            append_card_from_text(deck, buf);
            append_card_from_text(deck, "CE");
            continue;
        }
        double x1,y1,z1,x2,y2,z2,rad; int segs;
        if (sscanf(line, "%lf%*[, ]%lf%*[, ]%lf%*[, ]%lf%*[, ]%lf%*[, ]%lf%*[, ]%lf%*[, ]%d",
                   &x1,&y1,&z1,&x2,&y2,&z2,&rad,&segs) >= 8) {
            char buf[256];
            snprintf(buf, sizeof buf,
                     "GW %d, %d, %.6f, %.6f, %.6f, %.6f, %.6f, %.6f, %.6f",
                     i+1, segs, x1,y1,z1,x2,y2,z2,rad);
            append_card_from_text(deck, buf);
            i++;
        }
    }
    /* GE termination */
    append_card_from_text(deck, "GE");

    /* parse following sections based on headers; headers must be seen
       so we read raw lines rather than using the helper. */
    int section = 0; // 1=load, 2=source
    while (fgets(line, sizeof line, fp)) {
        if (strncmp(line, "***Load***", strlen("***Load***")) == 0) {
            section = 1;
            continue;
        }
        if (strncmp(line, "***Source***", strlen("***Source***")) == 0) {
            section = 2;
            continue;
        }
        if (strncmp(line, "***", strlen("***")) == 0) {
            section = 0;
            continue;
        }
        /* inline comments – wrap each in a CM/CE pair to retain ordering
           MMANA often puts the actual text on the *next* line after the
           ###Comment### marker, so handle both cases. */
        if (strncmp(line, "###Comment###", strlen("###Comment###")) == 0) {
            char *t = line + 13;
            while (*t && isspace((unsigned char)*t)) t++;
            char comment_buf[512];
            if (*t == '\0' || *t == '\n') {
                /* read following line for the comment text */
                if (fgets(comment_buf, sizeof comment_buf, fp)) {
                    t = comment_buf;
                } else {
                    t = "";
                }
            }
            /* trim newline and leading whitespace */
            char *nl = strchr(t, '\n'); if (nl) *nl = '\0';
            while (*t && isspace((unsigned char)*t)) t++;
            char buf[300];
            snprintf(buf, sizeof buf, "CM %s", t);
            append_card_from_text(deck, buf);
            append_card_from_text(deck, "CE");
            continue;
        }
        /* regular data line */
        if (section == 1) {
            int wire, seg;
            double R=0,X=0,L=0,C=0;
            if (sscanf(line, "%d%*[, ]%d%*[, ]%lf%*[, ]%lf%*[, ]%lf%*[, ]%lf",
                       &wire,&seg,&R,&X,&L,&C) >= 2) {
                char buf[256];
                snprintf(buf, sizeof buf,
                         "LD 0, %d, %d, %.6g, %.6g, %.6g, %.6g",
                         wire, seg, R, X, L, C);
                append_card_from_text(deck, buf);
            }
        } else if (section == 2) {
            int wire, seg;
            double mag=1.0, phase=0.0;
            if (sscanf(line, "%d%*[, ]%d%*[, ]%lf%*[, ]%lf",
                       &wire,&seg,&mag,&phase) >= 2) {
                double phrad = phase * M_PI / 180.0;
                double re = mag * cos(phrad);
                double im = mag * sin(phrad);
                char buf[256];
                snprintf(buf, sizeof buf,
                         "EX 0, %d, %d, %.6f, %.6f, 0,0,0",
                         wire, seg, re, im);
                append_card_from_text(deck, buf);
            }
        }
    }


    return 0;
}
