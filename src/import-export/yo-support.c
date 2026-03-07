/******************************************************************************
 * yo-support.c
 *
 * Functions to convert between OpenNEC deck structures and the Yagi
 * Optimizer ".yo" (also ".ant", ".yag") file format.  The format is a
 * text-based, line-oriented representation introduced by Brian Beezley in
 * 1994 for his Yagi Optimizer program.
 *
 * Importer notes
 * --------------
 * The coordinate system used for generated GW cards is:
 *   X axis — element half-lengths (both halves emitted, symmetric about X=0)
 *   Y axis — height above ground (from the optional "Height" option line)
 *   Z axis — position along the boom
 *
 * A GS scale card is appended before GE so that all GW values, which are
 * stored in the original file's measurement units, are converted to metres
 * by NEC.  An LD 5 card is added when a material is defined.  A GM card
 * with Z-translation is added when a "Stacked" option is present.
 *
 * Taper vs. length line disambiguation uses the sum-of-values-vs-0.15λ
 * heuristic from the YO documentation: small sums (diameters) identify
 * taper lines; large sums (measured lengths) identify length lines.
 *
 * Exporter notes
 * --------------
 * The exporter produces only a minimal, round-trip-capable .yo file.  It
 * extracts GW wire geometry (assuming the X/Y/Z conventions above), the FR
 * frequency, and any CM title comment.
 *
 *****************************************************************************/

#include "yo-support.h"
#include "deck.h"
#include "misc.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* -------------------------------------------------------------------------
 * Internal constants / limits
 * ---------------------------------------------------------------------- */

#define YO_MAX_TAPER   20   /* maximum diameters per taper line */
#define YO_MAX_ELEMS  100   /* maximum elements per YO file     */
#define YO_MAX_SEGS     7   /* maximum NEC segments per GW (per element half) */
#define YO_MIN_SEGS     1

/* Speed of light in m/s, for wavelength calculation */
#define YO_C_M_S  2.998e8

/* Default conductivity assumed by Yagi Optimizer (6061-T6 aluminium, S/m) */
#define YO_DEFAULT_CONDUCTIVITY  2.52e7

/* -------------------------------------------------------------------------
 * Shared card-creation helpers (same pattern as maa-support.c)
 * ---------------------------------------------------------------------- */

/* -------------------------------------------------------------------------
 * String utilities
 * ---------------------------------------------------------------------- */

/* Strip leading/trailing whitespace in-place. */
static char *yo_trim(char *s)
{
    while (*s && isspace((unsigned char)*s)) s++;
    char *e = s + strlen(s);
    while (e > s && isspace((unsigned char)*(e-1))) *--e = '\0';
    return s;
}

/* Case-insensitive prefix match; returns pointer past the prefix or NULL. */
static const char *yo_iprefix(const char *line, const char *prefix)
{
    while (*prefix) {
        if (tolower((unsigned char)*line) != tolower((unsigned char)*prefix))
            return NULL;
        line++; prefix++;
    }
    return line;
}

/* -------------------------------------------------------------------------
 * Unit parsing
 * Returns the scale factor (metres per unit); 0.0 on failure.
 * Only the first two characters of the unit token are examined, per the
 * YO specification.
 * ---------------------------------------------------------------------- */

static double yo_unit_scale(const char *unit_str)
{
    char u[3] = {0,0,0};
    const char *p = unit_str;
    while (*p && isspace((unsigned char)*p)) p++;
    u[0] = tolower((unsigned char)*p);
    if (*(p+1)) u[1] = tolower((unsigned char)*(p+1));

    if (u[0] == 'm' && u[1] == 'm')     return 0.001;
    if (u[0] == 'c' && u[1] == 'm')     return 0.01;
    if (u[0] == 'm')                    return 1.0;     /* metres */
    if (u[0] == 'i' || u[0] == '"')     return 0.0254;  /* inches */
    if (u[0] == 'f' || u[0] == '\'')    return 0.3048;  /* feet   */
    if (u[0] == 'w')                    return 0.0;     /* wavelengths – frequency needed */
    return 0.0;
}

/* Parse a floating-point value that may be immediately followed by a unit
 * suffix rune (', ", or alpha chars).  Returns the value in base units
 * (i.e. the input unit), and advances *pp past the parsed token.
 * If a unit suffix is found and differs from base_scale, the value is
 * converted to metres and then divided by base_scale so it ends up in base
 * units.  If the suffix unit is the same as base_scale (or no suffix), the
 * raw value is returned unchanged.  base_scale == 0 disables conversion. */
