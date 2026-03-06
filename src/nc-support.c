/******************************************************************************
 * nc-support.c
 *
 * Converts an OpenNEC deck_t to a cocoaNEC .nc source file.
 *
 * write_deck_nc() walks the deck and emits a single model(){} block:
 *   SY cards      → real variable declarations + assignments
 *   GW cards      → wire() calls; wires referenced by EX/LD get element vars
 *   EX cards      → voltageFeed() or currentFeed()
 *   LD cards      → impedanceLoad()
 *   FR cards      → setFrequency() + addFrequency()
 *   GN card       → the matching ground function
 *   RP cards      → azimuth/elevation plot calls for single-cut patterns
 *
 * Cards with no NC equivalent (GE, EN, GS, GH, GM, TL, NT, …) are skipped.
 *
 * read_deck_nc() is not yet implemented.
 *
 * Design note: i[] and f[] fields in card_t are 1-based (index 0 unused).
 * These are populated by OpenNEC's main input parser; write_deck_nc relies
 * on them being valid.  SY cards carry their name=expression in card_str,
 * not in i[]/f[], so they are always parsed from the string.
 *****************************************************************************/

#include "nc-support.h"
#include "deck.h"
#include "misc.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* Maximum GW tag number we track for element-variable assignment. */
#define NC_MAX_TAGS  1024

/* -------------------------------------------------------------------------
 * Helpers
 * ---------------------------------------------------------------------- */

/*
 * nc_fmt - format a double for NC output.
 * Uses %g so trailing zeros are stripped; adds ".0" when the result has no
 * decimal point or exponent, matching the cocoaNEC convention.
 */
static void nc_fmt(char *buf, size_t sz, double v)
{
    if (v == 0.0) v = fabs(v);   /* suppress -0.0 */
    snprintf(buf, sz, "%.8g", v);
    /* if no '.' or 'e', append ".0" so readers see it as a float */
    if (!strchr(buf, '.') && !strchr(buf, 'e') && !strchr(buf, 'E') &&
        !strchr(buf, 'n') /* nan */ && !strchr(buf, 'i') /* inf */) {
        size_t len = strlen(buf);
        if (len + 2 < sz) { buf[len] = '.'; buf[len+1] = '0'; buf[len+2] = '\0'; }
    }
}

/*
 * parse_sy_card_all - extract all name=expression assignments from a SY card.
 * A single SY card may carry multiple assignments separated by commas:
 *   SY Z=len*cos(ang/2), X=len*sin(ang/2)
 * Each assignment is appended starting at sy_names[n_sy] / sy_vals[n_sy].
 * Returns the number of new entries added (0 on parse failure).
 */
static int parse_sy_card_all(const card_t *c,
                             char sy_names[][64], char sy_vals[][256],
                             int n_sy, int max_sy)
{
    if (!c || !c->card_str) return 0;
    const char *p = c->card_str;
    /* skip "SY" token */
    while (*p && !isspace((unsigned char)*p)) p++;
    while (*p &&  isspace((unsigned char)*p)) p++;
    if (!*p) return 0;

    int added = 0;
    while (*p && (n_sy + added) < max_sy) {
        const char *eq = strchr(p, '=');
        if (!eq) break;

        /* name: from p to eq, trimmed */
        const char *ns = p;
        while (ns < eq && isspace((unsigned char)*ns)) ns++;
        const char *ne = eq;
        while (ne > ns && isspace((unsigned char)(ne[-1]))) ne--;
        size_t nlen = (size_t)(ne - ns);
        if (nlen == 0 || nlen >= 64) break;

        /* value: after '=', up to next ", identifier =" or end-of-string */
        const char *vs = eq + 1;
        while (*vs && isspace((unsigned char)*vs)) vs++;

        /* Scan for the next ", name=" boundary. */
        const char *end = vs;
        const char *next_p = NULL;
        const char *scan = vs;
        while (*scan) {
            if (*scan == ',') {
                const char *a = scan + 1;
                while (*a && isspace((unsigned char)*a)) a++;
                /* check if next token is an identifier followed by '=' */
                const char *id = a;
                while (*id && (isalnum((unsigned char)*id) || *id == '_')) id++;
                if (id > a) {
                    const char *a2 = id;
                    while (*a2 && isspace((unsigned char)*a2)) a2++;
                    if (*a2 == '=') {
                        end    = scan;   /* value ends at the comma */
                        next_p = a;      /* next assignment starts here */
                        break;
                    }
                }
            }
            scan++;
        }
        if (!next_p)
            end = vs + strlen(vs);

        /* trim trailing whitespace from value */
        size_t vlen = (size_t)(end - vs);
        while (vlen > 0 && (vs[vlen-1] == '\n' || vs[vlen-1] == '\r' ||
                            isspace((unsigned char)vs[vlen-1]))) vlen--;
        if (vlen == 0 || vlen >= 256) break;

        memcpy(sy_names[n_sy + added], ns, nlen);
        sy_names[n_sy + added][nlen] = '\0';
        memcpy(sy_vals [n_sy + added], vs, vlen);
        sy_vals [n_sy + added][vlen] = '\0';
        added++;

        if (!next_p) break;
        p = next_p;
    }
    return added;
}

