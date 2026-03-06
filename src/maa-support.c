/******************************************************************************
 * maa-support.c
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

#include "maa-support.h"
#include "deck.h"    // for insert_card
#include "misc.h"    // for add_error
#include "geometry.h" // for compute_segmentation

#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <limits.h>


/**
 * @copydoc write_deck_maa
 */

/**
 * @brief Parse the nth numeric token from a card's string (1-based, past the code).
 *
 * Skips the two-character card code and then counts comma/space-delimited
 * numeric tokens.  Returns the parsed value, or 0.0 if the field is absent
 * or the card has no string.  Used by the exporter to extract float fields
 * (re, im, R, X, …) without depending on the i[]/f[] arrays, which are only
 * populated after formula evaluation (which may be skipped with -n).
 */
static double card_field_n(const card_t *c, int n)
{
    if (!c || !c->card_str) return 0.0;
    const char *p = c->card_str;
    /* skip card code (non-space chars at start) */
    while (*p && !isspace((unsigned char)*p)) p++;
    int field = 0;
    while (*p) {
        while (*p && (isspace((unsigned char)*p) || *p == ',')) p++;
        if (!*p) break;
        field++;
        if (field == n) return strtod(p, NULL);
        /* skip this token */
        while (*p && *p != ',' && !isspace((unsigned char)*p)) p++;
    }
    return 0.0;
}

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
    fprintf(fp, "%s\n*\n", title);

    /* frequency: look for FR card. Cards created by append_card_from_text do
     * not have f[] fields populated, so parse the string directly.
     * NEC FR format: I1=IFRQ, I2=NFRQ, I3=IZPE, I4=NOPH, F1=FMHZ, F2=DELFRQ.
     * FMHZ is the 1st float field — the 5th token after the card code. */
    double freq = 14.0; /* default */
    for (int i = 0; i < deck->num_cards; i++) {
        card_t *c = &deck->cards[i];
        if (strcmp(c->card_code, "FR") == 0) {
            /* Try parsed fields first (cards loaded by read_deck + parse_deck) */
            if (c->f[1] != 0.0) {
                freq = c->f[1];
            } else if (c->card_str) {
                /* Fall back to parsing the string: skip "FR" then four int fields */
                const char *p = c->card_str;
                while (*p && !isdigit((unsigned char)*p) && *p != '-') p++; /* skip "FR" */
                /* skip 4 integer fields (I1..I4) separated by comma/space */
                for (int skip = 0; skip < 4; skip++) {
                    while (*p && (isdigit((unsigned char)*p) || *p == '-')) p++;
                    while (*p && (*p == ',' || isspace((unsigned char)*p))) p++;
                }
                if (*p) freq = atof(p);
            }
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
    /* Scan for segmentation annotation cards before writing wires so that
     * the wire table can restore the original SEG mode (e.g. -1) instead of
     * the computed positive count, allowing MMANA to re-auto-segment.
     *
     * The importer stores:
     *   ! maa-segmentation: dm1=N dm2=N sc=N ec=N [mode=N]  (global params)
     *   GW ..., count, ... !segmentation:M              (per-wire mode suffix) */
    int    exp_dm1 = 0, exp_dm2 = 0, exp_ec = 0;
    double exp_sc  = 0.0;
    int    exp_common_mode = INT_MIN; /* INT_MIN = not found */
    int    have_seg_params = 0;
    for (int i = 0; i < deck->num_cards; i++) {
        card_t *c = &deck->cards[i];
        if (c->card_code[0] != '!' || !c->card_str) continue;
        const char *s = c->card_str;
        while (*s == '!' || isspace((unsigned char)*s)) s++;
        if (strncmp(s, "maa-segmentation:", 17) != 0) continue;
        s += 17;
        const char *p = s;
        while (*p) {
            while (*p && (isspace((unsigned char)*p) || *p == ',')) p++;
            if (!*p) break;
            char key[16] = {0}; int ki = 0;
            while (*p && *p != '=' && ki < 15) key[ki++] = *p++;
            if (*p == '=') p++;
            char *end;
            double val = strtod(p, &end);
            if (end > p) {
                if      (strcmp(key, "dm1")  == 0) exp_dm1 = (int)val;
                else if (strcmp(key, "dm2")  == 0) exp_dm2 = (int)val;
                else if (strcmp(key, "sc")   == 0) exp_sc  = val;
                else if (strcmp(key, "ec")   == 0) exp_ec  = (int)val;
                else if (strcmp(key, "mode") == 0) exp_common_mode = (int)val;
                p = end;
            } else { p++; }
        }
        if (exp_dm1 > 0 && exp_dm2 > 0) have_seg_params = 1;
        break;
    }
    /* per-wire mode table (1-based tag index) */
    int per_wire_mode[1024];
    for (int i = 0; i < 1024; i++) per_wire_mode[i] = INT_MIN;
    if (have_seg_params) {
        for (int i = 0; i < deck->num_cards; i++) {
            card_t *c = &deck->cards[i];
            if (strcmp(c->card_code, "GW") != 0 || !c->card_str) continue;
            const char *ann = strstr(c->card_str, "!segmentation:");
            if (!ann) continue;
            int mode_val = (int)strtol(ann + 14, NULL, 10);
            int tag = 0;
            sscanf(c->card_str + 2, " %d", &tag);
            if (tag >= 1 && tag <= 1023) per_wire_mode[tag] = mode_val;
        }
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
        /* Restore original SEG mode when we have segmentation params:
         * prefer per-wire annotation, then common mode, then computed count */
        int segs = c->i[2];
        if (have_seg_params) {
            int tag = c->i[1];
            if (tag >= 1 && tag <= 1023 && per_wire_mode[tag] != INT_MIN)
                segs = per_wire_mode[tag];
            else if (exp_common_mode != INT_MIN)
                segs = exp_common_mode;
        }
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
        /* f[] may not be populated when -n skips formula eval; parse from string */
        double re = card_field_n(c, 4);
        double im = card_field_n(c, 5);
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
        /* f[] may not be populated when -n skips formula eval; parse from string */
        double R = card_field_n(c, 5);
        double X = card_field_n(c, 6);
        double L = card_field_n(c, 7);
        double C = card_field_n(c, 8);
        fprintf(fp, "%d, %d, %.6g, %.6g, %.6g, %.6g\n",
                wire, seg, R, X, L, C);
    }

    /* ***Segmentation*** block — written when the deck carries the annotation */
    if (have_seg_params) {
        double sc_out = exp_sc > 0.0 ? exp_sc : 2.0;
        int    ec_out = exp_ec > 0   ? exp_ec : 1;
        fprintf(fp, "***Segmentation***\n");
        fprintf(fp, "%d, %d, %.4g, %d\n", exp_dm1, exp_dm2, sc_out, ec_out);
    }

    /* ###Comment### block — always last in the file.
       Emit CM cards (excluding the title) and '!' comment cards
       (excluding segmentation: metadata). */
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
            const char *s = c->card_str;
            while (*s == '!' || isspace((unsigned char)*s)) s++;
            if (strncmp(s, "maa-segmentation:", 17) != 0)   /* exclude maa-segmentation: metadata */
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
    int any_minus1 = 0;
    int    wire_segs[1024] = {0}; /* original SEG mode per wire; <=0 means auto-seg */
    double wire_lens[1024] = {0}; /* physical wire length per wire (metres) */
    /* MMANA segmentation params from ***Segmentation*** block (order: DM1 DM2 SC EC) */
    int    seg_dm1 = 200, seg_dm2 = 20, seg_ec = 1;
    double seg_sc  = 2.0;
    /* uniformity of seg modes across auto-seg wires (computed after wire loop) */
    int    common_seg_mode   = 0;  /* mode value if all auto-seg wires share one mode */
    int    all_same_seg_mode = 1;  /* 0 if any two auto-seg wires differ in mode */
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
            int wi = i < 1024 ? i : 1023;
            wire_segs[wi] = segs;
            /* store wire length for segmentation computation later */
            double dx = x2-x1, dy = y2-y1, dz = z2-z1;
            wire_lens[wi] = sqrt(dx*dx + dy*dy + dz*dz);
            if (segs <= 0) any_minus1 = 1;
            i++;
        }
    }
    /* Determine if all auto-seg wires share the same mode (for annotation) */
    {
        int first_auto = 1; /* 1 until we see the first auto-seg wire */
        for (int i = 0; i < n_wires; i++) {
            if (wire_segs[i] <= 0) {
                if (first_auto) {
                    common_seg_mode = wire_segs[i];
                    first_auto = 0;
                } else if (wire_segs[i] != common_seg_mode) {
                    all_same_seg_mode = 0;
                    break;
                }
            }
        }
    }
    /* GE termination */
    int ge_card_idx = deck->num_cards; /* index where GE will be placed */
    append_card_from_text(deck, "GE");
    /* FR and GN are deferred until after the section-parsing loop so that
       the ***G/H/M/R/AzEl/X*** ground parameters can be read first and GN
       can be placed before FR in the correct NEC order. */

    /* parse following sections based on headers; headers must be seen
       so we read raw lines rather than using the helper. */
    int section = 0; /* 1=load 2=source 3=seg 4=ground */
    int skip_count = 0; /* set after a section header to eat the count line */
    /* The maa-segmentation annotation is deferred so it can be inserted
       just before GE (in the geometry block) rather than after EX/LD. */
    char seg_annotation[300] = {0};
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
            /* ***Segmentation***: one CSV line with order DM1, DM2, SC, EC.
               Store for use in post-processing; a comment card is also emitted
               so the values survive a round-trip through the deck. */
            int dm1_tmp = 0, dm2_tmp = 0, ec_tmp = 0;
            double sc_tmp = 0.0;
            if (sscanf(line, "%d%*[, ]%d%*[, ]%lf%*[, ]%d",
                       &dm1_tmp, &dm2_tmp, &sc_tmp, &ec_tmp) >= 3) {
                if (dm1_tmp > 0) seg_dm1 = dm1_tmp;
                if (dm2_tmp > 0) seg_dm2 = dm2_tmp;
                if (sc_tmp  > 1.0) seg_sc = sc_tmp;
                if (ec_tmp  > 0)   seg_ec = ec_tmp;
                if (all_same_seg_mode && common_seg_mode <= 0)
                    snprintf(seg_annotation, sizeof seg_annotation,
                             "! maa-segmentation: dm1=%d dm2=%d sc=%.4g ec=%d mode=%d",
                             seg_dm1, seg_dm2, seg_sc, seg_ec, common_seg_mode);
                else
                    snprintf(seg_annotation, sizeof seg_annotation,
                             "! maa-segmentation: dm1=%d dm2=%d sc=%.4g ec=%d",
                             seg_dm1, seg_dm2, seg_sc, seg_ec);
                /* annotation is inserted before GE after the section loop */
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

    /* Insert the maa-segmentation annotation just before GE so it appears in
     * the geometry block, above the EX/LD cards that follow GE. */
    if (seg_annotation[0]) {
        append_card_from_text(deck, seg_annotation); /* appended last for now */
        /* slide it left to ge_card_idx (before GE) */
        card_t tmp = deck->cards[deck->num_cards - 1];
        memmove(&deck->cards[ge_card_idx + 1],
                &deck->cards[ge_card_idx],
                (deck->num_cards - 1 - ge_card_idx) * sizeof(card_t));
        deck->cards[ge_card_idx] = tmp;
    }

    /* GW post-processing: fix auto-seg counts and EX segment references.
     *
     * The ***Segmentation*** params (DM1/DM2/SC/EC) live at the *end* of the
     * .maa file, after ***Source***, so they are only known here — after the
     * section-parsing loop above.  Similarly, EX cards emitted during source
     * parsing may carry "segs"-based expressions when the original wire used
     * auto-segmentation; once we have the real counts we can replace them with
     * concrete integers. */
    if (any_minus1) {
        double wl = (freq > 0.0) ? (299.8 / freq) : 1.0;

        /* --- pass 1: fix GW segment counts --- */
        for (int k = 0; k < deck->num_cards; k++) {
            card_t *c = &deck->cards[k];
            if (strcmp(c->card_code, "GW") != 0 || !c->card_str) continue;
            char *p1 = strchr(c->card_str, ',');
            if (!p1) continue;
            char *p2 = strchr(p1 + 1, ',');
            if (!p2) continue;
            int toklen = (int)(p2 - (p1 + 1));
            if (toklen <= 0 || toklen > 32) continue;
            char token[40];
            strncpy(token, p1 + 1, toklen);
            token[toklen] = '\0';
            char *ts = token;
            while (*ts && isspace((unsigned char)*ts)) ts++;
            char *te = ts + strlen(ts) - 1;
            while (te > ts && isspace((unsigned char)*te)) *te-- = '\0';
            int raw_segs = atoi(ts);
            if (raw_segs > 0) continue;

            int tag = 0;
            sscanf(c->card_str + 2, " %d", &tag);
            int widx = tag - 1;

            int computed = 1;
            if (widx >= 0 && widx < n_wires && wire_lens[widx] > 0.0)
                computed = compute_segmentation(wire_lens[widx], wl, raw_segs,
                                                seg_dm1, seg_dm2, seg_sc, seg_ec, NULL);
            if (computed < 1) computed = 1;
            if (widx >= 0 && widx < 1024)
                wire_segs[widx] = computed; /* now positive; used by EX pass below */

            char replacement[32];
            snprintf(replacement, sizeof replacement, " %d", computed);
            char ann[32] = "";
            if (!all_same_seg_mode)
                snprintf(ann, sizeof ann, " !segmentation:%d", raw_segs);

            size_t pre = (size_t)((p1 + 1) - c->card_str);
            size_t suf = strlen(p2);
            size_t newlen = pre + strlen(replacement) + suf + strlen(ann) + 1;
            char *ns = malloc(newlen);
            if (!ns) continue;
            memcpy(ns, c->card_str, pre);
            ns[pre] = '\0';
            strcat(ns, replacement);
            strcat(ns, p2);
            strcat(ns, ann);
            free(c->card_str);
            c->card_str = ns;
            if (c->orig_str) { free(c->orig_str); c->orig_str = strdup(ns); }
        }

        /* --- pass 2: fix EX segment expressions that reference "segs" ---
         * Patterns emitted by the source section when wsc <= 0:
         *   "(segs+1)/2"        centre attachment
         *   "(segs+1)/2+N"      centre + offset
         *   "segs"              end attachment
         *   "segs+N"/"segs-N"   end + offset
         * Replace each with the concrete integer now that wire_segs is set. */
        for (int k = 0; k < deck->num_cards; k++) {
            card_t *c = &deck->cards[k];
            if (strcmp(c->card_code, "EX") != 0 || !c->card_str) continue;
            /* EX format: "EX 0, wire, seg, re, im, 0,0,0" */
            char *p1 = strchr(c->card_str, ',');        /* after field-0 */
            if (!p1) continue;
            char *p2 = strchr(p1 + 1, ',');             /* after wire   */
            if (!p2) continue;
            char *p3 = strchr(p2 + 1, ',');             /* after seg    */
            if (!p3) continue;
            int seglen = (int)(p3 - (p2 + 1));
            if (seglen <= 0 || seglen > 64) continue;
            char segfield[68];
            strncpy(segfield, p2 + 1, seglen);
            segfield[seglen] = '\0';
            if (!strstr(segfield, "segs")) continue;    /* numeric — skip */

            int tag = 0;
            sscanf(p1 + 1, " %d", &tag);
            int wsc = (tag >= 1 && tag <= n_wires) ? wire_segs[tag - 1] : 1;
            if (wsc < 1) wsc = 1;

            /* evaluate expression */
            int seg_val = 1;
            char *sf = segfield;
            while (*sf && isspace((unsigned char)*sf)) sf++;
            if (strncmp(sf, "(segs+1)/2", 10) == 0) {
                seg_val = (wsc + 1) / 2;
                sf += 10;
                if (*sf == '+' || *sf == '-')
                    seg_val += (int)strtol(sf, NULL, 10);
            } else if (strncmp(sf, "segs", 4) == 0) {
                seg_val = wsc;
                sf += 4;
                if (*sf == '+' || *sf == '-')
                    seg_val += (int)strtol(sf, NULL, 10);
            }
            if (seg_val < 1)   seg_val = 1;
            if (seg_val > wsc) seg_val = wsc;

            char replacement[32];
            snprintf(replacement, sizeof replacement, " %d", seg_val);
            size_t pre = (size_t)((p2 + 1) - c->card_str);
            size_t suf = strlen(p3);
            size_t newlen = pre + strlen(replacement) + suf + 1;
            char *ns = malloc(newlen);
            if (!ns) continue;
            memcpy(ns, c->card_str, pre);
            ns[pre] = '\0';
            strcat(ns, replacement);
            strcat(ns, p3);
            free(c->card_str);
            c->card_str = ns;
            if (c->orig_str) { free(c->orig_str); c->orig_str = strdup(ns); }
        }
    }

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

    /* frequency card — after GN so it appears in the correct NEC order.
     * NEC FR: I1=IFRQ(0), I2=NFRQ(1), I3=IZPE(0), I4=NOPH(0), F1=FMHZ, F2=step(0) */
    if (freq > 0) {
        char buf[128];
        snprintf(buf, sizeof buf, "FR 0, 1, 0, 0, %.6f, 0", freq);
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