static double yo_parse_value(const char **pp, double base_scale)
{
    char *end;
    double val = strtod(*pp, &end);
    if (end == *pp) { *pp = end; return 0.0; }  /* no number */
    /* check for unit suffix */
    const char *su = end;
    while (*su && isspace((unsigned char)*su)) su++;
    /* unit suffixes: ', ", or letter */
    if (*su == '\'' || *su == '"' || isalpha((unsigned char)*su)) {
        const char *unit_start = su;
        /* consume the unit token */
        if (*su == '\'' || *su == '"') { su++; }
        else { while (*su && (isalpha((unsigned char)*su))) su++; }
        char unit_tok[16] = {0};
        size_t ul = (size_t)(su - unit_start);
        if (ul > 15) ul = 15;
        strncpy(unit_tok, unit_start, ul);
        double unit_scale = yo_unit_scale(unit_tok);
        if (unit_scale > 0.0 && base_scale > 0.0 && fabs(unit_scale - base_scale) > 1e-12) {
            /* convert: val * unit_scale = metres; / base_scale = base units */
            val = val * unit_scale / base_scale;
        }
        *pp = su;
    } else {
        *pp = end;
    }
    return val;
}

/* -------------------------------------------------------------------------
 * Material lookup: return conductivity in S/m for a named material.
 * Returns 0.0 if not recognised.
 * ---------------------------------------------------------------------- */

static double yo_material_conductivity(const char *name)
{
    if (yo_iprefix(name, "silver"))         return 6.30e7;
    if (yo_iprefix(name, "copper"))         return 5.80e7;
    if (yo_iprefix(name, "aluminum"))       return 3.50e7;
    if (yo_iprefix(name, "aluminium"))      return 3.50e7;
    if (yo_iprefix(name, "6063"))           return 2.92e7;
    if (yo_iprefix(name, "brass"))          return 1.60e7;
    if (yo_iprefix(name, "phosphor"))       return 1.50e7;
    if (yo_iprefix(name, "steel"))          return 1.00e7;
    return 0.0;
}

/* -------------------------------------------------------------------------
 * Taper / element data structures
 * ---------------------------------------------------------------------- */

typedef struct {
    int    n_diams;                  /* number of taper sections (1..YO_MAX_TAPER) */
    double diam[YO_MAX_TAPER];       /* diameters at each section boundary        */
    int    spacing;                  /* 1 = following length lines are incremental */
} yo_taper_t;

typedef struct {
    double boom_pos;                 /* position along the boom (Z axis)           */
    double half_len[YO_MAX_TAPER];   /* half-length to each taper boundary (X axis) */
    int    n_sections;               /* number of non-zero taper sections defined   */
    int    taper_idx;                /* which taper definition applies              */
} yo_elem_t;

/**
 * @copydoc read_deck_yo
 */