/* -------------------------------------------------------------------------
 * write_deck_nc
 * ---------------------------------------------------------------------- */

int write_deck_nc(const deck_t *deck, FILE *fp)
{
    if (!deck || !fp) return -1;

    /* ------------------------------------------------------------------
     * Pass 1: collect metadata
     * ------------------------------------------------------------------ */

    /* Title: first non-empty CM card comment */
    const char *title = "antenna";
    for (int i = 0; i < deck->num_cards; i++) {
        const card_t *c = &deck->cards[i];
        if (strcmp(c->card_code, "CM") != 0) continue;
        if (!c->comment) continue;
        const char *t = c->comment;
        while (*t && isspace((unsigned char)*t)) t++;
        if (*t) { title = t; break; }
    }

    /* SY symbols (up to 64): name[] and expression[] in deck order */
    char sy_names[64][64];
    char sy_vals [64][256];
    int  n_sy = 0;
    for (int i = 0; i < deck->num_cards && n_sy < 64; i++) {
        const card_t *c = &deck->cards[i];
        if (strcmp(c->card_code, "SY") != 0) continue;
        if (card_is_commented_out(c)) continue;
        n_sy += parse_sy_card_all(c, sy_names, sy_vals, n_sy, 64);
    }

    /* Identify which GW tags are used by EX (excitation) or LD (load).
     * ex_type_for_tag[tag]: 0=none, 1=voltage, 2=current
     * ld_for_tag[tag]:      1 if an LD card references this tag */
    int ex_type_for_tag[NC_MAX_TAGS];
    int ld_for_tag[NC_MAX_TAGS];
    memset(ex_type_for_tag, 0, sizeof(ex_type_for_tag));
    memset(ld_for_tag,      0, sizeof(ld_for_tag));

    for (int i = 0; i < deck->num_cards; i++) {
        const card_t *c = &deck->cards[i];
        if (card_is_commented_out(c)) continue;
        if (strcmp(c->card_code, "EX") == 0) {
            int t = c->i[2];   /* I2: wire tag */
            if (t > 0 && t < NC_MAX_TAGS)
                ex_type_for_tag[t] = (c->i[1] == 1) ? 2 : 1; /* I1: 1=current */
        } else if (strcmp(c->card_code, "LD") == 0) {
            int t = c->i[2];   /* I2: wire tag */
            if (t > 0 && t < NC_MAX_TAGS)
                ld_for_tag[t] = 1;
        }
    }

    /* Assign element variable names (e1, e2, …) to referenced GW tags,
     * in the order the wires appear in the deck. */
    char  elem_buf[NC_MAX_TAGS][16];
    char *elem_name[NC_MAX_TAGS];
    memset(elem_name, 0, sizeof(elem_name));
    int elem_count = 0;
    for (int i = 0; i < deck->num_cards; i++) {
        const card_t *c = &deck->cards[i];
        if (strcmp(c->card_code, "GW") != 0) continue;
        if (card_is_commented_out(c)) continue;
        int tag = c->i[1];
        if (tag <= 0 || tag >= NC_MAX_TAGS) continue;
        if ((ex_type_for_tag[tag] || ld_for_tag[tag]) && !elem_name[tag]) {
            elem_count++;
            snprintf(elem_buf[tag], 16, "e%d", elem_count);
            elem_name[tag] = elem_buf[tag];
        }
    }

    /* ------------------------------------------------------------------
     * Pass 2: emit
     * ------------------------------------------------------------------ */

    fprintf(fp, "// Generated by OpenNEC\n\n");
    fprintf(fp, "model ( \"%s\" )\n{\n", title);

    /* ---- Declarations ------------------------------------------------ */

    if (n_sy > 0) {
        fprintf(fp, "\treal");
        for (int i = 0; i < n_sy; i++)
            fprintf(fp, "%s %s", (i == 0) ? "" : ",", sy_names[i]);
        fprintf(fp, " ;\n");
    }

    if (elem_count > 0) {
        fprintf(fp, "\telement");
        bool first = true;
        for (int tag = 1; tag < NC_MAX_TAGS; tag++) {
            if (!elem_name[tag]) continue;
            fprintf(fp, "%s %s", first ? "" : ",", elem_name[tag]);
            first = false;
        }
        fprintf(fp, " ;\n");
    }

    if (n_sy > 0 || elem_count > 0)
        fprintf(fp, "\n");

    /* ---- SY symbol assignments --------------------------------------- */

    if (n_sy > 0) {
        for (int i = 0; i < n_sy; i++)
            fprintf(fp, "\t%s = %s ;\n", sy_names[i], sy_vals[i]);
        fprintf(fp, "\n");
    }

    /* ---- GW wire calls ----------------------------------------------- */

    char sx1[32], sy1[32], sz1[32], sx2[32], sy2[32], sz2[32], sr[32];
    for (int i = 0; i < deck->num_cards; i++) {
        const card_t *c = &deck->cards[i];
        if (strcmp(c->card_code, "GW") != 0) continue;
        if (card_is_commented_out(c)) continue;

        int tag  = c->i[1];     /* I1: tag   */
        int segs = c->i[2];     /* I2: segs  */
        nc_fmt(sx1, sizeof sx1, c->f[1]);   /* F1: x1    */
        nc_fmt(sy1, sizeof sy1, c->f[2]);   /* F2: y1    */
        nc_fmt(sz1, sizeof sz1, c->f[3]);   /* F3: z1    */
        nc_fmt(sx2, sizeof sx2, c->f[4]);   /* F4: x2    */
        nc_fmt(sy2, sizeof sy2, c->f[5]);   /* F5: y2    */
        nc_fmt(sz2, sizeof sz2, c->f[6]);   /* F6: z2    */
        nc_fmt(sr,  sizeof sr,  c->f[7]);   /* F7: radius */

        if (tag > 0 && tag < NC_MAX_TAGS && elem_name[tag]) {
            fprintf(fp, "\t%s = wire( %s, %s, %s, %s, %s, %s, %s, %d ) ;\n",
                    elem_name[tag],
                    sx1, sy1, sz1, sx2, sy2, sz2, sr, segs);
        } else {
            fprintf(fp, "\twire( %s, %s, %s, %s, %s, %s, %s, %d ) ;\n",
                    sx1, sy1, sz1, sx2, sy2, sz2, sr, segs);
        }
    }

    /* ---- EX excitation calls ----------------------------------------- */

    bool have_ex = false;
    for (int i = 0; i < deck->num_cards; i++) {
        const card_t *c = &deck->cards[i];
        if (strcmp(c->card_code, "EX") != 0) continue;
        if (card_is_commented_out(c)) continue;
        if (!have_ex) { fprintf(fp, "\n"); have_ex = true; }

        int         ex_type = c->i[1];  /* I1: 0=voltage, 1=current */
        int         tag     = c->i[2];  /* I2: wire tag              */
        const char *func    = (ex_type == 1) ? "currentFeed" : "voltageFeed";

        char src[32];
        if (tag > 0 && tag < NC_MAX_TAGS && elem_name[tag])
            snprintf(src, sizeof src, "%s", elem_name[tag]);
        else
            snprintf(src, sizeof src, "e%d", tag);

        char sre[32], sim_[32];
        nc_fmt(sre,  sizeof sre,  c->f[1]);   /* F1: real part */
        nc_fmt(sim_, sizeof sim_, c->f[2]);   /* F2: imag part */

        fprintf(fp, "\t%s( %s, %s, %s ) ;\n", func, src, sre, sim_);
    }

    /* ---- LD load calls ----------------------------------------------- */

    bool have_ld = false;
    for (int i = 0; i < deck->num_cards; i++) {
        const card_t *c = &deck->cards[i];
        if (strcmp(c->card_code, "LD") != 0) continue;
        if (card_is_commented_out(c)) continue;
        if (!have_ld) { fprintf(fp, "\n"); have_ld = true; }

        int tag = c->i[2];   /* I2: wire tag       */
        /* LD type 0: F1=R(Ω), F2=L(H), F3=C(F)
         * LD type 4: F1=R/m,  F2=X/m  (distributed)
         * Emit as impedanceLoad(elem, R, X); X approximated as 0 for type 0. */
        double R = c->f[1];
        double X = (c->i[1] == 4) ? c->f[2] : 0.0;

        char src[32];
        if (tag > 0 && tag < NC_MAX_TAGS && elem_name[tag])
            snprintf(src, sizeof src, "%s", elem_name[tag]);
        else
            snprintf(src, sizeof src, "e%d", tag);

        char sR[32], sX[32];
        nc_fmt(sR, sizeof sR, R);
        nc_fmt(sX, sizeof sX, X);
        fprintf(fp, "\timpedanceLoad( %s, %s, %s ) ;\n", src, sR, sX);
    }

    /* ---- FR frequency calls ------------------------------------------ */

    bool first_freq = true;
    for (int i = 0; i < deck->num_cards; i++) {
        const card_t *c = &deck->cards[i];
        if (strcmp(c->card_code, "FR") != 0) continue;
        if (card_is_commented_out(c)) continue;
        if (first_freq) fprintf(fp, "\n");

        int    n_freqs  = c->i[2];         /* I2: NFRQ, number of steps     */
        double f_start  = c->f[1];         /* F1: start frequency (MHz)     */
        double f_step   = c->f[2];         /* F2: step (MHz) for IFRQ=0     */
        /* IFRQ=1 (multiplicative) is uncommon; treat as linear for NC output. */
        if (n_freqs < 1) n_freqs = 1;

        char sf[32];
        for (int fi = 0; fi < n_freqs; fi++) {
            nc_fmt(sf, sizeof sf, f_start + fi * f_step);
            if (first_freq) {
                fprintf(fp, "\tsetFrequency( %s ) ;\n", sf);
                first_freq = false;
            } else {
                fprintf(fp, "\taddFrequency( %s ) ;\n", sf);
            }
        }
    }

    /* ---- GN ground call ---------------------------------------------- */

    fprintf(fp, "\n");
    bool have_gn = false;
    for (int i = 0; i < deck->num_cards; i++) {
        const card_t *c = &deck->cards[i];
        if (strcmp(c->card_code, "GN") != 0) continue;
        if (card_is_commented_out(c)) continue;
        have_gn = true;

        int    gn_type  = c->i[1];  /* I1: ITYP; -1=freespace, 1=perfect, 2=S-N */
        double gn_sigma = c->f[2];  /* F2: conductivity, S/m                     */

        if (gn_type == -1) {
            fprintf(fp, "\tfreespace() ;\n");
        } else if (gn_type == 1) {
            fprintf(fp, "\tperfectGround() ;\n");
        } else if (gn_type == 2) {
            fprintf(fp, "\tuseSommerfeldGround( 1 ) ;\n");
            /* also emit a ground quality function based on conductivity */
            if (gn_sigma >= 1.0)
                fprintf(fp, "\tsaltWaterGround() ;\n");
            else if (gn_sigma >= 0.020)
                fprintf(fp, "\tgoodGround() ;\n");
            else if (gn_sigma >= 0.005)
                fprintf(fp, "\taverageGround() ;\n");
            else
                fprintf(fp, "\taverageGround() ; // poor ground (sigma=%.4g S/m)\n", gn_sigma);
        } else {
            /* Real ground: map conductivity to the closest NC helper.
             * Approximate thresholds (S/m):
             *   salt water  ≥ 1.0     (~5 S/m typical)
             *   good        ≥ 0.020   (agricultural, 0.02–0.03)
             *   average     ≥ 0.005   (median soil, 0.005)
             *   poor        <  0.005  (rocky/sandy) */
            if (gn_sigma >= 1.0)
                fprintf(fp, "\tsaltWaterGround() ;\n");
            else if (gn_sigma >= 0.020)
                fprintf(fp, "\tgoodGround() ;\n");
            else if (gn_sigma >= 0.005)
                fprintf(fp, "\taverageGround() ;\n");
            else
                fprintf(fp, "\taverageGround() ; // poor ground (sigma=%.4g S/m)\n", gn_sigma);
        }
        break;  /* only one GN card is meaningful */
    }
    if (!have_gn)
        fprintf(fp, "\tfreespace() ;\n");

    /* ---- RP radiation pattern calls ---------------------------------- */

    for (int i = 0; i < deck->num_cards; i++) {
        const card_t *c = &deck->cards[i];
        if (strcmp(c->card_code, "RP") != 0) continue;
        if (card_is_commented_out(c)) continue;

        int    n_theta = c->i[2];   /* I2: NTH                */
        int    n_phi   = c->i[3];   /* I3: NPH                */
        double th0     = c->f[1];   /* F1: TH0 (zenith, deg)  */
        double ph0     = c->f[2];   /* F2: PH0 (azimuth, deg) */

        char sf[32];
        if (n_theta == 1) {
            /* Fixed zenith angle → azimuth sweep.
             * NC elevation = 90° - NEC zenith angle. */
            nc_fmt(sf, sizeof sf, 90.0 - th0);
            fprintf(fp, "\tazimuthPlotForElevationAngle( %s ) ;\n", sf);
        } else if (n_phi == 1) {
            /* Fixed azimuth angle → elevation sweep. */
            nc_fmt(sf, sizeof sf, ph0);
            fprintf(fp, "\televationPlotForAzimuthAngle( %s ) ;\n", sf);
        }
        /* Full-sphere RP has no single NC equivalent; skip silently. */
    }

    fprintf(fp, "}\n");
    return 0;
}

/* -------------------------------------------------------------------------
 * read_deck_nc  (not yet implemented)
 * ---------------------------------------------------------------------- */

int read_deck_nc(deck_t *deck, FILE *fp)
{
    (void)deck;
    (void)fp;
    return -1;
}
