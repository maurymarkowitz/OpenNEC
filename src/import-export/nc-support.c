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
#include <stdint.h>

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
 * nc_fmt_unit - format a double for geometry output using unit constants.
 * Prefer expressing lengths as a multiple of common measurement constants
 * (m, cm, mm, ft, in, mil) rather than a raw numeric multiplier. Prefer
 * exact integer multiples when possible, otherwise pick the unit that
 * yields the shortest decimal multiplier (e.g. 1.5*ft).
 * Used as a fallback when a card field has no associated formula expression.
 */
static void nc_fmt_unit(char *buf, size_t sz, double v)
{
    const double tol_int = 1e-9;

    if (v == 0.0) {
        snprintf(buf, sz, "0.0");
        return;
    }

    struct unit { const char *name; double val; } units[] = {
        {"m", 1.0},
        {"cm", 0.01},
        {"mm", 0.001},
        {"ft", 0.3048},
        {"in", 0.0254},
        {"mil", 0.0000254}
    };
    const int nunits = sizeof(units) / sizeof(units[0]);

    double abs_v = fabs(v);
    for (int i = 0; i < nunits; ++i) {
        double n = abs_v / units[i].val;
        double rn = round(n);
        if (fabs(n - rn) < tol_int) {
            if (rn == 0.0) {
                snprintf(buf, sz, "0.0");
            } else if (v < 0) {
                if (strcmp(units[i].name, "ft") == 0)
                    snprintf(buf, sz, "-%d'", (int)rn);
                else if (strcmp(units[i].name, "in") == 0)
                    snprintf(buf, sz, "-%d\"", (int)rn);
                else
                    snprintf(buf, sz, "-%d%s", (int)rn, units[i].name);
            } else {
                if (strcmp(units[i].name, "ft") == 0)
                    snprintf(buf, sz, "%d'", (int)rn);
                else if (strcmp(units[i].name, "in") == 0)
                    snprintf(buf, sz, "%d\"", (int)rn);
                else
                    snprintf(buf, sz, "%d%s", (int)rn, units[i].name);
            }
            return;
        }
    }

    int best = 0;
    char bests[64];
    size_t bestlen = SIZE_MAX;
    for (int i = 0; i < nunits; ++i) {
        double n = abs_v / units[i].val;
        char tmp[64];
        snprintf(tmp, sizeof tmp, "%.6g", n);
        size_t len = strlen(tmp);
        if (len < bestlen) {
            bestlen = len;
            best = i;
            strncpy(bests, tmp, sizeof bests - 1);
            bests[sizeof bests - 1] = '\0';
        }
    }
    if (v < 0)
        if (strcmp(units[best].name, "ft") == 0)
            snprintf(buf, sz, "-%s'", bests);
        else if (strcmp(units[best].name, "in") == 0)
            snprintf(buf, sz, "-%s\"", bests);
        else
            snprintf(buf, sz, "-%s%s", bests, units[best].name);
    else
        if (strcmp(units[best].name, "ft") == 0)
            snprintf(buf, sz, "%s'", bests);
        else if (strcmp(units[best].name, "in") == 0)
            snprintf(buf, sz, "%s\"", bests);
        else
            snprintf(buf, sz, "%s%s", bests, units[best].name);
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

/**
 * @copydoc write_deck_nc
 */
int write_deck_nc(const deck_t *deck, FILE *fp)
{
    if (!deck || !fp) return -1;

    /* ------------------------------------------------------------------
     * Pass 1: collect metadata
     * ------------------------------------------------------------------ */

    /* Title: first non-empty CM card comment */
    const char *title = "(no title)";
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
        for (int i = 0; i < n_sy; i++) {
            /* Always emit the original formula expression as-is.
             * The NC reader handles ', ", and unit constants natively. */
            fprintf(fp, "\t%s = %s ;\n", sy_names[i], sy_vals[i]);
        }
        fprintf(fp, "\n");
    }

    /* ---- GW wire calls ----------------------------------------------- */

    char sx1[128], sy1[128], sz1[128], sx2[128], sy2[128], sz2[128], sr[128];
    for (int i = 0; i < deck->num_cards; i++) {
        const card_t *c = &deck->cards[i];
        if (strcmp(c->card_code, "GW") != 0) continue;
        if (card_is_commented_out(c)) continue;

        /* For each float field, prefer inline formula; fall back to numeric. */
#define GWF(buf, sz, fld) do { \
            char _key[] = {'F', '0'+(fld), '\0'}; \
            const char *_f = c->flt_form_inline[(fld)] ? lookup_formula(c, _key) : NULL; \
            if (_f) strncpy((buf), _f, (sz)-1), (buf)[(sz)-1] = '\0'; \
            else nc_fmt_unit((buf), (sz), c->f[(fld)]); \
        } while (0)

        int tag  = c->i[1];     /* I1: tag   */
        int segs = c->i[2];     /* I2: segs  */
        GWF(sx1, sizeof sx1, 1);   /* F1: x1    */
        GWF(sy1, sizeof sy1, 2);   /* F2: y1    */
        GWF(sz1, sizeof sz1, 3);   /* F3: z1    */
        GWF(sx2, sizeof sx2, 4);   /* F4: x2    */
        GWF(sy2, sizeof sy2, 5);   /* F5: y2    */
        GWF(sz2, sizeof sz2, 6);   /* F6: z2    */
        GWF(sr,  sizeof sr,  7);   /* F7: radius */
#undef GWF

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

/* =========================================================================
 * read_deck_nc — NC → NEC-2 deck importer
 *
 * Parses the cocoaNEC .nc program format and populates `deck` with the
 * equivalent NEC-2 cards (CM/CE, SY, GW, GE, EX, LD, FR, GN, RP, EN).
 *
 * Supported constructs:
 *   model("name") { ... }    → CM + CE + body
 *   real / int declarations   → SY cards
 *   element declarations      → element variable tracking (no SY)
 *   name = expr ;             → SY name=expr
 *   name = wire(...) ;        → GW + element binding
 *   wire(...) / line(...)     → GW
 *   voltageFeed / currentFeed → EX
 *   impedanceLoad             → LD
 *   setFrequency / addFrequency / frequencySweep → FR
 *   freespace / perfectGround / averageGround /
 *     goodGround / saltWaterGround / useSommerfeldGround / ground()  → GN
 *   azimuthPlotForElevationAngle / elevationPlotForAzimuthAngle      → RP
 *   control() { ... }         → skipped
 *   all other unknown calls   → skipped
 *
 * Unit-suffix expansion (performed during expression parsing):
 *   N"      → N*0.0254   (inches → metres)
 *   N'      → N*0.3048   (feet → metres)
 *   #N      → AWG gauge N wire radius in metres
 *   sind/cosd/tand/atand  → sin/cos/tan/atan (tinyexpr already degree-mode)
 *
 * The built-in constant `c` (speed of light, ~299.792458 MHz·m) is injected
 * as a SY card when referenced in any expression.
 * ======================================================================= */

/* -------------------------------------------------------------------------
 * Internal parser helpers
 * ---------------------------------------------------------------------- */

/** Skip whitespace and // line comments; advance *pp. */
static void nc_skip(const char **pp)
{
    for (;;) {
        while (**pp && isspace((unsigned char)**pp)) (*pp)++;
        if ((*pp)[0] == '/' && (*pp)[1] == '/') {
            while (**pp && **pp != '\n') (*pp)++;
            continue;
        }
        break;
    }
}

/**
 * Like nc_skip but accumulates // comment lines into cbuf (newline-delimited).
 * Used when scanning before model() to collect header comments.
 */
static void nc_skip_collect(const char **pp, char *cbuf, int csz, int *cn)
{
    for (;;) {
        while (**pp && isspace((unsigned char)**pp)) (*pp)++;
        if ((*pp)[0] == '/' && (*pp)[1] == '/') {
            (*pp) += 2;
            while (**pp == ' ' || **pp == '\t') (*pp)++; /* trim leading space */
            while (**pp && **pp != '\n') {
                if (*cn < csz - 1) cbuf[(*cn)++] = **pp;
                (*pp)++;
            }
            cbuf[*cn] = '\0';
            if (*cn < csz - 1) cbuf[(*cn)++] = '\n'; /* line separator */
            cbuf[*cn] = '\0';
            continue;
        }
        break;
    }
}

/** Read an identifier into buf[sz]; advance *pp.  Returns chars written. */
static int nc_ident(const char **pp, char *buf, int sz)
{
    nc_skip(pp);
    int n = 0;
    while (**pp && (isalnum((unsigned char)**pp) || **pp == '_')) {
        if (n < sz - 1) buf[n++] = **pp;
        (*pp)++;
    }
    buf[n] = '\0';
    return n;
}

/**
 * Read a quoted string (without quotes) into buf[sz]; advance *pp past
 * the closing '"'.  Returns chars written.
 */
static int nc_qstring(const char **pp, char *buf, int sz)
{
    nc_skip(pp);
    if (**pp != '"') return 0;
    (*pp)++;
    int n = 0;
    while (**pp && **pp != '"') {
        if (n < sz - 1) buf[n++] = **pp;
        (*pp)++;
    }
    if (**pp == '"') (*pp)++;
    buf[n] = '\0';
    return n;
}

/*
 * nc_is_str_quote - true when the '"' at *p is a string delimiter.
 * A '"' that immediately follows a digit or ')' is an NC inches marker
 * (e.g. 48" = 48 inches) and must NOT toggle string mode.
 */
static bool nc_is_str_quote(char prev)
{
    return !(isdigit((unsigned char)prev) || prev == ')');
}

/**
 * Extract the content of a balanced (...) block starting at '(' into
 * buf[sz]; advance *pp past the closing ')'.  Returns 1 on success.
 */
static int nc_parens(const char **pp, char *buf, int sz)
{
    nc_skip(pp);
    if (**pp != '(') return 0;
    (*pp)++;
    int depth = 1, n = 0;
    bool in_str = false;
    char prev = '(';
    while (**pp && depth > 0) {
        char c = **pp;
        if (c == '"' && nc_is_str_quote(prev)) in_str = !in_str;
        if (!in_str) {
            if (c == '(')      depth++;
            else if (c == ')') { if (--depth == 0) { (*pp)++; break; } }
        }
        if (n < sz - 1) buf[n++] = c;
        prev = c;
        (*pp)++;
    }
    buf[n] = '\0';
    return 1;
}

/**
 * If the rest of the current line (before any newline) contains a // comment,
 * capture the trimmed text into buf[sz] and return 1.  Does NOT consume any
 * characters — the caller's position is unchanged.  Returns 0 if no comment.
 */
static int nc_eol_comment(const char *p, char *buf, int sz)
{
    while (*p == ' ' || *p == '\t') p++;
    if (p[0] != '/' || p[1] != '/') return 0;
    p += 2;
    while (*p == ' ' || *p == '\t') p++;   /* trim leading space */
    int n = 0;
    while (*p && *p != '\n' && n < sz - 1) buf[n++] = *p++;
    while (n > 0 && (buf[n-1] == ' ' || buf[n-1] == '\t')) n--;  /* trim tail */
    buf[n] = '\0';
    return n > 0 ? 1 : 0;
}

/**
 * Retroactively append " ! comment" to the last card's card_str.
 */
static void nc_card_append_comment(deck_t *deck, const char *cmt)
{
    if (!deck || deck->num_cards == 0 || !cmt || !cmt[0]) return;
    card_t *card = &deck->cards[deck->num_cards - 1];
    if (!card->card_str) return;
    size_t old_len = strlen(card->card_str);
    size_t add_len = 4 + strlen(cmt);   /* " ! " + cmt + NUL */
    char *ns = realloc(card->card_str, old_len + add_len);
    if (!ns) return;
    card->card_str = ns;
    snprintf(card->card_str + old_len, add_len, " ! %s", cmt);
}

/**
 * Skip a balanced { ... } block.  *pp should point at the opening '{'.
 */
static void nc_skip_block(const char **pp)
{
    nc_skip(pp);
    if (**pp != '{') return;
    (*pp)++;
    int depth = 1;
    bool in_str = false;
    char prev = '{';
    while (**pp && depth > 0) {
        char c = **pp;
        if (c == '"' && nc_is_str_quote(prev)) in_str = !in_str;
        if (!in_str) {
            if (c == '{') depth++;
            else if (c == '}') depth--;
        }
        prev = c;
        (*pp)++;
    }
}

/**
 * Read characters into buf[sz] until `delim` is found at nesting depth 0.
 * Does NOT consume the delimiter.  Trailing whitespace is stripped.
 * Returns 0 if nothing was read.
 */
static int nc_read_to(const char **pp, char *buf, int sz, char delim)
{
    int n = 0, depth = 0;
    bool in_str = false;
    char prev = '\0';
    while (**pp && !(depth == 0 && **pp == delim && !in_str)) {
        char c = **pp;
        if (c == '"' && nc_is_str_quote(prev)) in_str = !in_str;
        if (!in_str) {
            if (c == '(' || c == '[') depth++;
            else if (c == ')' || c == ']') depth--;
        }
        if (n < sz - 1) buf[n++] = c;
        prev = c;
        (*pp)++;
    }
    while (n > 0 && isspace((unsigned char)buf[n-1])) n--;
    buf[n] = '\0';
    return n;
}

/**
 * Skip past a semicolon (if present).
 */
static void nc_semi(const char **pp)
{
    nc_skip(pp);
    if (**pp == ';') (*pp)++;
}

/**
 * Split a balanced-comma-separated argument string into out[0..max-1].
 * Respects nested parentheses.  Returns argument count.
 */
static int nc_split(const char *s, char out[][256], int max)
{
    int n = 0, depth = 0, di = 0;
    bool in_str = false;
    char prev = '\0';
    char cur[512];
    for (; *s && n < max; s++) {
        char c = *s;
        if (c == '"' && nc_is_str_quote(prev)) in_str = !in_str;
        if (!in_str) {
            if (c == '(') depth++;
            else if (c == ')') depth--;
        }
        if (!in_str && depth == 0 && c == ',') {
            while (di > 0 && isspace((unsigned char)cur[di-1])) di--;
            cur[di] = '\0';
            const char *t = cur; while (*t && isspace((unsigned char)*t)) t++;
            strncpy(out[n++], t, sizeof(out[0]) - 1); out[n-1][sizeof(out[0]) - 1] = '\0';
            di = 0;
        } else {
            if (di < 510) cur[di++] = c;
        }
        prev = c;
    }
    /* last (or only) arg */
    while (di > 0 && isspace((unsigned char)cur[di-1])) di--;
    cur[di] = '\0';
    const char *t = cur; while (*t && isspace((unsigned char)*t)) t++;
    if (*t && n < max) { strncpy(out[n++], t, sizeof(out[0]) - 1); out[n-1][sizeof(out[0]) - 1] = '\0'; }
    return n;
}

/* -------------------------------------------------------------------------
 * NC reader state definitions
 * ---------------------------------------------------------------------- */

#define NCR_MAX_VARS   64
#define NCR_MAX_GW    512
#define NCR_MAX_POST  256

typedef struct {
    const char *p;            /* current scan position            */

    /* variable registries */
    char  sy_vars[NCR_MAX_VARS][64];  /* real/int variable names    */
    bool  sy_assigned[NCR_MAX_VARS];  /* has this variable received an assignment */
    char  sy_values[NCR_MAX_VARS][256]; /* assigned value expression */
    int   n_syv;
    char  el_vars[NCR_MAX_VARS][64];  /* element variable names     */
    int   el_tags[NCR_MAX_VARS];      /* assigned GW tag (0=none)   */
    char  el_segs[NCR_MAX_VARS][64];  /* segment-count expression   */
    int   n_elv;
    int   next_tag;           /* next GW tag to assign            */

    /* variables used in model() expressions */
    char  used_vars[NCR_MAX_VARS][64]; /* identifiers referenced in formulas */
    int   n_used_vars;

    /* emitted-card flags */
    bool  need_c;             /* inject SY c=299.792458?          */
    bool  have_gn;            /* any GN call seen?                */
    bool  have_fr;            /* any FR card emitted?             */
    int   sommerfeld;         /* useSommerfeldGround arg          */
    bool  first_freq;         /* setFrequency not yet emitted     */
} ncr_t;

/* -------------------------------------------------------------------------
 * Control-section variable detection
 *
 * When reading .nc files, we need to warn if a variable used in the model()
 * section is only assigned in the control() section, as conversion to flat
 * NEC format will lose the control-section assignments.
 * ---------------------------------------------------------------------- */

/**
 * Extract all identifiers from an expression string into the given array.
 * Returns count of unique identifiers found.
 */
static int nc_extract_identifiers(const char *expr, char idents[][64], int max)
{
    int count = 0;
    const char *p = expr;
    
    while (*p) {
        /* Skip non-identifier characters */
        if (!isalpha((unsigned char)*p) && *p != '_') {
            p++;
            continue;
        }
        
        /* Read identifier */
        char ident[64];
        int n = 0;
        while (*p && (isalnum((unsigned char)*p) || *p == '_')) {
            if (n < 63) ident[n++] = *p;
            p++;
        }
        ident[n] = '\0';
        
        /* Skip built-in functions and constants */
        if (strcmp(ident, "sin") == 0 || strcmp(ident, "cos") == 0 ||
            strcmp(ident, "tan") == 0 || strcmp(ident, "sqrt") == 0 ||
            strcmp(ident, "abs") == 0 || strcmp(ident, "log") == 0 ||
            strcmp(ident, "exp") == 0 || strcmp(ident, "asin") == 0 ||
            strcmp(ident, "acos") == 0 || strcmp(ident, "atan") == 0 ||
            strcmp(ident, "sinh") == 0 || strcmp(ident, "cosh") == 0 ||
            strcmp(ident, "tanh") == 0 || strcmp(ident, "sind") == 0 ||
            strcmp(ident, "cosd") == 0 || strcmp(ident, "tand") == 0 ||
            strcmp(ident, "atand") == 0 || strcmp(ident, "c") == 0 ||
            strcmp(ident, "pi") == 0 || strcmp(ident, "e") == 0) {
            continue;
        }
        
        /* Check if already in list */
        bool already = false;
        for (int i = 0; i < count; i++) {
            if (strcmp(idents[i], ident) == 0) { already = true; break; }
        }
        if (!already && count < max) {
            strncpy(idents[count++], ident, sizeof(idents[0]) - 1);
            idents[count-1][sizeof(idents[0]) - 1] = '\0';
        }
    }
    
    return count;
}

/**
 * Scan the control() block text (starting at first 'control') and extract
 * all variable names that appear to be assigned (on LHS of '=').
 * Returns count of variables found.
 */
static int nc_scan_control_vars(const char *text, char vars[][64], int max)
{
    int count = 0;
    const char *p = text;
    
    /* Find 'control' keyword at statement level */
    while (*p) {
        while (*p && isspace((unsigned char)*p)) p++;
        if (p[0] == 'c' && p[1] == 'o' && p[2] == 'n' && p[3] == 't' &&
            p[4] == 'r' && p[5] == 'o' && p[6] == 'l' &&
            (isspace((unsigned char)p[7]) || p[7] == '(')) {
            p += 7;
            break;
        }
        /* Skip to next line */
        while (*p && *p != '\n') p++;
        if (*p == '\n') p++;
    }
    
    if (!*p) return 0;  /* control() not found */
    
    /* Skip to opening brace */
    while (*p && *p != '{') p++;
    if (*p != '{') return 0;
    p++;
    
    /* Scan inside control() block */
    int depth = 1;
    while (*p && depth > 0) {
        /* Skip whitespace and comments */
        while (*p && isspace((unsigned char)*p)) p++;
        if (p[0] == '/' && p[1] == '/') {
            while (*p && *p != '\n') p++;
            continue;
        }
        
        if (*p == '{') { depth++; p++; continue; }
        if (*p == '}') { depth--; p++; continue; }
        if (!*p) break;
        
        /* Try to read an identifier */
        if (isalpha((unsigned char)*p) || *p == '_') {
            char ident[64];
            int n = 0;
            while (*p && (isalnum((unsigned char)*p) || *p == '_')) {
                if (n < 63) ident[n++] = *p;
                p++;
            }
            ident[n] = '\0';
            
            /* Skip whitespace after identifier */
            while (*p && isspace((unsigned char)*p)) p++;
            
            /* If next non-whitespace is '=', this is an assignment */
            if (*p == '=') {
                /* Check if not already in list */
                bool already = false;
                for (int i = 0; i < count; i++) {
                    if (strcmp(vars[i], ident) == 0) { already = true; break; }
                }
                if (!already && count < max) {
                    memcpy(vars[count++], ident, 63);
                    vars[count-1][63] = '\0';
                }
            }
        } else {
            p++;
        }
    }
    
    return count;
}

/**
 * Helper: add identifiers from an expression to the used_vars list
 */
static void nc_track_expr_vars(ncr_t *s, const char *expr)
{
    if (!expr || !*expr) return;
    
    char expr_idents[32][64];
    int n_idents = nc_extract_identifiers(expr, expr_idents, 32);
    for (int i = 0; i < n_idents; i++) {
        bool already = false;
        for (int j = 0; j < s->n_used_vars; j++) {
            if (strcmp(s->used_vars[j], expr_idents[i]) == 0) {
                already = true;
                break;
            }
        }
        if (!already && s->n_used_vars < NCR_MAX_VARS) {
            strncpy(s->used_vars[s->n_used_vars++], expr_idents[i], 63);
            s->used_vars[s->n_used_vars-1][63] = '\0';
        }
    }
}

/* -------------------------------------------------------------------------
 * Expression transformation: NC → tinyexpr-compatible
 * ---------------------------------------------------------------------- */

/**
 * AWG gauge N (positive) → wire radius in metres.
 * Formula: d_mm = 0.127 * 92^((36-N)/39) ; r_m = d_mm/2000
 */
/* static helper currently unused; keep for potential future use. */
static double awg_radius_m(int gauge) __attribute__((unused));
static double awg_radius_m(int gauge)
{
    double d_mm = 0.127 * pow(92.0, (36.0 - gauge) / 39.0);
    return d_mm / 2000.0;
}

/**
 * Transform an NC expression string to a tinyexpr-compatible string:
 *   N"       → N*0.0254   (the " immediately follows a digit in dst)
 *   N'       → N*0.3048   (the ' immediately follows a digit in dst)
 *   #N       → numeric AWG radius (decimal)
 *   sind(    → sin(
 *   cosd(    → cos(
 *   tand(    → tan(
 *   atand(   → atan(
 */
static void expand_expr(const char *src, char *dst, int dsz)
{
    int di = 0;
    while (*src && di < dsz - 1) {
        /* AWG: #<digits> -> emit awgN symbol (keep as symbol, not numeric) */
        if (*src == '#' && isdigit((unsigned char)src[1])) {
            char *end;
            long g = strtol(src + 1, &end, 10);
            int  n = snprintf(dst + di, dsz - di, "awg%ld", g);
            if (n > 0) di += n;
            src = end;
            continue;
        }
        /* inch suffix: digit already in dst, next src char is '"' -> emit in */
        if (*src == '"' && di > 0 && isdigit((unsigned char)dst[di-1])) {
            int n = snprintf(dst + di, dsz - di, "in");
            if (n > 0) di += n;
            src++;
            continue;
        }
        /* feet suffix -> ft */
        if (*src == '\'' && di > 0 && isdigit((unsigned char)dst[di-1])) {
            int n = snprintf(dst + di, dsz - di, "ft");
            if (n > 0) di += n;
            src++;
            continue;
        }
        /* SI micro/nano/pico suffixes: digit followed by bare u/n/p (not part
         * of a longer identifier such as uH, nF, pF which the evaluator already
         * knows).  Emit a numeric multiplier so no extra unit constant is needed. */
        if (di > 0 && isdigit((unsigned char)dst[di-1])
                && !isalnum((unsigned char)src[1]) && src[1] != '_') {
            const char *mul = NULL;
            if      (*src == 'u') mul = "*1e-6";
            else if (*src == 'n') mul = "*1e-9";
            else if (*src == 'p') mul = "*1e-12";
            if (mul) {
                int n = snprintf(dst + di, dsz - di, "%s", mul);
                if (n > 0) di += n;
                src++;
                continue;
            }
        }
        /* trig degree variants */
        if (strncmp(src, "sind(",  5) == 0) {
            int n = snprintf(dst + di, dsz - di, "sin("); if (n>0) di+=n; src+=5; continue;
        }
        if (strncmp(src, "cosd(",  5) == 0) {
            int n = snprintf(dst + di, dsz - di, "cos("); if (n>0) di+=n; src+=5; continue;
        }
        if (strncmp(src, "tand(",  5) == 0) {
            int n = snprintf(dst + di, dsz - di, "tan("); if (n>0) di+=n; src+=5; continue;
        }
        if (strncmp(src, "atand(", 6) == 0) {
            int n = snprintf(dst + di, dsz - di, "atan("); if (n>0) di+=n; src+=6; continue;
        }
        /* Strip spaces: the card field parser splits on whitespace, so
         * expressions like "h - s" would be tokenized as three separate
         * fields.  Spaces carry no semantic meaning in NC expressions. */
        if (*src == ' ') { src++; continue; }
        dst[di++] = *src++;
    }
    dst[di] = '\0';
}

/**
 * Return the centre-segment expression for a wire with `segs` segments.
 * If segs_str is a pure integer, compute the integer.
 * Otherwise emit "(<segs_str>+1)/2".
 */
static void centre_seg(const char *segs_str, char *out, int sz)
{
    char *end;
    long n = strtol(segs_str, &end, 10);
    if (*end == '\0' && n > 0)
        snprintf(out, sz, "%ld", (n + 1) / 2);
    else
        snprintf(out, sz, "(%s+1)/2", segs_str);
}

/**
 * Return true if the expression string contains the standalone identifier 'c'.
 * Used to decide if we need to inject SY c=299.792458.
 */
static bool expr_uses_c(const char *s)
{
    const char *start = s;
    for (; *s; s++) {
        if (*s != 'c') continue;
        char l = (s > start) ? s[-1] : '\0';
        char r = s[1];
        bool left_ok  = (l == '\0' || (!isalnum((unsigned char)l)  && l != '_'));
        bool right_ok = (r == '\0' || (!isalnum((unsigned char)r)  && r != '_'));
        if (left_ok && right_ok) return true;
    }
    return false;
}

/* -------------------------------------------------------------------------
 * NC reader state
 * ---------------------------------------------------------------------- */

/* Forward declaration */
static void ncr_stmt(ncr_t *s, deck_t *deck, char **post, int *np, int maxp);

/* -------------------------------------------------------------------------
 * Wire helper
 * ---------------------------------------------------------------------- */

/**
 * Process a wire()  (or line()) argument string; emit a GW card into deck.
 * Returns the assigned tag number, or -1 on error.
 * *seg_out receives the (expanded) segment-count expression.
 */
static int ncr_wire(ncr_t *s, const char *argstr, deck_t *deck,
                    char *seg_out, int seg_sz)
{
    char args[8][256];
    int n = nc_split(argstr, args, 8);
    if (n < 8) return -1;

    char xa[8][256];
    for (int i = 0; i < 8; i++) expand_expr(args[i], xa[i], 256);

    int tag = s->next_tag++;
    if (seg_out) {
        strncpy(seg_out, xa[7], seg_sz - 1);
        seg_out[seg_sz - 1] = '\0';
    }

    /* Track all variables used in wire arguments */
    for (int i = 0; i < 8; i++) {
        char wire_idents[32][64];
        int n_idents = nc_extract_identifiers(xa[i], wire_idents, 32);
        for (int j = 0; j < n_idents; j++) {
            bool already = false;
            for (int k = 0; k < s->n_used_vars; k++) {
                if (strcmp(s->used_vars[k], wire_idents[j]) == 0) {
                    already = true;
                    break;
                }
            }
            if (!already && s->n_used_vars < NCR_MAX_VARS) {
                strncpy(s->used_vars[s->n_used_vars++], wire_idents[j], sizeof(s->used_vars[0]) - 1);
                s->used_vars[s->n_used_vars-1][sizeof(s->used_vars[0]) - 1] = '\0';
            }
        }
    }

    char buf[512];
    snprintf(buf, sizeof buf,
             "GW %d, %s, %s, %s, %s, %s, %s, %s, %s",
             tag, xa[7],
             xa[0], xa[1], xa[2],
             xa[3], xa[4], xa[5],
             xa[6]);
    append_card_from_text(deck, buf);
    return tag;
}

/* -------------------------------------------------------------------------
 * Resolve an element-or-inline-wire argument
 * ---------------------------------------------------------------------- */

/**
 * Given an argument string that is either:
 *   (a) a variable name  → look up in el_vars → return tag, set *seg
 *   (b) wire(...) / line(...) inline call → process, return tag, set *seg
 *
 * Returns tag number (≥1) or -1 if not resolvable.
 */
static int ncr_resolve_elem(ncr_t *s, const char *arg,
                            deck_t *deck,
                            char *seg_out, int seg_sz)
{
    const char *a = arg;
    while (*a && isspace((unsigned char)*a)) a++;

    /* check for inline wire() / line() */
    if (strncmp(a, "wire(", 5) == 0 || strncmp(a, "line(", 5) == 0) {
        const char *inner = strchr(a, '(');
        if (!inner) return -1;
        inner++;
        /* extract content up to matching ')' */
        char wargs[512];
        int depth = 1, n = 0;
        const char *p = inner;
        while (*p && depth > 0) {
            if (*p == '(') depth++;
            else if (*p == ')') { if (--depth == 0) break; }
            if (n < 510) wargs[n++] = *p;
            p++;
        }
        wargs[n] = '\0';
        return ncr_wire(s, wargs, deck, seg_out, seg_sz);
    }

    /* look up element variable */
    char name[64]; int ni = 0;
    while (*a && (isalnum((unsigned char)*a) || *a == '_') && ni < 63)
        name[ni++] = *a++;
    name[ni] = '\0';
    for (int i = 0; i < s->n_elv; i++) {
        if (strcmp(s->el_vars[i], name) == 0 && s->el_tags[i] > 0) {
            if (seg_out) {
                strncpy(seg_out, s->el_segs[i], seg_sz - 1);
                seg_out[seg_sz - 1] = '\0';
            }
            return s->el_tags[i];
        }
    }
    return -1;
}

/* -------------------------------------------------------------------------
 * Post-card (post-GE) accumulator helper
 * ---------------------------------------------------------------------- */

static void post_add(char **post, int *np, int maxp, const char *str)
{
    if (*np >= maxp) return;
    post[(*np)++] = strdup(str);
}

/* -------------------------------------------------------------------------
 * Function call dispatcher
 * ---------------------------------------------------------------------- */

static void ncr_func(ncr_t *s, const char *func, const char *argbuf,
                     deck_t *deck, char **post, int *np, int maxp)
{
    /* ---- geometry ---------------------------------------------------- */
    if (strcmp(func, "wire") == 0 || strcmp(func, "line") == 0) {
        char seg[64];
        ncr_wire(s, argbuf, deck, seg, sizeof seg);
        return;
    }

    /* ---- excitation -------------------------------------------------- */
    if (strcmp(func, "voltageFeed") == 0 || strcmp(func, "currentFeed") == 0) {
        char args[4][256];
        int  na = nc_split(argbuf, args, 4);
        if (na < 3) return;
        int  ex_type = (strcmp(func, "currentFeed") == 0) ? 1 : 0;
        char seg[64] = "1";
        int  tag = ncr_resolve_elem(s, args[0], deck, seg, sizeof seg);
        if (tag < 0) return;
        char cseg[64];
        centre_seg(seg, cseg, sizeof cseg);
        char re[256], im[256];
        expand_expr(args[1], re, sizeof re);
        expand_expr(args[2], im, sizeof im);
        nc_track_expr_vars(s, re);
        nc_track_expr_vars(s, im);
        char buf[1024];
        snprintf(buf, sizeof buf,
                 "EX %d, %d, %s, 0, %s, %s",
                 ex_type, tag, cseg, re, im);
        post_add(post, np, maxp, buf);
        return;
    }
    if (strcmp(func, "voltageFeedAtSegment") == 0 ||
        strcmp(func, "currentFeedAtSegment") == 0) {
        char args[5][256];
        int  na = nc_split(argbuf, args, 5);
        if (na < 4) return;
        int  ex_type = (strncmp(func, "current", 7) == 0) ? 1 : 0;
        char seg[64] = "1";
        int  tag = ncr_resolve_elem(s, args[0], deck, seg, sizeof seg);
        if (tag < 0) return;
        char re[256], im[256];
        expand_expr(args[1], re, sizeof re);
        expand_expr(args[2], im, sizeof im);
        char seg_e[64]; expand_expr(args[3], seg_e, sizeof seg_e);
        char buf[1024];
        snprintf(buf, sizeof buf,
                 "EX %d, %d, %s, 0, %s, %s",
                 ex_type, tag, seg_e, re, im);
        post_add(post, np, maxp, buf);
        return;
    }

    /* ---- loading ----------------------------------------------------- */
    if (strcmp(func, "impedanceLoad") == 0 ||
        strcmp(func, "impedanceAtSegments") == 0) {
        char args[6][256];
        int  na = nc_split(argbuf, args, 6);
        if (na < 3) return;
        char seg[64] = "1";
        int  tag = ncr_resolve_elem(s, args[0], deck, seg, sizeof seg);
        if (tag < 0) return;
        char cseg[64];
        centre_seg(seg, cseg, sizeof cseg);
        char R[64], X[64];
        expand_expr(args[1], R, sizeof R);
        expand_expr(args[2], X, sizeof X);
        char buf[512];
        if (na >= 5 /* has fromSeg, toSeg */) {
            char s1[64], s2[64];
            expand_expr(args[3], s1, sizeof s1);
            expand_expr(args[4], s2, sizeof s2);
            snprintf(buf, sizeof buf,
                     "LD 4, %d, %s, %s, %s, %s", tag, s1, s2, R, X);
        } else {
            snprintf(buf, sizeof buf,
                     "LD 4, %d, %s, %s, %s, %s", tag, cseg, cseg, R, X);
        }
        post_add(post, np, maxp, buf);
        return;
    }
    if (strcmp(func, "lumpedSeriesLoad") == 0 ||
        strcmp(func, "distributedSeriesLoad") == 0) {
        char args[5][256];
        int  na = nc_split(argbuf, args, 5);
        if (na < 4) return;
        char seg[64] = "1";
        int  tag = ncr_resolve_elem(s, args[0], deck, seg, sizeof seg);
        if (tag < 0) return;
        char cseg[64];
        centre_seg(seg, cseg, sizeof cseg);
        char R[64], L[64], C[64];
        expand_expr(args[1], R, sizeof R); /* R Ω */
        expand_expr(args[2], L, sizeof L); /* L H */
        expand_expr(args[3], C, sizeof C); /* C F */
        /* LD type 0: R/L/C in series; fields: F1=R, F2=L, F3=C */
        char buf[512];
        snprintf(buf, sizeof buf,
                 "LD 0, %d, %s, %s, %s, %s, %s", tag, cseg, cseg, R, L, C);
        post_add(post, np, maxp, buf);
        return;
    }
    if (strcmp(func, "resistiveLoad") == 0) {
        char args[2][256]; nc_split(argbuf, args, 2);
        char seg[64] = "1";
        int tag = ncr_resolve_elem(s, args[0], deck, seg, sizeof seg);
        if (tag < 0) return;
        char cseg[64]; centre_seg(seg, cseg, sizeof cseg);
        char R[64]; expand_expr(args[1], R, sizeof R);
        char buf[256];
        snprintf(buf, sizeof buf, "LD 4, %d, %s, %s, %s, 0", tag, cseg, cseg, R);
        post_add(post, np, maxp, buf);
        return;
    }

    /* ---- frequency --------------------------------------------------- */
    if (strcmp(func, "setFrequency") == 0) {
        char f[64]; expand_expr(argbuf, f, sizeof f);
        char buf[128];
        snprintf(buf, sizeof buf, "FR 0, 1, 0, 0, %s, 0", f);
        post_add(post, np, maxp, buf);
        s->first_freq = false;
        s->have_fr = true;
        return;
    }
    if (strcmp(func, "addFrequency") == 0) {
        char f[64]; expand_expr(argbuf, f, sizeof f);
        char buf[128];
        snprintf(buf, sizeof buf, "FR 0, 1, 0, 0, %s, 0", f);
        post_add(post, np, maxp, buf);
        s->have_fr = true;
        return;
    }
    if (strcmp(func, "frequencySweep") == 0) {
        char args[3][256]; int na = nc_split(argbuf, args, 3);
        if (na < 3) return;
        /* f0, f1, n  → compute step = (f1-f0)/(n-1) or emit approximation */
        char f0[256], f1[256], nf[256];
        expand_expr(args[0], f0, sizeof f0);
        expand_expr(args[1], f1, sizeof f1);
        expand_expr(args[2], nf, sizeof nf);
        /* Try to compute step numerically if all are literals */
        char *e0, *e1, *en;
        double v0 = strtod(f0, &e0);
        double v1 = strtod(f1, &e1);
        long   vn = strtol(nf, &en, 10);
        char buf[256];
        if (*e0=='\0' && *e1=='\0' && *en=='\0' && vn >= 2) {
            double step = (v1 - v0) / (vn - 1);
            snprintf(buf, sizeof buf,
                     "FR 0, %ld, 0, 0, %.8g, %.8g", vn, v0, step);
        } else {
            /* fall back: emit only the start frequency */
            char buf2[512];  /* Increased from 256 to safely accommodate format string */
            snprintf(buf2, sizeof buf2, "FR 0, 1, 0, 0, %s, 0", f0);
            post_add(post, np, maxp, buf2);
            return;
        }
        post_add(post, np, maxp, buf);
        s->first_freq = false;
        s->have_fr = true;
        return;
    }

    /* ---- ground ------------------------------------------------------ */
    if (strcmp(func, "freespace") == 0) {
        if (!s->have_gn) { post_add(post, np, maxp, "GN -1"); s->have_gn = true; }
        return;
    }
    if (strcmp(func, "perfectGround") == 0) {
        if (!s->have_gn) { post_add(post, np, maxp, "GN 1"); s->have_gn = true; }
        return;
    }
    if (strcmp(func, "averageGround") == 0) {
        if (!s->have_gn) { post_add(post, np, maxp, "GN 0, 0, 0, 0, 13, 0.005"); s->have_gn = true; }
        return;
    }
    if (strcmp(func, "goodGround") == 0) {
        if (!s->have_gn) { post_add(post, np, maxp, "GN 0, 0, 0, 0, 20, 0.0303"); s->have_gn = true; }
        return;
    }
    if (strcmp(func, "poorGround") == 0) {
        if (!s->have_gn) { post_add(post, np, maxp, "GN 0, 0, 0, 0, 13, 0.002"); s->have_gn = true; }
        return;
    }
    if (strcmp(func, "saltWaterGround") == 0) {
        if (!s->have_gn) { post_add(post, np, maxp, "GN 0, 0, 0, 0, 81, 5.0"); s->have_gn = true; }
        return;
    }
    if (strcmp(func, "freshWaterGround") == 0) {
        if (!s->have_gn) { post_add(post, np, maxp, "GN 0, 0, 0, 0, 80, 0.001"); s->have_gn = true; }
        return;
    }
    if (strcmp(func, "ground") == 0) {
        /* ground( epsilon_r, sigma ) */
        char args[2][256]; int na = nc_split(argbuf, args, 2);
        if (na < 2) return;
        char eps[256], sig[256];
        expand_expr(args[0], eps, sizeof eps);
        expand_expr(args[1], sig, sizeof sig);
        char buf[512];  /* Increased from 256 to safely accommodate format string */
        int gn_type = s->sommerfeld ? 2 : 0;
        snprintf(buf, sizeof buf, "GN %d, 0, 0, 0, %s, %s", gn_type, eps, sig);
        if (!s->have_gn) { post_add(post, np, maxp, buf); s->have_gn = true; }
        return;
    }
    if (strcmp(func, "useSommerfeldGround") == 0) {
        s->sommerfeld = (int)strtol(argbuf, NULL, 10);
        return;
    }

    /* ---- radiation pattern ------------------------------------------ */
    if (strcmp(func, "azimuthPlotForElevationAngle") == 0) {
        char ea[256]; expand_expr(argbuf, ea, sizeof ea);
        double elev = strtod(ea, NULL);
        char buf[256];
        snprintf(buf, sizeof buf,
                 "RP 0, 1, 360, 1000, %.6g, 0, 0, 1", 90.0 - elev);
        post_add(post, np, maxp, buf);
        return;
    }
    if (strcmp(func, "elevationPlotForAzimuthAngle") == 0) {
        char aa[256]; expand_expr(argbuf, aa, sizeof aa);
        double az = strtod(aa, NULL);
        char buf[256];
        snprintf(buf, sizeof buf,
                 "RP 0, 181, 1, 1000, 0, %.6g, 1, 0", az);
        post_add(post, np, maxp, buf);
        return;
    }

    /* ---- misc -------------------------------------------------------- */
    if (strcmp(func, "useExtendedKernel") == 0) {
        if (strtol(argbuf, NULL, 10))
            post_add(post, np, maxp, "EK");
        return;
    }
    /* any other function call: skip silently */
}

/* -------------------------------------------------------------------------
 * Statement parser
 * ---------------------------------------------------------------------- */

static void ncr_stmt(ncr_t *s, deck_t *deck, char **post, int *np, int maxp)
{
    nc_skip(&s->p);
    if (!*s->p || *s->p == '}') return;

    /* read leading identifier */
    char ident[128];
    if (!nc_ident(&s->p, ident, sizeof ident) || !ident[0]) {
        /* non-identifier: skip to semicolon */
        while (*s->p && *s->p != ';' && *s->p != '}') {
            if (*s->p == '{') { nc_skip_block(&s->p); continue; }
            s->p++;
        }
        nc_semi(&s->p);
        return;
    }

    nc_skip(&s->p);

    /* ---- type declarations ------------------------------------------ */
    bool is_real_int = (strcmp(ident, "real") == 0 || strcmp(ident, "int") == 0 || strcmp(ident, "float") == 0);
    bool is_elem     = (strcmp(ident, "element") == 0);
    bool is_other_type = (!is_real_int && !is_elem &&
                          (strcmp(ident, "coaxtype") == 0 ||
                           strcmp(ident, "vector")   == 0 ||
                           strcmp(ident, "transform") == 0));
    if (is_real_int || is_elem || is_other_type) {
        while (*s->p && *s->p != ';') {
            nc_skip(&s->p);
            char name[64];
            nc_ident(&s->p, name, sizeof name);
            if (name[0]) {
                if (is_real_int && s->n_syv < NCR_MAX_VARS) {
                    bool already = false;
                    for (int i = 0; i < s->n_syv; i++) {
                        if (strcmp(s->sy_vars[i], name) == 0) { already = true; break; }
                    }
                    if (!already) {
                        strncpy(s->sy_vars[s->n_syv], name, sizeof(s->sy_vars[s->n_syv]));
                        s->sy_assigned[s->n_syv] = false;
                        s->n_syv++;
                    }
                }
                if (is_elem && s->n_elv < NCR_MAX_VARS) {
                    strncpy(s->el_vars[s->n_elv], name, sizeof(s->el_vars[s->n_elv]));
                    s->el_tags[s->n_elv] = 0;
                    s->el_segs[s->n_elv][0] = '\0';
                    s->n_elv++;
                }
            }
            nc_skip(&s->p);
            if (*s->p == ',') s->p++;
        }
        nc_semi(&s->p);
        return;
    }

    /* ---- function call: ident( ... ) ; -------------------------------- */
    if (*s->p == '(') {
        char argbuf[1024];
        nc_parens(&s->p, argbuf, sizeof argbuf);
        int cards_before = deck->num_cards;
        ncr_func(s, ident, argbuf, deck, post, np, maxp);
        nc_skip(&s->p);
        /* consume optional ';' — some calls omit it */
        nc_semi(&s->p);
        /* attach any end-of-line // comment to the last card emitted */
        char eol[256] = {0};
        if (nc_eol_comment(s->p, eol, sizeof eol) && deck->num_cards > cards_before)
            nc_card_append_comment(deck, eol);
        return;
    }

    /* ---- assignment: ident = RHS ; ------------------------------------ */
    if (*s->p == '=') {
        s->p++; /* skip '=' */
        nc_skip(&s->p);

        /* Is this an element variable? */
        int ei = -1;
        for (int i = 0; i < s->n_elv; i++) {
            if (strcmp(s->el_vars[i], ident) == 0) { ei = i; break; }
        }

        if (ei >= 0) {
            /* element assignment: RHS must be wire()/line() call */
            char func[64];
            nc_ident(&s->p, func, sizeof func);
            nc_skip(&s->p);
            if ((strcmp(func, "wire") == 0 || strcmp(func, "line") == 0)
                && *s->p == '(') {
                char wargs[1024];
                nc_parens(&s->p, wargs, sizeof wargs);
                char seg[64];
                int tag = ncr_wire(s, wargs, deck, seg, sizeof seg);
                if (tag >= 0) {
                    s->el_tags[ei] = tag;
                    strncpy(s->el_segs[ei], seg, sizeof(s->el_segs[ei]));
                }
            } else {
                /* skip to semicolon */
                while (*s->p && *s->p != ';' && *s->p != '}') s->p++;
            }
            nc_skip(&s->p);
            nc_semi(&s->p);
            /* attach any end-of-line // comment to the GW card */
            {
                char eol[256] = {0};
                if (nc_eol_comment(s->p, eol, sizeof eol))
                    nc_card_append_comment(deck, eol);
            }
            return;
        }

        /* Regular (SY) assignment — read the expression */
        char expr_raw[512];
        nc_read_to(&s->p, expr_raw, sizeof expr_raw, ';');
        nc_semi(&s->p);

        /* capture any end-of-line // comment before nc_skip moves past newline */
        char eol[256] = {0};
        nc_eol_comment(s->p, eol, sizeof eol);

        char expr[512];
        expand_expr(expr_raw, expr, sizeof expr);

        /* Extract identifiers used in this expression and add to used_vars */
        {
            char expr_idents[32][64];
            int n_idents = nc_extract_identifiers(expr, expr_idents, 32);
            for (int i = 0; i < n_idents; i++) {
                bool already = false;
                for (int j = 0; j < s->n_used_vars; j++) {
                    if (strcmp(s->used_vars[j], expr_idents[i]) == 0) {
                        already = true;
                        break;
                    }
                }
                if (!already && s->n_used_vars < NCR_MAX_VARS) {
                    strncpy(s->used_vars[s->n_used_vars], expr_idents[i], sizeof(s->used_vars[s->n_used_vars]));
                    s->n_used_vars++;
                }
            }
        }

        /* If this identifier is one of the declared real/int vars, mark it assigned */
        int assigned_index = -1;
        for (int i = 0; i < s->n_syv; i++) {
            if (strcmp(s->sy_vars[i], ident) == 0) {
                assigned_index = i;
                break;
            }
        }
        if (assigned_index < 0 && s->n_syv < NCR_MAX_VARS) {
            assigned_index = s->n_syv;
            strncpy(s->sy_vars[s->n_syv], ident, sizeof(s->sy_vars[s->n_syv]));
            s->sy_assigned[s->n_syv] = false;
            s->sy_values[s->n_syv][0] = '\0';
            s->n_syv++;
        }
        if (assigned_index >= 0) {
            s->sy_assigned[assigned_index] = true;
            strncpy(s->sy_values[assigned_index], expr, sizeof(s->sy_values[assigned_index]));
        }

        /* check if 'c' constant is needed */
        if (expr_uses_c(expr)) s->need_c = true;

        /* Do not append SY cards now; they will be emitted before geometry. */
        return;
    }

    /* ---- cannot parse: skip to semicolon ------------------------------ */
    while (*s->p && *s->p != ';' && *s->p != '}') {
        if (*s->p == '{') { nc_skip_block(&s->p); continue; }
        s->p++;
    }
    nc_semi(&s->p);
}

/**
 * @copydoc read_deck_nc
 */
int read_deck_nc(context_t *ctx, deck_t *deck, FILE *fp, errors_list_t *errors)
{
    if (!deck || !fp) return -1;

    /* ---- read file into buffer ---------------------------------------- */
    fseek(fp, 0, SEEK_END);
    long fsz = ftell(fp);
    rewind(fp);
    if (fsz <= 0) return -1;

    char *buf = malloc((size_t)fsz + 1);
    if (!buf) return -1;
    size_t nr = fread(buf, 1, (size_t)fsz, fp);
    buf[nr] = '\0';

    /* ---- initialise parser state ------------------------------------- */
    ncr_t s;
    memset(&s, 0, sizeof s);
    s.p         = buf;
    s.next_tag  = 1;
    s.first_freq = true;

    /* ---- find model( "name" ) { ---------------------------------------- */
    const char *title = "antenna";
    char title_buf[256] = {0};

    /* Accumulate // comment lines seen before model() */
    char pre_comments[4096] = {0};
    int  pre_n = 0;

    /* Scan for 'model' at top level */
    for (;;) {
        nc_skip_collect(&s.p, pre_comments, sizeof pre_comments, &pre_n);
        if (!*s.p) { free(buf); return -1; }
        char tok[128];
        nc_ident(&s.p, tok, sizeof tok);
        nc_skip(&s.p);
        if (strcmp(tok, "model") == 0 && *s.p == '(') {
            /* extract model name */
            char args[512];
            nc_parens(&s.p, args, sizeof args);
            const char *a = args;
            while (*a && isspace((unsigned char)*a)) a++;
            if (*a == '"') {
                nc_qstring(&a, title_buf, sizeof title_buf);
                title = title_buf;
            }
            break;
        }

        /* top-level real/int/float declarations (outside model) */
        if (strcmp(tok, "real") == 0 || strcmp(tok, "int") == 0 || strcmp(tok, "float") == 0) {
            while (*s.p && *s.p != ';') {
                nc_skip(&s.p);
                char name[64];
                nc_ident(&s.p, name, sizeof name);
                if (name[0] && s.n_syv < NCR_MAX_VARS) {
                    bool already = false;
                    for (int i = 0; i < s.n_syv; i++) {
                        if (strcmp(s.sy_vars[i], name) == 0) { already = true; break; }
                    }
                    if (!already) {
                        strncpy(s.sy_vars[s.n_syv], name, sizeof(s.sy_vars[s.n_syv]));
                        s.sy_assigned[s.n_syv] = false;
                        s.n_syv++;
                    }
                }
                nc_skip(&s.p);
                if (*s.p == ',') s.p++;
            }
            nc_semi(&s.p);
            continue;
        }

        /* skip any other top-level construct */
        if (*s.p == '(') {
            char tmp[1024]; nc_parens(&s.p, tmp, sizeof tmp);
            nc_skip(&s.p);
        }
        if (*s.p == '{') nc_skip_block(&s.p);
        else {
            while (*s.p && *s.p != ';' && *s.p != '{' && *s.p != '}') s.p++;
            if (*s.p == ';') s.p++;
        }
    }

    /* ---- emit CM + CE ----------------------------------------- */
    {
        char cm[512];
        snprintf(cm, sizeof cm, "CM %s", title);
        append_card_from_text(deck, cm);
    }
    /* ---- emit pre-model comments as CM cards (before CE) ------------- */
    if (pre_n > 0) {
        // if there is a title, add a blank line to make it obvious
        bool has_title = (title_buf[0] != '\0');
        if (has_title) append_card_from_text(deck, "CM");
        const char *line = pre_comments;
        while (*line) {
            const char *nl = strchr(line, '\n');
            int len = nl ? (int)(nl - line) : (int)strlen(line);
            char cm[512];
            snprintf(cm, sizeof cm, "CM %.*s", len, line);
            append_card_from_text(deck, cm);
            if (!nl) break;
            line = nl + 1;
        }
    }
    append_card_from_text(deck, "CE");

    /* ---- parse model body -------------------------------------------- */
    nc_skip(&s.p);
    if (*s.p != '{') { free(buf); return -1; }
    s.p++; /* skip '{' */

    /* post-GE card buffer (EX, LD, FR, GN, RP, EN) */
    char *post[NCR_MAX_POST];
    memset(post, 0, sizeof post);
    int   np = 0;

    int depth = 1;
    while (*s.p && depth > 0) {
        /* Collect top-level // comment lines and emit as NEC '!' cards.
         * These appear interleaved with geometry (GW) cards in the output,
         * preserving the author's inline documentation. */
        for (;;) {
            while (*s.p && isspace((unsigned char)*s.p)) s.p++;
            if (s.p[0] != '/' || s.p[1] != '/') break;
            s.p += 2; /* skip // */
            while (*s.p == ' ' || *s.p == '\t') s.p++; /* trim leading space */
            char cmt[512]; int ci = 0;
            while (*s.p && *s.p != '\n' && ci < (int)sizeof(cmt) - 1)
                cmt[ci++] = *s.p++;
            while (ci > 0 && isspace((unsigned char)cmt[ci-1])) ci--;
            cmt[ci] = '\0';
            char ccard[540];
            snprintf(ccard, sizeof ccard, "! %s", cmt);
            append_card_from_text(deck, ccard);
        }
        nc_skip(&s.p);
        if (!*s.p) break;
        if (*s.p == '}') {
            if (--depth == 0) { s.p++; break; }
            s.p++;
            continue;
        }
        if (*s.p == '{') {
            depth++;
            s.p++;
            continue;
        }
        ncr_stmt(&s, deck, post, &np, NCR_MAX_POST);
    }

    /* ---- detect variables used in model() but only assigned in control() ---- */
    {
        char control_vars[NCR_MAX_VARS][64];
        int n_control_vars = nc_scan_control_vars(s.p, control_vars, NCR_MAX_VARS);
        
        /* For each variable used in model(), check if it's not assigned in model()
         * but IS assigned in control() */
        for (int i = 0; i < s.n_used_vars; i++) {
            const char *used_var = s.used_vars[i];
            
            /* Check if assigned in model() */
            bool assigned_in_model = false;
            for (int j = 0; j < s.n_syv; j++) {
                if (strcmp(s.sy_vars[j], used_var) == 0 && s.sy_assigned[j]) {
                    assigned_in_model = true;
                    break;
                }
            }
            
            /* Check if assigned in control() */
            bool assigned_in_control = false;
            for (int j = 0; j < n_control_vars; j++) {
                if (strcmp(control_vars[j], used_var) == 0) {
                    assigned_in_control = true;
                    break;
                }
            }
            
            /* Warn if used in model() but only assigned in control() */
            if (!assigned_in_model && assigned_in_control) {
                size_t mlen = strlen(used_var) * 2 + 256;  /* Increased buffer for safe formatting */
                char *msg = malloc(mlen);
                if (msg) {
                    snprintf(msg, mlen, "WARNING: variable '%s' is used in model() but only assigned in control(); the generated NEC output will have SY %s=0 and requires user initialization.", used_var, used_var);
                    add_error(ctx, errors, msg, WARNING);
                    free(msg);
                }
            }
        }
    }

    /* ---- emit SY variables (at top of NEC deck) ---------------------- */
    int ce_index = -1;
    for (int i = 0; i < deck->num_cards; i++) {
        if (deck->cards[i].card_code[0] == 'C' && deck->cards[i].card_code[1] == 'E') {
            ce_index = i;
            break;
        }
    }
    int insert_at = (ce_index >= 0) ? (ce_index + 1) : 0;

    for (int i = 0; i < s.n_syv; i++) {
        char zbuf[384];
        if (s.sy_assigned[i]) {
            snprintf(zbuf, sizeof zbuf, "SY %s=%s", s.sy_vars[i], s.sy_values[i]);
        } else {
            snprintf(zbuf, sizeof zbuf, "SY %s=0 ' value not set in model(),  in SY", s.sy_vars[i]);
        }
        append_card_from_text(deck, zbuf);
        int last_index = deck->num_cards - 1;
        if (last_index >= 0 && insert_at <= last_index) {
            move_card(deck, last_index, insert_at);
            insert_at++;
        }
    }

    /* ---- GE termination ---------------------------------------------- */
    append_card_from_text(deck, "GE 0");

    /* ---- inject SY c=299.792458 if needed (before other post cards) ---
     * Actually it must come before GW, so it's too late here.
     * We already emitted SY inline.  But 'c' in expressions was emitted
     * inline too.  Check if any emitted SY still uses bare 'c' — if the
     * user referenced 'c' but never declared it, we need to inject it.
     * Since SY cards were emitted before any GW, we can't retroactively
     * insert before them.  Best we can do: emit a ! comment noting it.
     * TODO: two-pass approach for cleaner 'c' injection. */

    /* ---- GN handling: if useSommerfeldGround was seen but no ground()
     * gave us a GN card, patch any emitted ground GN to use type=2, or
     * emit a default. */
    if (s.sommerfeld && !s.have_gn) {
        /* default: average ground with Sommerfeld */
        post_add(post, &np, NCR_MAX_POST,
                 "GN 2, 0, 0, 0, 13, 0.005");
        s.have_gn = true;
    } else if (s.sommerfeld) {
        /* scan post[] for the GN card and upgrade its type field */
        for (int i = 0; i < np; i++) {
            if (!post[i]) continue;
            if (strncmp(post[i], "GN ", 3) == 0) {
                /* parse the type integer */
                char *p = post[i] + 3;
                while (*p && isspace((unsigned char)*p)) p++;
                int gn_t = (int)strtol(p, NULL, 10);
                if (gn_t == 0) {
                    /* upgrade to Sommerfeld-Norton (type 2) */
                    char newgn[256];
                    snprintf(newgn, sizeof newgn, "GN 2%s", p + 1);
                    free(post[i]);
                    post[i] = strdup(newgn);
                }
                break;
            }
        }
    }
    if (!s.have_gn) {
        post_add(post, &np, NCR_MAX_POST, "GN -1");
    }

    /* ---- emit post-GE cards ------------------------------------------ */
    /* Two-pass strategy:
     *   Pass 1 – emit every non-FR card (EX, LD, GN, NT, TL, …) in order.
     *   Pass 2 – emit each FR card immediately followed by an RP card.
     * This ensures EX/GN/LD are set before the first frequency run, and
     * that each distinct frequency produces its own radiation-pattern output.
     * (NEC-2 only generates output when RP is encountered; a bare sequence of
     * FR…FR…RP only produces output for the last frequency.) */

    /* Pass 1: non-FR cards */
    for (int i = 0; i < np; i++) {
        if (post[i] && !(post[i][0]=='F' && post[i][1]=='R' &&
                          (post[i][2]==' ' || post[i][2]=='\0'))) {
            append_card_from_text(deck, post[i]);
        }
    }

    /* Pass 2: FR cards, each followed by RP */
    if (!s.have_fr) {
        /* no frequency specified – use a default 14 MHz */
        append_card_from_text(deck, "FR 0, 1, 0, 0, 14.0, 0");
        append_card_from_text(deck, "RP 0, 37, 73, 1000, 0, 0, 5, 5");
    } else {
        for (int i = 0; i < np; i++) {
            if (post[i] && post[i][0]=='F' && post[i][1]=='R' &&
                           (post[i][2]==' ' || post[i][2]=='\0')) {
                append_card_from_text(deck, post[i]);
                append_card_from_text(deck, "RP 0, 37, 73, 1000, 0, 0, 5, 5");
            }
        }
    }

    /* free remaining post entries */
    for (int i = 0; i < np; i++) free(post[i]);

    /* ---- EN termination ---------------------------------------------- */
    append_card_from_text(deck, "EN");

    free(buf);
    return 0;
}