int read_deck_yo(deck_t *deck, FILE *fp)
{
    if (!deck || !fp) return -1;

    char line[512];
    char title[64]   = "(untitled)";
    double freq_mhz  = 0.0;          /* first/centre frequency */
    double height    = 0.0;          /* antenna height in base units */
    double cond      = 0.0;          /* material conductivity, 0 = none */
    double stacked   = 0.0;          /* stacking separation in base units */
    double base_scale = 0.0254;      /* default = inches                 */


    /* --- taper definitions (up to 20 blocks, each triggered by a taper line) */
    yo_taper_t tapers[20];
    int        n_tapers = 0;

    /* --- element records */
    yo_elem_t  elems[YO_MAX_ELEMS];
    int        n_elems = 0;

    /* --- states while reading data lines */
    int    current_taper = -1;       /* index into tapers[] */

    /* cumulative boom position when spacing mode is active */
    double boom_cumul = 0.0;

    /* ---------------------------------------------------------------
     * Phase 1: read title line
     * ------------------------------------------------------------ */
    while (fgets(line, sizeof line, fp)) {
        char *t = yo_trim(line);
        if (t[0] == '\0') continue;       /* skip blank lines at top */
        /* title is the first non-blank line */
        size_t tl = strlen(t);
        if (tl > sizeof title - 1) tl = sizeof title - 1;
        strncpy(title, t, tl);
        title[tl] = '\0';
        break;
    }

    /* ---------------------------------------------------------------
     * Phase 2: read option lines, frequency line, geometry line, and
     * data lines, all in sequence.
     * The "section" we are in is inferred from the line content.
     *
     * States:
     *   0 = looking for frequency / geometry / options
     *   1 = looking for geometry line (found frequency)
     *   2 = reading data lines (found geometry)
     * ------------------------------------------------------------ */
    int state = 0;
    double lambda_in_units = 0.0;   /* wavelength in base units, set once freq + unit known */

    while (fgets(line, sizeof line, fp)) {
        char *t = yo_trim(line);
        if (t[0] == '\0') {
            if (state == 2) break;   /* blank line ends data section */
            continue;
        }

        /* ---- option lines (can appear before frequency) ---- */
        if (state == 0) {
            /* Height */
            const char *rest;
            if ((rest = yo_iprefix(t, "height")) != NULL && isspace((unsigned char)*rest)) {
                double hval = yo_parse_value(&rest, base_scale);
                if (hval != 0.0) height = hval;
                continue;
            }
            /* Stacked / Dual / KLM */
            if ((rest = yo_iprefix(t, "stacked")) != NULL) {
                stacked = yo_parse_value(&rest, base_scale);
                continue;
            }
            if (yo_iprefix(t, "dual") || yo_iprefix(t, "klm")) {
                /* dual/KLM: ignore phasing details, but note stacking */
                continue;
            }
            /* Material names */
            double mc = yo_material_conductivity(t);
            if (mc > 0.0) { cond = mc; continue; }
            /* Resistivity <val> */
            if ((rest = yo_iprefix(t, "resistivity")) != NULL && isspace((unsigned char)*rest)) {
                double rho = yo_parse_value(&rest, 0.0);
                if (rho > 0.0) cond = 1.0 / rho;
                continue;
            }
            /* Conductivity <val> [IACS] */
            if ((rest = yo_iprefix(t, "conductivity")) != NULL && isspace((unsigned char)*rest)) {
                double c = yo_parse_value(&rest, 0.0);
                /* IACS 1.0 = 5.80e7 S/m (copper) */
                if (c > 0.0) cond = c * 5.80e7;   /* assume IACS */
                continue;
            }

            /* ---- Frequency line: first character is a digit or '.' ---- */
            if (isdigit((unsigned char)t[0]) || t[0] == '.') {
                /* parse up to 4 frequencies, pull off optional unit suffix */
                double freqs[4] = {0,0,0,0};
                int nf = 0;
                const char *p = t;
                while (*p && nf < 4) {
                    while (*p && isspace((unsigned char)*p)) p++;
                    if (!*p || (!isdigit((unsigned char)*p) && *p != '.' && *p != '-')) break;
                    char *end;
                    double v = strtod(p, &end);
                    if (end == p) break;
                    freqs[nf++] = v;
                    p = end;
                }
                /* optional unit suffix after the numbers */
                while (*p && isspace((unsigned char)*p)) p++;
                double fscale = 1.0;  /* default = MHz */
                if (yo_iprefix(p, "ghz"))      fscale = 1000.0;
                else if (yo_iprefix(p, "khz")) fscale = 0.001;
                /* take the centre frequency (index 1 if multiple, else index 0) */
                if (nf >= 3)
                    freq_mhz = freqs[1] * fscale;
                else if (nf >= 1)
                    freq_mhz = freqs[0] * fscale;
                state = 1;
                continue;
            }
        }

        if (state == 1) {
            /* ---- Geometry line: "<N> elements[, <unit>]" ---- */
            const char *rest;
            if ((rest = yo_iprefix(t, "dual")) || (rest = yo_iprefix(t, "klm")) || (rest = yo_iprefix(t, "stacked"))) {
                /* can appear here too */
                continue;
            }
            /* Height option can occasionally appear late too */
            if ((rest = yo_iprefix(t, "height")) != NULL && isspace((unsigned char)*rest)) {
                double hval = yo_parse_value(&rest, 1.0); /* temporary */
                if (hval != 0.0) height = hval;
                continue;
            }
            /* look for integers followed by "elements" */
            char *end;
            long n = strtol(t, &end, 10);
            if (end != t && n > 0) {
                /* skip whitespace and "elements" keyword */
                const char *p = end;
                while (*p && isspace((unsigned char)*p)) p++;
                if (yo_iprefix(p, "elements")) {
                    p += 8;
                    while (*p && (isspace((unsigned char)*p) || *p == ',')) p++;
                    /* remaining text is the unit */
                    double sc = yo_unit_scale(p);
                    if (sc > 0.0) base_scale = sc;
                }
                /* compute wavelength in base units */
                if (freq_mhz > 0.0 && base_scale > 0.0)
                    lambda_in_units = (YO_C_M_S / (freq_mhz * 1e6)) / base_scale;
                state = 2;
                current_taper = -1;
                boom_cumul = 0.0;
            }
            continue;
        }

        if (state == 2) {
            /* ---- Data lines: taper or length ---- */
            /* compute sum of all numeric values on this line to classify */
            int spacing_flag = 0;
            const char *p = t;
            /* check for leading "spacing" keyword */
            const char *after_spacing = yo_iprefix(p, "spacing");
            if (after_spacing && (*after_spacing == '\0' || isspace((unsigned char)*after_spacing))) {
                spacing_flag = 1;
                p = after_spacing;
            }

            /* parse all numeric tokens on the line */
            double vals[YO_MAX_TAPER + 2];
            int nv = 0;
            double sum = 0.0;
            while (*p && nv < YO_MAX_TAPER + 2) {
                while (*p && isspace((unsigned char)*p)) p++;
                if (!*p) break;
                if (!isdigit((unsigned char)*p) && *p != '.' && *p != '-') break;
                double v = yo_parse_value(&p, base_scale);
                vals[nv++] = v;
                sum += fabs(v);
            }
            if (nv == 0) continue;  /* empty / comment line */

            /* ---- Classification: small sum → taper, large sum → length ----
             * The YO documentation states: "if the sum of all the numbers
             * on a given line are greater than .15 wavelengths, the line is
             * a taper line".  In practice this is inverted: element lengths
             * are large (>> 0.15λ) while diameters are tiny (<< 0.15λ).
             * We therefore treat a large sum as a LENGTH line and a small
             * sum as a TAPER line.  If no wavelength is available, fall
             * back to the spacing_flag as tiebreaker. */
            int is_taper;
            if (lambda_in_units > 0.0)
                is_taper = (sum <= 0.15 * lambda_in_units);
            else
                is_taper = spacing_flag || (nv > 0 && vals[0] < 1.0);

            if (is_taper) {
                /* --- taper line --- */
                if (n_tapers < 20) {
                    yo_taper_t *tp = &tapers[n_tapers];
                    memset(tp, 0, sizeof *tp);
                    tp->spacing = spacing_flag;
                    tp->n_diams = nv;
                    for (int k = 0; k < nv && k < YO_MAX_TAPER; k++)
                        tp->diam[k] = vals[k];
                    current_taper = n_tapers;
                    n_tapers++;
                    /* spacing mode resets cumulative boom position */
                    if (spacing_flag) boom_cumul = 0.0;
                }
            } else {
                /* --- length line --- */
                if (n_elems >= YO_MAX_ELEMS) continue;
                yo_elem_t *el = &elems[n_elems];
                memset(el, 0, sizeof *el);
                el->taper_idx = current_taper;

                /* first value is the boom position (Y in YO → Z axis in NEC) */
                double raw_pos = vals[0];
                if (current_taper >= 0 && tapers[current_taper].spacing) {
                    boom_cumul += raw_pos;
                    el->boom_pos = boom_cumul;
                } else {
                    el->boom_pos = raw_pos;
                }

                /* remaining values are half-lengths to each taper boundary */
                int ns = nv - 1;
                if (ns > YO_MAX_TAPER) ns = YO_MAX_TAPER;
                el->n_sections = ns;
                /* detect whole-element length: value > 0.3λ → divide by 2 */
                for (int k = 0; k < ns; k++) {
                    double hlen = vals[k + 1];
                    if (lambda_in_units > 0.0 && hlen > 0.3 * lambda_in_units)
                        hlen *= 0.5;
                    el->half_len[k] = hlen;
                }
                n_elems++;
            }
        }
    }

    /* ---------------------------------------------------------------
     * Phase 3: emit NEC cards
     * ------------------------------------------------------------ */

    /* CM + CE from title */
    {
        char buf[128];
        snprintf(buf, sizeof buf, "CM %s", title);
        append_card_from_text(deck, buf);
        append_card_from_text(deck, "CE");
    }

    /* Each element → GW cards */
    int tag = 1;
    for (int e = 0; e < n_elems; e++) {
        yo_elem_t *el = &elems[e];
        yo_taper_t *tp = (el->taper_idx >= 0 && el->taper_idx < n_tapers)
                         ? &tapers[el->taper_idx] : NULL;

        double z_boom   = el->boom_pos;
        double y_height = height;  /* in base units */

        int ns = el->n_sections;
        if (ns <= 0 || tp == NULL) {
            /* degenerate: no taper info, skip */
            continue;
        }

        /* number of non-zero section spans */
        int nonzero = 0;
        for (int k = 0; k < ns && k < tp->n_diams; k++)
            if (el->half_len[k] > 0.0) nonzero++;

        if (nonzero <= 1 || tp->n_diams <= 1) {
            /* --- Simple (non-tapered) element: one GW for full element ---
             * Sum all non-zero section spans to get the half-length; use the
             * diameter of the last (outermost) non-zero section. */
            double hlen = 0.0;
            double diam = (tp->n_diams >= 1) ? tp->diam[0] : 0.0;
            for (int k = 0; k < ns && k < tp->n_diams; k++) {
                if (el->half_len[k] > 0.0) {
                    hlen += el->half_len[k];
                    diam  = tp->diam[k];
                }
            }
            if (hlen <= 0.0) continue;
            double rad = diam * 0.5;
            char buf[256];
            snprintf(buf, sizeof buf,
                     "GW %d, 5, %.6f, %.6f, %.6f, %.6f, %.6f, %.6f, %.6f",
                     tag, -hlen, y_height, z_boom, hlen, y_height, z_boom, rad);
            append_card_from_text(deck, buf);
            tag++;
        } else {
            /* --- Tapered element: one GW per section, each half emitted ---
             * The half_len[] values are section SPANS (not cumulative from the
             * boom); accumulate them to obtain GW start/end boundaries.
             * Sections with span == 0 are skipped (that taper tier not used).
             * Determine segment count per section: 2 × active sections, clamped. */
            int segs_per_sect = 2 * nonzero;
            if (segs_per_sect < YO_MIN_SEGS) segs_per_sect = YO_MIN_SEGS;
            if (segs_per_sect > YO_MAX_SEGS) segs_per_sect = YO_MAX_SEGS;

            double prev = 0.0;
            for (int k = 0; k < ns && k < tp->n_diams; k++) {
                double span = el->half_len[k];
                double diam = tp->diam[k];
                if (span <= 0.0 || diam <= 0.0) continue;  /* skip unused tier */
                double boundary = prev + span;
                double rad = diam * 0.5;
                char buf[256];
                /* right half (positive X) */
                snprintf(buf, sizeof buf,
                         "GW %d, %d, %.6f, %.6f, %.6f, %.6f, %.6f, %.6f, %.6f",
                         tag, segs_per_sect,
                         prev, y_height, z_boom, boundary, y_height, z_boom, rad);
                append_card_from_text(deck, buf);
                tag++;
                /* left half (negative X, mirrored) */
                snprintf(buf, sizeof buf,
                         "GW %d, %d, %.6f, %.6f, %.6f, %.6f, %.6f, %.6f, %.6f",
                         tag, segs_per_sect,
                         -prev, y_height, z_boom, -boundary, y_height, z_boom, rad);
                append_card_from_text(deck, buf);
                tag++;
                prev = boundary;
            }
        }
    }

    /* GS — unit scale to metres */
    if (base_scale > 0.0) {
        char buf[80];
        snprintf(buf, sizeof buf, "GS 0, 0, %.8g", base_scale);
        append_card_from_text(deck, buf);
    }

    /* GE */
    append_card_from_text(deck, "GE");

    /* GM for stacking: translate second array along Z axis */
    if (stacked > 0.0) {
        /* tag all geometry created so far, duplicate it, translate by stacked */
        char buf[128];
        snprintf(buf, sizeof buf,
                 "GM 0, 1, 0, 0, 0, 0, 0, %.6f", stacked);
        append_card_from_text(deck, buf);
    }

    /* LD 5 for material conductivity */
    if (cond > 0.0) {
        char buf[80];
        snprintf(buf, sizeof buf,
                 "LD 5, 0, 0, 0, %.6g, 1.0", cond);
        append_card_from_text(deck, buf);
    }

    /* FR — NEC format: I1=IFRQ(0), I2=NFRQ(1), I3=IZPE(0), I4=NOPH(0), F1=FMHZ, F2=step(0) */
    if (freq_mhz > 0.0) {
        char buf[80];
        snprintf(buf, sizeof buf, "FR 0, 1, 0, 0, %.6f, 0", freq_mhz);
        append_card_from_text(deck, buf);
    }

    /* RP — full 3D far-field pattern (37 theta × 73 phi, 5° steps) */
    append_card_from_text(deck, "RP 0, 37, 73, 1000, 0, 0, 5, 5");

    /* EN */
    append_card_from_text(deck, "EN");

    return 0;
}

