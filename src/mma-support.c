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

    /* populate comment field for CM/CE/'!': write_deck_nec prints card_code+comment */
    {
        int code_end = 0;
        if (strcmp(card.card_code, "CM") == 0 || strcmp(card.card_code, "CE") == 0)
            code_end = 2;
        else if (card.card_code[0] == '!' || card.card_code[0] == '#')
            code_end = 1;
        if (code_end > 0) {
            const char *rest = p + code_end;
            card.comment = strdup(rest);
        }
    }

    if (insert_card(deck, &card, deck->num_cards) < 0) {
        return -1;
    }
    return 0;
}

/* insert a card at an arbitrary position (0..num_cards). Similar to
   append_card_from_text but allows specifying the insertion index. */
static int insert_card_from_text_at(deck_t *deck, const char *text, int pos)
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

    if (pos < 0) pos = 0;
    if (pos > deck->num_cards) pos = deck->num_cards;
    if (insert_card(deck, &card, pos) < 0) {
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

    /* title: use first CM/comment card if present and non-empty; blank line otherwise */
    const char *title = "";
    int title_card_idx = -1;
    for (int i = 0; i < deck->num_cards; i++) {
        card_t *c = &deck->cards[i];
        if (card_is_commented_out(c) || strcmp(c->card_code, "CM") == 0) {
            if (c->comment && c->comment[0] != '\0') {
                title = c->comment;
                /* skip any leading space left from "CM <text>" parsing */
                while (*title && isspace((unsigned char)*title)) title++;
                title_card_idx = i;
            }
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
    /* Variant B: ***Wires*** header with wire count on its own line */
    fprintf(fp, "***Wires***\n");
    fprintf(fp, "%d\n", nw);

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

    /* Variant B: ***Source*** comes before ***Load*** */

    /* sources: Variant B format  w<N>c, <phase°>, <mag> */
    fprintf(fp, "***Source***\n");
    fprintf(fp, "%d, 0\n", ns);
    for (int i = 0, count = 0; i < deck->num_cards && count < ns; i++) {
        card_t *c = &deck->cards[i];
        if (strcmp(c->card_code, "EX") != 0) continue;
        count++;
        int wire = c->i[2];
        double re = c->f[1];
        double im = c->f[2];
        double mag = hypot(re, im);
        double phase = atan2(im, re) * 180.0 / M_PI;
        fprintf(fp, "w%dc, %.2f, %.6f\n", wire, phase, mag);
    }

    /* loads */
    fprintf(fp, "***Load***\n");
    fprintf(fp, "%d, 0\n", nl);
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

    /* ###Comment### block — always last in the file.
       Emit CM cards (excluding the title) and '!' comment cards
       (excluding importer metadata lines that start with "maa-"). */
    for (int i = 0; i < deck->num_cards; i++) {
        if (i == title_card_idx) continue;
        card_t *c = &deck->cards[i];
        const char *txt = NULL;
        if (card_is_commented_out(c) || strcmp(c->card_code, "CM") == 0) {
            txt = c->comment ? c->comment : c->orig_str;
            if (txt) {
                while (*txt && isspace((unsigned char)*txt)) txt++;
            }
        } else if (c->card_code[0] == '!' && c->card_str) {
            /* '!' comment card from importer — skip internal metadata markers */
            const char *s = c->card_str;
            while (*s == '!' || isspace((unsigned char)*s)) s++;
            if (strncmp(s, "maa-", 4) != 0)   /* exclude maa-segmentation etc. */
                txt = s;
        }
        if (txt && txt[0] != '\0')
            fprintf(fp, "###Comment###\n%s\n", txt);
    }

    /* end marker */
    fprintf(fp, "\n");
    return 0;
}

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

/**
 * @copydoc read_deck_maa
 */
int read_deck_maa(deck_t *deck, FILE *fp)
{
    if (!deck || !fp) return -1;
    char line[512];
    int n_wires = 0;
    double freq = 0.0;
    int title_ce_index = -1;
    int any_minus1 = 0;
    int wire_segs[1024] = {0}; /* raw seg count per wire (0-based); may be <=0 for auto-seg */
    /* ###Comment### blocks are always placed after EN — collect here and flush at the end */
    char *pending_comments[256];
    int   n_pending = 0;

    /* read title:
     * - Variant A / Variant B with title: first non-blank line is the title text.
     * - Variant B without title: file starts with a bare '*' separator line; that
     *   line is consumed here and no CM/CE are emitted.
     * We stop at the first non-blank line regardless of whether it starts with '*'. */
    line[0] = '\0';
    while (fgets(line, sizeof line, fp)) {
        if (line[0] == '\n') continue;   /* skip leading blank lines */
        break;                           /* stop at first non-blank line */
    }
    if (line[0] == '\0') return -1;      /* truly empty file */
    if (line[0] != '*') {
        /* real title text */
        char buf[256];
        size_t l = strlen(line);
        if (l && line[l-1] == '\n') line[l-1] = '\0';
        snprintf(buf, sizeof buf, "CM %s", line);
        append_card_from_text(deck, buf);
        append_card_from_text(deck, "CE");
        title_ce_index = deck->num_cards - 1;
    }
    /* else: Variant B no-title — the '*' line was consumed; frequency follows next */

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
    /* frequency is saved so that we can emit the FR card after the
       GE termination later; MMANA expects FR to come immediately after GE,
       not at the top of the deck.  */

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
            snprintf(buf, sizeof buf, "! %s", t);
            /* defer: emit after EN */
            if (n_pending < 256) pending_comments[n_pending++] = strdup(buf);
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
            wire_segs[i < 1024 ? i : 1023] = segs;
            if (segs <= 0) any_minus1 = 1;
            i++;
        }
    }
    /* if needed insert SY default and replace -1 tokens with the literal
       'segs' in GW card strings so the user can edit the SY value later. */
    if (any_minus1) {
        /* insert SY helper just after CE when present, otherwise at position 0 */
        int insert_at = (title_ce_index >= 0) ? title_ce_index + 1 : 0;
        const char *sytext = "SY segs=10 !default segment count, change to realistic value";
        insert_card_from_text_at(deck, sytext, insert_at);
        for (int k = 0; k < deck->num_cards; k++) {
            card_t *c = &deck->cards[k];
            if (strcmp(c->card_code, "GW") != 0) continue;
            if (!c->card_str) continue;
            char *p1 = strchr(c->card_str, ',');
            if (!p1) continue;
            char *p2 = strchr(p1 + 1, ',');
            if (!p2) continue;
            /* extract token between p1 and p2 */
            int toklen = (int)(p2 - (p1 + 1));
            if (toklen <= 0 || toklen > 32) continue;
            char token[40];
            strncpy(token, p1 + 1, toklen);
            token[toklen] = '\0';
            /* trim */
            char *ts = token;
            while (*ts && isspace((unsigned char)*ts)) ts++;
            char *te = token + strlen(token) - 1;
            while (te > ts && isspace((unsigned char)*te)) *te-- = '\0';
            if (atoi(ts) <= 0) {
                /* build new string: prefix up to p1+1, then ' segs', then remainder from p2 */
                size_t newlen = strlen(c->card_str) + 5;
                char *ns = malloc(newlen);
                if (!ns) continue;
                size_t pre = (size_t)(p1 - c->card_str) + 1;
                strncpy(ns, c->card_str, pre);
                ns[pre] = '\0';
                strcat(ns, " segs");
                strcat(ns, p2);
                free(c->card_str);
                c->card_str = ns;
                /* mirror to orig_str */
                if (c->orig_str) {
                    free(c->orig_str);
                    c->orig_str = strdup(c->card_str);
                }
            }
        }
    }

    /* GE termination */
    append_card_from_text(deck, "GE");
    /* FR and GN are deferred until after the section-parsing loop so that
       the ***G/H/M/R/AzEl/X*** ground parameters can be read first and GN
       can be placed before FR in the correct NEC order. */

    /* parse following sections based on headers; headers must be seen
       so we read raw lines rather than using the helper. */
    int section = 0; /* 1=load 2=source 3=seg 4=ground */
    int skip_count = 0; /* set after a section header to eat the count line */
    /* ground parameters collected from ***G/H/M/R/AzEl/X*** */
    int   g_type    = 0;   /* 0=free-space 1=perfect 2=real/MININEC -1=S-N  */
    double g_sigma_ms = 0.0; /* conductivity in mS/m (0=unspecified/default) */
    int   g_radials = 0;   /* radials count (informational; no GD without radius/len) */
    int   g_found   = 0;   /* set to 1 when a ground line has been parsed */
    while (fgets(line, sizeof line, fp)) {
        if (strncmp(line, "***Load***", strlen("***Load***")) == 0) {
            section = 1;
            skip_count = 1;
            continue;
        }
        if (strncmp(line, "***Source***", strlen("***Source***")) == 0) {
            section = 2;
            skip_count = 1;
            continue;
        }
        if (strncmp(line, "***Segmentation***", strlen("***Segmentation***")) == 0) {
            section = 3;
            continue;
        }
        if (strncmp(line, "***G/H/M/R", strlen("***G/H/M/R")) == 0) {
            section = 4;
            continue;
        }
        if (strncmp(line, "***", strlen("***")) == 0) {
            section = 0;
            skip_count = 0;
            continue;
        }
        /* skip the count line that immediately follows a section header */
        if (skip_count) {
            skip_count = 0;
            continue;
        }
        /* ###Comment### — always deferred to after EN */
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
            snprintf(buf, sizeof buf, "! %s", t);
            if (n_pending < 256) pending_comments[n_pending++] = strdup(buf);
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
            int wire = 0, seg = 1;
            char seg_expr[64] = {0};
            int seg_uses_expr = 0;
            double mag = 1.0, phase = 0.0;
            /* Variant B:  <src-designator>, <phase°>, <mag>
               Designator: [WwVv] <wire#> [CcBbEe] [<signed-offset>]
               W/V = voltage source; C=centre, B=begin, E=end;
               optional signed integer offset from the attachment point. */
            if ((line[0] == 'w' || line[0] == 'W' ||
                 line[0] == 'v' || line[0] == 'V') &&
                isdigit((unsigned char)line[1])) {
                char *p = line + 1;
                wire = (int)strtol(p, &p, 10);
                /* attachment letter: C (default), B, or E */
                char attach = 'C';
                if      (*p == 'c' || *p == 'C') { attach = 'C'; p++; }
                else if (*p == 'b' || *p == 'B') { attach = 'B'; p++; }
                else if (*p == 'e' || *p == 'E') { attach = 'E'; p++; }
                /* optional signed offset immediately after the letter */
                int offset = 0;
                if (*p == '+' || *p == '-' || isdigit((unsigned char)*p)) {
                    offset = (int)strtol(p, &p, 10);
                }
                /* resolve to NEC 1-based segment index.
                   When the wire uses MMANA auto-segmentation (wsc <= 0) and
                   the attachment is C or E, emit a tinyexpr expression that
                   references the 'segs' symbol from the SY card, so the
                   correct segment is automatically used once the user sets
                   segs to a concrete value.  B-attachment has no dependency
                   on the total segment count so it always resolves to an
                   integer. */
                int wsc = (wire >= 1 && wire <= n_wires) ? wire_segs[wire-1] : 0;
                if (wsc <= 0) {
                    if (attach == 'C') {
                        if (offset == 0)
                            snprintf(seg_expr, sizeof seg_expr, "(segs+1)/2");
                        else
                            snprintf(seg_expr, sizeof seg_expr, "(segs+1)/2%+d", offset);
                        seg_uses_expr = 1;
                    } else if (attach == 'E') {
                        if (offset == 0)
                            snprintf(seg_expr, sizeof seg_expr, "segs");
                        else
                            snprintf(seg_expr, sizeof seg_expr, "segs%+d", offset);
                        seg_uses_expr = 1;
                    } else { /* B — first segment, no segs dependency */
                        seg = 1 + offset;
                        if (seg < 1) seg = 1;
                    }
                } else {
                    if (attach == 'C') {
                        seg = (wsc + 1) / 2 + offset;
                    } else if (attach == 'B') {
                        seg = 1 + offset;
                    } else { /* E */
                        seg = wsc + offset;
                    }
                    /* clamp to valid range */
                    if (seg < 1)   seg = 1;
                    if (seg > wsc) seg = wsc;
                }
                /* now read phase and magnitude */
                while (*p && (isspace((unsigned char)*p) || *p == ',')) p++;
                phase = strtod(p, &p);
                while (*p && (isspace((unsigned char)*p) || *p == ',')) p++;
                mag   = strtod(p, &p);
            } else {
                /* Variant A:  wire, seg, mag, phase° */
                sscanf(line, "%d%*[, ]%d%*[, ]%lf%*[, ]%lf",
                       &wire, &seg, &mag, &phase);
            }
            if (wire > 0) {
                double phrad = phase * M_PI / 180.0;
                double re = mag * cos(phrad);
                double im = mag * sin(phrad);
                char buf[256];
                if (seg_uses_expr)
                    snprintf(buf, sizeof buf,
                             "EX 0, %d, %s, %.6f, %.6f, 0,0,0",
                             wire, seg_expr, re, im);
                else
                    snprintf(buf, sizeof buf,
                             "EX 0, %d, %d, %.6f, %.6f, 0,0,0",
                             wire, seg, re, im);
                append_card_from_text(deck, buf);
            }
        } else if (section == 3) {
            /* ***Segmentation***: one line, four values:
               max-segs, segs-per-wavelength, taper-ratio, min-segs-per-wire */
            int max_segs = 0, segs_per_wl = 0, min_segs = 0;
            double taper = 0.0;
            if (sscanf(line, "%d%*[, ]%d%*[, ]%lf%*[, ]%d",
                       &max_segs, &segs_per_wl, &taper, &min_segs) == 4) {
                char buf[256];
                snprintf(buf, sizeof buf,
                         "! maa-segmentation: max-segs=%d segs-per-wl=%d taper=%.4g min-segs=%d",
                         max_segs, segs_per_wl, taper, min_segs);
                append_card_from_text(deck, buf);
            }
            section = 0; /* only one data line in this section */
        } else if (section == 4) {
            /* ***G/H/M/R/AzEl/X***: one data line with 7 comma-separated fields:
               gtype, sigma_mS/m, radials, ref_Z, phi_pts, el_pts, unused
               ground types: 0=free-space, 1=perfect, 2=real/MININEC, -1=S-N */
            double f2 = 0.0;
            int    f3 = 0;
            if (sscanf(line, "%d%*[, ]%lf%*[, ]%d", &g_type, &f2, &f3) >= 1) {
                g_sigma_ms = f2;
                g_radials  = f3;
                g_found    = 1;
            }
            section = 0; /* only one data line in this section */
        }
    } /* end while (section-parsing loop) */

    /* emit GN card if the file specified a ground model.
       Placed before FR so the deck is in the correct NEC order. */
    if (g_found && g_type != 0) {
        char buf[256];
        if (g_type == 1) {
            /* perfect / ideal ground */
            append_card_from_text(deck, "GN 1");
        } else {
            /* real ground: g_type 2 = MININEC (GN 0), -1 = Sommerfeld-Norton (GN 2)
               Field 2 is conductivity in mS/m; the file does not carry epsr so
               we derive it from standard NEC soil-type correlations. */
            int nec_gtype = (g_type == -1) ? 2 : 0;
            double sigma_s, epsr;
            if (g_sigma_ms <= 0.0) {
                epsr = 13.0; sigma_s = 0.005;          /* unspecified → average  */
            } else if (g_sigma_ms <= 1.0) {
                epsr =  5.0; sigma_s = g_sigma_ms / 1000.0; /* poor/rocky          */
            } else if (g_sigma_ms <= 8.0) {
                epsr = 13.0; sigma_s = g_sigma_ms / 1000.0; /* average             */
            } else if (g_sigma_ms <= 30.0) {
                epsr = 17.0; sigma_s = g_sigma_ms / 1000.0; /* good/agricultural   */
            } else {
                epsr = 25.0; sigma_s = g_sigma_ms / 1000.0; /* very good           */
            }
            snprintf(buf, sizeof buf,
                     "GN %d, 0, 0, 0, %.4g, %.6g",
                     nec_gtype, epsr, sigma_s);
            append_card_from_text(deck, buf);
            if (g_radials > 0) {
                snprintf(buf, sizeof buf,
                         "! maa-ground-radials: %d (GD card not emitted: radius/length unknown)",
                         g_radials);
                append_card_from_text(deck, buf);
            }
        }
    }

    /* frequency card — after GN so it appears in the correct NEC order */
    if (freq > 0) {
        char buf[128];
        snprintf(buf, sizeof buf, "FR 0,0,%.6f,0,0,0,0,0", freq);
        append_card_from_text(deck, buf);
    }

    /* append RP for full 3D far-field pattern (37 theta × 73 phi, 5° steps) */
    append_card_from_text(deck, "RP 0, 37, 73, 1000, 0, 0, 5, 5");

    /* append EN terminator */
    append_card_from_text(deck, "EN");

    /* flush ###Comment### blocks — always after EN */
    for (int ci = 0; ci < n_pending; ci++) {
        append_card_from_text(deck, pending_comments[ci]);
        free(pending_comments[ci]);
    }

    return 0;
}