/**
 * @copydoc write_deck_yo
 */
int write_deck_yo(const deck_t *deck, FILE *fp)
{
    if (!deck || !fp) return -1;

    /* --- title from first CM card --- */
    const char *title = "";
    for (int i = 0; i < deck->num_cards; i++) {
        card_t *c = &deck->cards[i];
        if (strcmp(c->card_code, "CM") == 0 && c->comment && c->comment[0] != '\0') {
            title = c->comment;
            break;
        }
    }
    fprintf(fp, "%s\n", title);

    /* --- frequency from FR card --- */
    double freq = 14.0;
    for (int i = 0; i < deck->num_cards; i++) {
        card_t *c = &deck->cards[i];
        if (strcmp(c->card_code, "FR") == 0) {
            freq = c->f[1];
            break;
        }
    }
    fprintf(fp, "%.6f  MHz\n", freq);

    /* --- count GW wires --- */
    int nw = 0;
    for (int i = 0; i < deck->num_cards; i++)
        if (strcmp(deck->cards[i].card_code, "GW") == 0) nw++;

    /* --- unit scale from GS card (default: assume metres) --- */
    double gs_scale = 1.0;
    for (int i = 0; i < deck->num_cards; i++) {
        card_t *c = &deck->cards[i];
        if (strcmp(c->card_code, "GS") == 0) {
            gs_scale = c->f[3];
            break;
        }
    }

    /* convert scale to an output unit */
    const char *unit_name = "meters";
    if      (fabs(gs_scale - 0.0254) < 1e-6)  unit_name = "inches";
    else if (fabs(gs_scale - 0.3048) < 1e-5)  unit_name = "feet";
    else if (fabs(gs_scale - 0.01)   < 1e-9)  unit_name = "centimeters";
    else if (fabs(gs_scale - 0.001)  < 1e-10) unit_name = "millimeters";

    fprintf(fp, "%d elements, %s\n", nw, unit_name);

    /* --- collect unique radii (taper line) --- */
    /* simple heuristic: one taper line with all unique radii×2 */
    double diams[YO_MAX_TAPER];
    int    nd = 0;
    for (int i = 0; i < deck->num_cards && nd < YO_MAX_TAPER; i++) {
        card_t *c = &deck->cards[i];
        if (strcmp(c->card_code, "GW") != 0) continue;
        double d = c->f[7] * 2.0;          /* radius → diameter */
        int found = 0;
        for (int k = 0; k < nd; k++) if (fabs(diams[k] - d) < 1e-9) { found = 1; break; }
        if (!found && nd < YO_MAX_TAPER) diams[nd++] = d;
    }
    /* emit taper line */
    fprintf(fp, "          ");
    for (int k = 0; k < nd; k++) fprintf(fp, "  %.6f", diams[k]);
    fprintf(fp, "\n");

    /* --- length lines: one per GW, using the wire Y-coordinate as half-length
         and the Z-coordinate as boom position.  This convention matches what
         read_deck_yo produces (element axis = X, boom = Z, height = Y). --- */
    for (int i = 0; i < deck->num_cards; i++) {
        card_t *c = &deck->cards[i];
        if (strcmp(c->card_code, "GW") != 0) continue;
        /* x1,y1,z1, x2,y2,z2, radius stored in f[1..7] */
        double x1 = c->f[1], x2 = c->f[4];
        double z    = c->f[3];               /* Z = boom position */
        double half_len = fabs(x2 - x1) * 0.5;
        double diam = c->f[7] * 2.0;
        /* find which taper-column matches this diameter */
        int col = 0;
        for (int k = 0; k < nd; k++) if (fabs(diams[k] - diam) < 1e-9) { col = k; break; }
        fprintf(fp, "  %.6f", z);          /* boom position */
        for (int k = 0; k < nd; k++) {
            if (k == col) fprintf(fp, "  %.6f", half_len);
            else          fprintf(fp, "  0.000000");
        }
        fprintf(fp, "\n");
    }

    return 0;
}
