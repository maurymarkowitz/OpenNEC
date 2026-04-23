/******************************************************************************
 * output.c
 *
 * output.c contains a number of routines that write data from the deck to
 * various types of files. This includes the main output file in write_nec_out
 * which attempts to match the format of the nec2c .out files as closely as
 * possible.
 *
 * OpenNEC adds functions for writing the decks themselves, in .onec format.
 * in addition to allowing a deck to be created in code and then written.
 * These functions can also be used as a way to fix problems in existing files,
 * like split lines or non-standard comment markers and such, simply load up
 * the deck and then save it again.
 *
 *****************************************************************************/

#include "internals.h"
#include "output.h"
#include <stdint.h>
#include <math.h>

/* Forward declarations for internal write functions */
static void write_header(const context_t *ctx, const deck_t *deck, FILE *pfile);
static int write_structure(context_t *ctx, const deck_t *deck, FILE *pfile);
static int write_segments(context_t *ctx, const deck_t *deck, FILE *pfile);
static void write_patches(const context_t *ctx, const deck_t *deck, FILE *pfile);
static void write_input_cards_excluding_end(FILE *file, const context_t *ctx, const deck_t *deck, int batch_start, int batch_end, int card_number_offset);
static void write_frequency_data(FILE *file, const context_t *ctx);
static void write_loading_data(FILE *file, const context_t *ctx);
static void write_environment_data(FILE *file, const context_t *ctx);
static void write_matrix_timing(FILE *file, const context_t *ctx);
static void write_network_data(FILE *file, const context_t *ctx);
static void write_matrix_asymmetry(FILE *file, const context_t *ctx);
static void write_network_excitation(FILE *file, const context_t *ctx);
static void write_antenna_input_parameters(FILE *file, const context_t *ctx);
static void write_coupling_data(context_t *ctx);
static void write_currents(FILE *file, const context_t *ctx);
static void write_power_budget(FILE *file, const context_t *ctx);
static void write_radiation_pattern_header(FILE *file, const context_t *ctx);
static void write_radiation_pattern_data(FILE *file, const context_t *ctx);
static void write_average_power_gain(FILE *file, const context_t *ctx);
static void write_normalized_gain(FILE *file, const context_t *ctx);
static void write_near_field_data(FILE *file, const context_t *ctx);
static void write_near_field_plot(const context_t *ctx);

/******************************************************************************
 * Output Format Specification
 *
 * Centralized specification table for all formatting differences between
 * nec2c format and original Fortran format.
 */
typedef struct {
    const char *header_separator;      /* Section header separator string */
    const char *frequency_label;       /* "FREQUENCY :" vs "FREQUENCY=" */
    const char *wavelength_label;      /* "WAVELENGTH:" vs "WAVELENGTH=" */
    const char *length_units;          /* "Mtr" vs "METERS" */
    const char *freq_units;            /* "MHz" vs "MHZ" */
    const char *matrix_sep;            /* "---------- MATRIX TIMING ----------" vs "--------- MATRIX TIMING ---------" */
    const char *matrix_fill_format;    /* "FILL: %d msec" vs "FILL= %8.3f SEC." */
    const char *matrix_factor_format;  /* "FACTOR: %d msec" vs "FACTOR= %8.3f SEC." */
    int use_seconds_for_timing;        /* 1 for Fortran (seconds), 0 for nec2c (milliseconds) */
    const char *loading_all_tag;       /* "ALL" vs "0" when consolidating universal loads */
    int pre_freq_blank_lines;          /* Number of blank lines before frequency header */
} output_format_spec_t;

static const output_format_spec_t format_specs[] = {
    /* NEC2C format (index 0 = OUTPUT_FORMAT_NEC2C) */
    {
        .header_separator = "--------- ",
        .frequency_label = "FREQUENCY :",
        .wavelength_label = "WAVELENGTH:",
        .length_units = "Mtr",
        .freq_units = "MHz",
        .matrix_sep = "---------- MATRIX TIMING ----------",
        .matrix_fill_format = "FILL: %d msec",
        .matrix_factor_format = "FACTOR: %d msec",
        .use_seconds_for_timing = 0,
        .loading_all_tag = "0",
        .pre_freq_blank_lines = 2
    },
    /* ORIGINAL Fortran format (index 1 = OUTPUT_FORMAT_ORIGINAL) */
    {
        .header_separator = "- - - - - - ",
        .frequency_label = "FREQUENCY=",
        .wavelength_label = "WAVELENGTH=",
        .length_units = "METERS",
        .freq_units = "MHZ",
        .matrix_sep = "- - - MATRIX TIMING - - -",
        .matrix_fill_format = "FILL=%8.3f SEC.,",
        .matrix_factor_format = "FACTOR=%8.3f SEC.",
        .use_seconds_for_timing = 1,
        .loading_all_tag = "ALL",
        .pre_freq_blank_lines = 4
    }
};

static inline const output_format_spec_t* get_format(const context_t *ctx)
{
    int fmt = ctx->output_format;
    if (fmt < 0 || fmt >= (int)(sizeof(format_specs)/sizeof(format_specs[0])))
        fmt = DEFAULT_OUTPUT_FORMAT;
    return &format_specs[fmt];
}

/******************************************************************************
 * format_coord
 *
 * Formats a coordinate value, converting negative zero to positive zero.
 * Uses sprintf to detect and fix -0.00000 format issues.
 */
static void format_coord(char *buf, int len, double value, const char *fmt)
{
    snprintf(buf, len, fmt, value);
    // Find the position of the first non-space character
    int i = 0;
    while (i < len && buf[i] == ' ')
        i++;
    
    // If we found "-0." at position i, check if all digits after are zeros
    if (i < len - 2 && buf[i] == '-' && buf[i+1] == '0' && buf[i+2] == '.')
    {
        // Check if all remaining digits are zero
        int all_zero = 1;
        int j;
        for (j = i + 3; j < len && buf[j]; j++)
        {
            if (buf[j] != '0')
            {
                all_zero = 0;
                break;
            }
        }
        if (all_zero)
        {
            buf[i] = ' ';  // Replace minus with space for proper alignment
        }
    }
}

/******************************************************************************
 * is_inline_formula
 *
 * Returns true if the formula key (e.g. "F7", "I3") corresponds to a field
 * that was originally written inline (i.e. the value is already emitted as
 * part of the card's field list and must not be repeated in the onec comment).
 */
static bool is_inline_formula(const card_t *card, const char *key)
{
  if (!key || strlen(key) != 2)
    return false;
  int idx = key[1] - '0';
  if (key[0] == 'F' && idx >= 1 && idx <= MAX_FLT_FIELDS)
    return card->flt_form_inline[idx];
  if (key[0] == 'I' && idx >= 1 && idx <= MAX_INT_FIELDS)
    return card->int_form_inline[idx];
  return false;
}

/******************************************************************************
 * write_deck_onec
 *
 * Writes a deck in the onec format, which is basically everything in the
 * deck. This will cause the deck to be written in cannoical onec format,
 * so reading in a deck and then writing it back out may result in
 * differences in ordering of options, spacing, separators being stripped,
 * etc. This is by design.
 *
 */
void write_deck_onec(const context_t *ctx, const deck_t *deck, FILE *file)
{
  card_t *card;
  int MAX_FLTS, MAX_INTS;

  for (int i = 0; i < deck->num_cards; i++)
  {
    card = &deck->cards[i];

    // if we are past the EN at the end of the deck, write out the whole string
    if (i > deck->deck_end)
    {
      fputs(card->card_str, file);
      fputc('\n', file);
      continue;
    }

    // comment cards care also easy
    if (is_comment(card))
    {
      fputs(card->card_code, file);
      fputs(card->comment, file);
      fputc('\n', file);
      continue;
    }

    // the ONEC cards like SY are also generally simple
    if (is_extension(card))
    {
      // NOTE: extn_code is the inline comment *separator* found in the original
      // line (e.g. '!' or '\'').  Do NOT write it as a leading prefix before the
      // card mnemonic — that would make read_deck treat the line as a
      // commented-out (ignored) hidden card on the next round-trip.
      fputs(card->card_code, file);

      key_value_t *head = card->formulas;
      while (head != NULL)
      {
        // whitespace and the key
        fputs(" ", file);
        fputs(head->key, file);
        // use the separator they used, or default to = because it's likely an SY
        if (strcmp(&head->separator, "") != 0)
        {
          fputc(head->separator, file);
        }
        else
        {
          fputc('=', file);
        }
        // now the value
        fputs(head->value, file);

        // move to the next pair, adding a comment if there is another
        head = head->next;
        if (head != NULL)
          fputs(",", file);
      }
      // is there also a comment?  Use the original separator char if known.
      // For SY cards the inline comment lands in extn_str (not comment), so check both.
      const char *cmt_text = (card->comment != NULL && strlen(card->comment) > 0)
                               ? card->comment
                               : card->extn_str;
      if (cmt_text != NULL && strlen(cmt_text) > 0)
      {
        char sep = (card->extn_code[0] != '\0') ? card->extn_code[0] : '!';
        fputc(' ', file);
        fputc(sep, file);
        fputs(cmt_text, file);
      }
      fputc('\n', file);
      continue;
    }

    // all the rest of the cards have multiple parts to put together

    // for commented-out cards, write the leading marker before the code
    if (card->cmt_code[0] != '\0')
    {
      fputc(card->cmt_code[0], file);
    }
    // start with the card code
    fputs(card->card_code, file);

    // Determine the field separator to use for this card.
    // Priority: card's own detected style, then deck-wide consensus.
    // FSEP_COLUMN_ALIGNED uses a tab — an approximation that is visually similar
    // and better than collapsing everything to a single space.
    // FSEP_SPACE_COMMA: space before first field, commas between the rest.
    field_sep_t fsep_type = (card->field_sep != FSEP_UNKNOWN)
                              ? card->field_sep : deck->field_sep;
    const char *fsep_first = " ";  // separator between mnemonic and field 1
    const char *fsep_rest  = " ";  // separator between subsequent fields
    if (fsep_type == FSEP_TAB || fsep_type == FSEP_COLUMN_ALIGNED) {
      fsep_first = fsep_rest = "\t";
    } else if (fsep_type == FSEP_COMMA) {
      fsep_first = fsep_rest = ",";
    } else if (fsep_type == FSEP_SPACE_COMMA) {
      fsep_first = " ";
      fsep_rest  = ",";
    }

    // get the number of fields for this sort of card
    MAX_INTS = max_int_fields(card);
    MAX_FLTS = max_flt_fields(card);

    // int fields depending on the card type
    if (is_control(card) || is_geometry(card))
    {
      // there is one special case in the integers, if it is a GX card the second
      // integer has to writen as a three digit number
      if (strcmp(card->card_code, "GX") == 0)
      {
        fprintf(file, "%s%d", fsep_first, card->i[1]);
        fprintf(file, "%s%3d", fsep_rest, card->i[2]);
      }
      // other cards might have a formula
      else
      {
        int field_num = 0;
        for (int j = 1; j <= card->ints_used && j <= MAX_INTS; j++)
        {
          const char *fsep = (field_num++ == 0) ? fsep_first : fsep_rest;
          // Look up formula for this integer field (fields are 1-based: I1..I4)
          char key[8];
          snprintf(key, sizeof(key), "I%d", j);
          const char *formula = lookup_formula(card, key);

          if (formula != NULL && is_inline_formula(card, key))
          {
            fprintf(file, "%s%s", fsep, formula);
          }
          else
          {
            fprintf(file, "%s%d", fsep, card->i[j]);
          }
        }
      }

      // floats are a number or a formula (fields are 1-based: F1..F7)
      {
        int field_num = (card->ints_used > 0 || strcmp(card->card_code, "GX") == 0) ? 1 : 0;
        for (int j = 1; j <= card->flts_used && j <= MAX_FLTS; j++)
        {
          const char *fsep = (field_num++ == 0) ? fsep_first : fsep_rest;
          // Look up formula for this float field
          char key[8];
          snprintf(key, sizeof(key), "F%d", j);
          const char *formula = lookup_formula(card, key);

          if (formula != NULL && is_inline_formula(card, key))
          {
            fprintf(file, "%s%s", fsep, formula);
          }
          else
          {
            fprintf(file, "%s%G", fsep, card->f[j]);
          }
        }
      }

      // the basic NEC fields are output, now see if there's anything after that

      // Compute hasOnec first — needed to decide whether extn_str is safe to use
      // as a plain comment fallback (see comment_text below).
      bool hasOnec = false;
      // only treat ignore as an onec annotation if it's the annotated form (not prefix-commented)
      if (card->ignore && card->cmt_code[0] == '\0')
        hasOnec = true;
      if (card->invisible)
        hasOnec = true;
      if (card->extensns != NULL)
        hasOnec = true;
      // only flag hasOnec for formulas that are NOT already emitted inline as card fields
      if (card->formulas != NULL)
      {
        key_value_t *f = card->formulas;
        while (f != NULL)
        {
          if (!is_inline_formula(card, f->key))
          {
            hasOnec = true;
            break;
          }
          f = f->next;
        }
      }

      // Determine the effective comment text.
      // card->comment is set by parse_key_values() when a "comment:" key was found.
      // card->extn_str holds the raw tail after any inline '!' / '\'' marker.
      // When hasOnec == false a plain inline comment (e.g. "GW 1 5 ... 0.01 ! my dipole")
      // lives only in extn_str — fall back to it so round-trips preserve plain comments.
      // When hasOnec IS true, extn_str is the full raw extension string already parsed
      // into individual pieces; do NOT re-emit it raw or fields will be duplicated.
      const char *comment_text = (card->comment != NULL && strlen(card->comment) > 0)
                                   ? card->comment
                                   : (!hasOnec ? card->extn_str : NULL);
      bool hasComment = (comment_text != NULL && strlen(comment_text) > 0);

      // if we found anything, print the comment marker found on this
      // card, the global one in the deck, or the onec default, !
      // NOTE: extn_code is char[1] (no null terminator), so use fputc not fputs.
      if (hasComment || hasOnec)
      {
        fputc(' ', file);
        if (card->extn_code[0] != '\0')
        {
          fputc(card->extn_code[0], file);
        }
        else if (deck->extn_code != 0)
        {
          fputc(deck->extn_code, file);
        }
        else
        {
          fputc('!', file);
        }
      }

      // if we have *only* a comment, just print that and we're done,
      // otherwise we have to export the fields one by one
      if (hasComment && !hasOnec)
      {
        fputs(comment_text, file);
      }
      else
      {
        if (card->ignore && card->cmt_code[0] == '\0')
        {
          fputs(" ignore:true", file);
        }
        if (card->invisible)
        {
          fputs(" invisible:true", file);
        }
        // formulas next - only the ones that aren't inline (inline ones are already in the card fields)
        if (card->formulas != NULL)
        {
          key_value_t *form = card->formulas;
          while (form != NULL)
          {
            if (!is_inline_formula(card, form->key))
            {
              fputc(' ', file);
              fputs(form->key, file);
              fputc('=', file);
              fputs(form->value, file);
            }
            form = form->next;
          }
        }
        // any other key/value pairs
        if (card->extensns != NULL)
        {
          /* walk the list and print any extensions except "invisible"; also
             strip out unwanted invisible entries if the flag is false. */
          key_value_t **pp = &card->extensns;
          while (*pp)
          {
            key_value_t *pair = *pp;
            if (pair->key && strcasecmp(pair->key, "invisible") == 0)
            {
              if (!card->invisible)
              {
                /* drop this pair entirely */
                *pp = pair->next;
                free(pair->key);
                free(pair->value);
                free(pair);
                continue;
              }
              else
              {
                /* keep it in the list but do not print; flag output above
                   will generate the correct text */
                pp = &pair->next;
                continue;
              }
            }

            fputc(' ', file);
            fputs(pair->key, file);
            fputc('=', file);
            fputs(pair->value, file);
            pp = &pair->next;
          }
        }
        // and then finally the comment which has to be at the end of the line
        if (card->comment != NULL && strlen(card->comment) > 0)
        {
          fputs(" comment:", file);
          fputs(card->comment, file);
        }
      }
      // close the line
      fputc('\n', file);
    } /* if command or geometry */
  } /* for over cards */
}

/******************************************************************************
 * write_coupling_data()
 *
 * Renders the CP (coupling) isolation table accumulated by compute_coupling() in
 * calculations.c.  No-op if no coupling rows were recorded.
 */
static void write_coupling_data(context_t *ctx)
{
  if (ctx->yparm.num_coupling_rows == 0)
    return;

  fprintf(ctx->output_fp, "\n\n\n"
                          "                        -----------"
                          " ISOLATION DATA -----------\n\n"
                          " ------- COUPLING BETWEEN ------     MAXIMUM    "
                          " ---------- FOR MAXIMUM COUPLING ----------\n"
                          "            SEG              SEG    COUPLING  LOAD"
                          " IMPEDANCE (2ND SEG)         INPUT IMPEDANCE \n"
                          " TAG  SEG   No:   TAG  SEG   No:      (DB)       "
                          " REAL     IMAGINARY         REAL       IMAGINARY");

  for (int i = 0; i < ctx->yparm.num_coupling_rows; i++)
  {
    coupling_row_t *r = &ctx->yparm.coupling_rows[i];
    if (!r->is_error)
    {
      fprintf(ctx->output_fp, "\n"
                              " %4d %4d %5d  %4d %4d %5d  %9.3f"
                              "  %12.5E %12.5E  %12.5E %12.5E",
              r->tag1, r->seg1, r->segno1,
              r->tag2, r->seg2, r->segno2,
              r->coupling_db,
              r->zl_real, r->zl_imag, r->zin_real, r->zin_imag);
    }
    else
    {
      fprintf(ctx->output_fp, "\n"
                              " %4d %4d %5d   %4d %4d %5d  **ERROR** "
                              "COUPLING IS NOT BETWEEN 0 AND 1. (= %12.5E)",
              r->tag1, r->seg1, r->segno1,
              r->tag2, r->seg2, r->segno2,
              r->c_value);
    }
  }
}

/******************************************************************************
 * write_nec_output()
 *
 * Writes a standard NEC-style output file, using various work functions.
 *
 */

/******************************************************************************
 * write_nec_preamble()
 *
 * Writes the one-time geometry header section: file header, structure
 * specification, segments, patches, and input cards.  Called once per
 * simulation section, before the frequency loop begins.
 */
void write_nec_preamble(context_t *ctx, const deck_t *deck, FILE *file)
{
  /* Original Fortran format starts with page control character '1' (form feed) */
  if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL)
    fprintf(file, "1\n");
    
  write_header(ctx, deck, file);
  write_structure(ctx, deck, file);
  write_segments(ctx, deck, file);
  write_patches(ctx, deck, file);
  /* Write input cards excluding EN and NX cards (they're output separately at end) */
  /* Use the last card index if deck_end is invalid or before geometry end */
  int batch_end = deck->deck_end;
  if (batch_end < 0 || batch_end >= deck->num_cards || batch_end <= deck->geometry_end) {
    batch_end = deck->num_cards - 1;
  }
  write_input_cards_excluding_end(file, ctx, deck, deck->geometry_end + 1, batch_end, 0);
}

/******************************************************************************
 * write_frequency_step_output()
 *
 * Writes all per-frequency-step output sections (frequency data, loading,
 * currents, power budget, radiation patterns, near field).  Called once at
 * the end of each frequency step in execute_frequency_loop().
 */
void write_frequency_step_output(FILE *file, context_t *ctx)
{
  write_frequency_data(file, ctx);
  write_loading_data(file, ctx);
  write_environment_data(file, ctx);
  write_matrix_timing(file, ctx);
  write_network_data(file, ctx);
  write_matrix_asymmetry(file, ctx);
  write_network_excitation(file, ctx);
  write_antenna_input_parameters(file, ctx);
  write_coupling_data(ctx);
  write_currents(file, ctx);
  write_power_budget(file, ctx);
  write_radiation_pattern_header(file, ctx);
  write_radiation_pattern_data(file, ctx);
  write_average_power_gain(file, ctx);
  write_normalized_gain(file, ctx);
  write_near_field_data(file, ctx);
  write_near_field_plot(ctx);
}

/*
 * write_extra_pattern_output()
 *
 * Writes only the radiation-pattern or near-field section, without repeating
 * the frequency header, loading, timing, or power budget.  Called when a
 * second RP/NE/NH card appears without an intervening FR — mirrors nec2c's
 * igo==4→5→6 path.
 */
void write_extra_pattern_output(FILE *file, context_t *ctx)
{
  write_radiation_pattern_header(file, ctx);
  write_radiation_pattern_data(file, ctx);
  write_average_power_gain(file, ctx);
  write_normalized_gain(file, ctx);
  write_near_field_data(file, ctx);
  write_near_field_plot(ctx);
}

void write_nec_output(context_t *ctx, const deck_t *deck, FILE *file)
{
  if (ctx->freq_step_output_written) {
    /* Per-step output (preamble + per-frequency sections) was already written
     * inside execute_frequency_loop().  Only write the trailing footer. */
    write_footer(file, ctx, deck);
    return;
  }

  /* Single-frequency or legacy path: write everything in one pass. */
  write_nec_preamble(ctx, deck, file);
  write_frequency_step_output(file, ctx);
  write_footer(file, ctx, deck);
}

/******************************************************************************
 * NEC-2 Fortran NGF/WGF Format
 *
 * OpenNEC reads and writes the original NEC-2 Fortran unformatted sequential
 * binary format for Numerical Green's Function (NGF/WGF) files. This is the
 * same format produced by the WG card in the original Fortran NEC-2 and is
 * compatible with files from 4nec2, the Cebik collection, and other tools.
 *
 * Fortran unformatted records: each WRITE statement produces one record:
 *   [int32 N][...N bytes of data...][int32 N]
 *
 * Record sequence (Fortran NEC-2 NGFWRT subroutine):
 *
 *   Rec 1 (88 bytes):
 *     N(i32), NP(i32), M(i32), MP(i32),
 *     WLAM(f64), FMHZ(f64),
 *     IPSYM(i32), KSYMP(i32), IPERF(i32), NRADL(i32),
 *     EPSR(f64), SIG(f64), SCRWLT(f64), SCRWRT(f64),
 *     NLOAD(i32), KCOM(i32)
 *
 *   Rec 2 (3*N*8 bytes, if N > 0):  X[N], Y[N], Z[N]  midpoints in wavelengths
 *   Rec 3 (3*N*8 bytes, if N > 0):  SI[N], BI[N], ALP[N]  len/radius in lambda, cab
 *   Rec 4 (2*N*8 bytes, if N > 0):  BET[N], SALP[N]  sab and salp direction cosines
 *   Rec 5 (2*N*4 bytes, if N > 0):  ICON1[N], ICON2[N]
 *   Rec 6 (  N*4 bytes, if N > 0):  ITAG[N]
 *   (optional: ZARRAY[N] if NLOAD > 0 — skipped on read, not written)
 *
 *   Rec 7 (32 bytes): ICASE, NBLOKS, NPBLK, NLAST, NBLSYM, NPSYM, NLSYM, IMAT
 *   (optional: patch coefficients if IPERF==2 — skipped on read, not written)
 *   (optional: SSX block if NOP > 1 — skipped on read, not written)
 *
 *   Rec 8 (4*NEQ + 800 bytes): IP[NEQ] (pivots), COM[100] (comments)
 *
 *   Rec 9 (IOUT*16 bytes): CM matrix as COMPLEX*16 (= C double complex),
 *          where IOUT = NEQ*NPEQ for ICASE <= 2
 *
 * On GF read, X/Y/Z/SI/BI are multiplied by WLAM (wavelengths -> metres),
 * then wire endpoints are reconstructed from midpoint + direction cosines.
 ******************************************************************************/

/* ---------------------------------------------------------------------------
 * Fortran unformatted record I/O helpers
 * -------------------------------------------------------------------------*/

/* Write a Fortran record from one buffer: [int32 len][data][int32 len] */
static bool fw1(FILE *f, const void *data, int32_t nbytes)
{
  if (fwrite(&nbytes, 4, 1, f) != 1)
    return false;
  if (fwrite(data, 1, (size_t)nbytes, f) != (size_t)nbytes)
    return false;
  if (fwrite(&nbytes, 4, 1, f) != 1)
    return false;
  return true;
}

/* Write a Fortran record from two discontiguous buffers */
static bool fw2(FILE *f,
                const void *a, int32_t alen,
                const void *b, int32_t blen)
{
  int32_t total = alen + blen;
  if (fwrite(&total, 4, 1, f) != 1)
    return false;
  if (fwrite(a, 1, (size_t)alen, f) != (size_t)alen)
    return false;
  if (fwrite(b, 1, (size_t)blen, f) != (size_t)blen)
    return false;
  if (fwrite(&total, 4, 1, f) != 1)
    return false;
  return true;
}

/* Write a Fortran record from three discontiguous buffers */
static bool fw3(FILE *f,
                const void *a, int32_t alen,
                const void *b, int32_t blen,
                const void *c, int32_t clen)
{
  int32_t total = alen + blen + clen;
  if (fwrite(&total, 4, 1, f) != 1)
    return false;
  if (fwrite(a, 1, (size_t)alen, f) != (size_t)alen)
    return false;
  if (fwrite(b, 1, (size_t)blen, f) != (size_t)blen)
    return false;
  if (fwrite(c, 1, (size_t)clen, f) != (size_t)clen)
    return false;
  if (fwrite(&total, 4, 1, f) != 1)
    return false;
  return true;
}

/* Read a Fortran record, validating that its length == expected bytes */
static bool fr1(FILE *f, void *buf, int32_t expected)
{
  int32_t n, n2;
  if (fread(&n, 4, 1, f) != 1)
    return false;
  if (n != expected)
    return false;
  if (fread(buf, 1, (size_t)n, f) != (size_t)n)
    return false;
  if (fread(&n2, 4, 1, f) != 1)
    return false;
  return n2 == n;
}

/* Read a Fortran record of any length into a freshly allocated buffer.
 * Caller must free *out on success. Returns false on I/O error. */
static bool fr_alloc(FILE *f, void **out, int32_t *out_len)
{
  int32_t n, n2;
  if (fread(&n, 4, 1, f) != 1)
    return false;
  *out_len = n;
  *out = malloc((size_t)n);
  if (!*out)
    return false;
  if (fread(*out, 1, (size_t)n, f) != (size_t)n)
  {
    free(*out);
    return false;
  }
  if (fread(&n2, 4, 1, f) != 1)
  {
    free(*out);
    return false;
  }
  if (n2 != n)
  {
    free(*out);
    return false;
  }
  return true;
}

/* Skip a Fortran record entirely */
static bool fr_skip(FILE *f)
{
  int32_t n, n2;
  if (fread(&n, 4, 1, f) != 1)
    return false;
  if (fseek(f, (long)n, SEEK_CUR) != 0)
    return false;
  if (fread(&n2, 4, 1, f) != 1)
    return false;
  return n2 == n;
}

/******************************************************************************
 * write_greens_binary()
 *
 * Writes an NGF/WGF file in NEC-2 Fortran unformatted sequential format,
 * compatible with the original NEC-2 Fortran WG card output.
 *
 * At call time the geometry arrays (x, y, z, si, bi) are in wavelength units
 * (scaled by fr = fmhz/CVEL during the frequency loop). The Fortran NEC-2
 * format stores them in wavelength units, so no conversion is needed here.
 *
 * @param file  Output file (opened in binary mode).
 * @param ctx   Simulation context.
 * @param neq   Number of equations (matrix dimension = N for wire-only).
 * @param cm    Unfactored CM matrix, column-major, neq*neq complex doubles.
 * @return      true on success, false on I/O error.
 */
bool write_greens_binary(FILE *file, const context_t *ctx,
                         int neq, const complex double *cm)
{
  if (!file || !cm || neq <= 0)
    return false;

  const int N = ctx->geometry.num_segs;
  const int NP = ctx->geometry.num_segs_sym; /* segs per symmetry copy (= N for no symmetry) */
  const int M = ctx->geometry.num_patches;
  const int MP = ctx->geometry.num_patches_sym;

  /* ---------- Record 1: header (88 bytes) ---------- */
  {
    uint8_t rec[88];
    uint8_t *p = rec;
#define AP4(v)                 \
  do                           \
  {                            \
    int32_t _v = (int32_t)(v); \
    memcpy(p, &_v, 4);         \
    p += 4;                    \
  } while (0)
#define AP8(v)               \
  do                         \
  {                          \
    double _v = (double)(v); \
    memcpy(p, &_v, 8);       \
    p += 8;                  \
  } while (0)
    AP4(N);
    AP4(NP);
    AP4(M);
    AP4(MP);
    AP8(ctx->geometry.wavelength);
    AP8(ctx->save.freq_mhz);
    AP4(ctx->geometry.symmetry_flag);
    AP4(ctx->gnd.has_ground);
    /* Write IPERF <= 1: OpenNEC does not write a patch-coefficient record
     * for finite-ground (iperf==2).  The reader will try to skip that record
     * when IPERF==2 in the header, so cap it at 1 so the skip is never
     * attempted when reading back an OpenNEC-written NGF file.
     * (Fortran-generated .wgf files with IPERF==2 still work because they
     *  contain the patch-coefficient record that the read-side fr_skip needs.) */
    AP4(ctx->gnd.is_perfect > 1 ? 1 : ctx->gnd.is_perfect);
    AP4(ctx->gnd.num_radials);
    AP8(ctx->save.ground_epsr);
    AP8(ctx->save.ground_sigma);
    AP8(ctx->gnd.screen_wire_len);
    AP8(ctx->gnd.screen_wire_radius);
    AP4(0); /* NLOAD — loads not stored in NGF */
    AP4(0); /* KCOM  — comments not stored in NGF */
#undef AP4
#undef AP8
    if (!fw1(file, rec, 88))
      return false;
  }

  /* ---------- Records 2-6: segment geometry ----------
   * x/y/z/si/bi are in wavelength units at call time — written as-is.
   * Direction cosines (cab/sab/salp) are dimensionless — written as-is.
   * ICON1/ICON2/ITAG cast to int32_t (= Fortran INTEGER*4). */
  if (N > 0)
  {
    int32_t n8 = (int32_t)(N * 8); /* bytes in one double[N] array */
    int32_t n4 = (int32_t)(N * 4); /* bytes in one  int32[N] array */

    /* Rec 2: X[N], Y[N], Z[N] */
    if (!fw3(file, ctx->geometry.x_center, n8,
             ctx->geometry.y_center, n8,
             ctx->geometry.z_center, n8))
      return false;

    /* Rec 3: SI[N], BI[N], ALP[N]  (ALP = cab = x-direction cosine) */
    if (!fw3(file, ctx->geometry.half_len, n8,
             ctx->geometry.radius, n8,
             ctx->geometry.dir_cos_x, n8))
      return false;

    /* Rec 4: BET[N], SALP[N]  (BET = sab, SALP = salp) */
    if (!fw2(file, ctx->geometry.dir_cos_y, n8,
             ctx->geometry.dir_cos_z, n8))
      return false;

    /* Rec 5: ICON1[N], ICON2[N] — OpenNEC stores as int; cast to int32_t */
    {
      int32_t *tmp = (int32_t *)malloc(2 * (size_t)N * sizeof(int32_t));
      if (!tmp)
        return false;
      for (int i = 0; i < N; i++)
        tmp[i] = (int32_t)ctx->geometry.seg_end1_conn[i];
      for (int i = 0; i < N; i++)
        tmp[N + i] = (int32_t)ctx->geometry.seg_end2_conn[i];
      bool ok = fw1(file, tmp, n4 * 2);
      free(tmp);
      if (!ok)
        return false;
    }

    /* Rec 6: ITAG[N] */
    {
      int32_t *tmp = (int32_t *)malloc((size_t)N * sizeof(int32_t));
      if (!tmp)
        return false;
      for (int i = 0; i < N; i++)
        tmp[i] = (int32_t)ctx->geometry.tag_nums[i];
      bool ok = fw1(file, tmp, n4);
      free(tmp);
      if (!ok)
        return false;
    }
  }

  /* ---------- Record 7: matrix blocking parameters (32 bytes) ----------
   * ICASE=1: full complex in-core matrix (no symmetry, no patches). */
  {
    int32_t IMAT = (int32_t)(neq * NP);
    int32_t rec7[8] = {
        1,            /* ICASE  = 1 */
        1,            /* NBLOKS = 1 */
        (int32_t)neq, /* NPBLK */
        (int32_t)neq, /* NLAST */
        1,            /* NBLSYM */
        (int32_t)NP,  /* NPSYM */
        (int32_t)NP,  /* NLSYM */
        IMAT          /* IMAT = NEQ * NPEQ */
    };
    if (!fw1(file, rec7, 32))
      return false;
  }

  /* ---------- Optional symmetry submatrix (when NP < N) ----------------------
   * Fortran NEC-2 GFOUT: IF(NOP.GT.1) WRITE(IGFL) ((SSX(I,J),I=1,NOP),J=1,NOP)
   * Fortran NEC-2 GFIL:  IF(NOP.GT.1) READ (IGFL) ((SSX(I,J),I=1,NOP),J=1,NOP)
   * where NOP = NEQ/NPEQ (number of symmetry copies) and SSX is the NOP×NOP
   * scattering / DFT matrix used in the symmetry-expanded solve.
   *
   * OpenNEC's reader calls fr_skip() (size-agnostic), but Fortran NEC-2 does a
   * typed READ of exactly NOP×NOP values.  We therefore write the real SSX matrix
   * so that an OpenNEC-written .ngf is readable by Fortran NEC-2.
   *
   * Two cases, matching Fortran FBLOCK (lines 3936-3961 of nec2-1.2.1.2.f):
   *   ipsym <= 0  (GR, rotational): SSX[i][j] = exp(2πi·i·j / NOP)  (full DFT)
   *   ipsym >  0  (GX, plane):  Recursive Hadamard construction, NOP ∈ {2,4,8}
   *
   * Note: the Fortran loop (DO I=2,NOP; DO J=I,NOP) omits the first row/column,
   * relying on COMMON-block zero-initialization.  The true DFT first row/col is
   * all-ones; we fill the full NOP×NOP matrix correctly. */
  if (NP > 0 && neq / NP > 1)
  {
    int NOP = neq / NP;
    complex double *ssx = (complex double *)calloc((size_t)NOP * (size_t)NOP,
                                                   sizeof(complex double));
    if (!ssx)
      return false;

    if (ctx->geometry.symmetry_flag <= 0)
    {
      /* Rotational symmetry (GR): NOP-point DFT matrix.
       * SSX[i][j] = exp(2πi * i * j / NOP)  for i,j = 0..NOP-1 (0-indexed). */
      double phaz = 2.0 * M_PI / NOP;
      for (int i = 0; i < NOP; i++)
      {
        for (int j = 0; j < NOP; j++)
        {
          double arg = phaz * i * j;
          ssx[i * NOP + j] = cos(arg) + I * sin(arg);
        }
      }
    }
    else
    {
      /* Plane symmetry (GX, NOP = 2 / 4 / 8): Hadamard-like construction.
       * Direct C port of Fortran FBLOCK lines 3947-3961. */
      ssx[0] = 1.0 + 0.0 * I; /* SSX(1,1) = (1,0) */
      int KK = 1;
      int KA = NOP / 2;
      if (NOP == 8)
        KA = 3;
      for (int K = 0; K < KA; K++)
      {
        for (int ii = 0; ii < KK; ii++)
        {
          for (int jj = 0; jj < KK; jj++)
          {
            complex double d = ssx[ii * NOP + jj];
            ssx[ii * NOP + (jj + KK)] = d;         /* SSX(I,   J+KK) =  DETER */
            ssx[(ii + KK) * NOP + (jj + KK)] = -d; /* SSX(I+KK,J+KK) = -DETER */
            ssx[(ii + KK) * NOP + jj] = d;         /* SSX(I+KK,J)    =  DETER */
          }
        }
        KK *= 2;
      }
    }

    bool ok = fw1(file, ssx, (int32_t)(NOP * NOP * 16));
    free(ssx);
    if (!ok)
      return false;
  }

  /* ---------- Record 8: IP[NEQ] + COM[100] — written as zeros ---------- */
  {
    size_t rec8_len = (size_t)neq * 4 + 100 * 8;
    uint8_t *buf = (uint8_t *)calloc(rec8_len, 1);
    if (!buf)
      return false;
    bool ok = fw1(file, buf, (int32_t)rec8_len);
    free(buf);
    if (!ok)
      return false;
  }

  /* ---------- Record 9: CM matrix (IOUT = NEQ*NPEQ complex values) ----------
   * C double complex and Fortran COMPLEX*16 have identical binary layout. */
  {
    int32_t iout = (int32_t)(neq * NP);
    if (!fw1(file, cm, iout * 16))
      return false;
  }

  fflush(file);
  return true;
}

/******************************************************************************
 * read_greens_binary()
 *
 * Reads a Fortran NEC-2 unformatted NGF/WGF file. Compatible with files
 * written by write_greens_binary() and by the original Fortran NEC-2 WG card.
 *
 * Coordinates are stored in wavelength units in the file. On read they are
 * multiplied by WLAM to convert to metres, then wire endpoints are
 * reconstructed from midpoint + direction cosines (matching Fortran NEC-2
 * GF read-back behaviour).
 *
 * @param file  Input file (opened in binary mode).
 * @param ctx   Simulation context.
 * @return      true on success, false on format error.
 */
bool read_greens_binary(FILE *file, context_t *ctx)
{
  if (!file || !ctx)
    return false;

  /* ---------- Record 1: header (88 bytes) ---------- */
  int32_t N, NP, M, MP;
  double WLAM, FMHZ;
  int32_t IPSYM, KSYMP, IPERF, NRADL;
  double EPSR, SIG, SCRWLT, SCRWRT;
  int32_t NLOAD;
  {
    uint8_t rec[88];
    if (!fr1(file, rec, 88))
      goto err;
    const uint8_t *p = rec;
#define GP4(v)         \
  do                   \
  {                    \
    int32_t _v;        \
    memcpy(&_v, p, 4); \
    (v) = _v;          \
    p += 4;            \
  } while (0)
#define GP8(v)         \
  do                   \
  {                    \
    double _v;         \
    memcpy(&_v, p, 8); \
    (v) = _v;          \
    p += 8;            \
  } while (0)
    GP4(N);
    GP4(NP);
    GP4(M);
    GP4(MP);
    GP8(WLAM);
    GP8(FMHZ);
    GP4(IPSYM);
    GP4(KSYMP);
    GP4(IPERF);
    GP4(NRADL);
    GP8(EPSR);
    GP8(SIG);
    GP8(SCRWLT);
    GP8(SCRWRT);
    GP4(NLOAD); /* KCOM not needed */
#undef GP4
#undef GP8
  }

  if (N < 0 || N > 1000000 || NP < 0 || M < 0 || MP < 0)
    goto err;

  /* ---------- Records 2-6: segment geometry ---------- */
  if (N > 0)
  {
    size_t nd = (size_t)N * sizeof(double);
    size_t ni = (size_t)N * sizeof(int);

    mem_realloc(ctx, (void *)&ctx->geometry.x_center, nd);
    mem_realloc(ctx, (void *)&ctx->geometry.y_center, nd);
    mem_realloc(ctx, (void *)&ctx->geometry.z_center, nd);
    mem_realloc(ctx, (void *)&ctx->geometry.half_len, nd);
    mem_realloc(ctx, (void *)&ctx->geometry.radius, nd);
    mem_realloc(ctx, (void *)&ctx->geometry.dir_cos_x, nd);
    mem_realloc(ctx, (void *)&ctx->geometry.dir_cos_y, nd);
    mem_realloc(ctx, (void *)&ctx->geometry.dir_cos_z, nd);
    mem_realloc(ctx, (void *)&ctx->geometry.seg_end1_conn, ni);
    mem_realloc(ctx, (void *)&ctx->geometry.seg_end2_conn, ni);
    mem_realloc(ctx, (void *)&ctx->geometry.tag_nums, ni);
    mem_realloc(ctx, (void *)&ctx->geometry.card_nums, ni);
    mem_realloc(ctx, (void *)&ctx->geometry.end1_x, nd);
    mem_realloc(ctx, (void *)&ctx->geometry.end1_y, nd);
    mem_realloc(ctx, (void *)&ctx->geometry.end1_z, nd);
    mem_realloc(ctx, (void *)&ctx->geometry.end2_x, nd);
    mem_realloc(ctx, (void *)&ctx->geometry.end2_y, nd);
    mem_realloc(ctx, (void *)&ctx->geometry.end2_z, nd);

    /* Rec 2: X[N], Y[N], Z[N] (wavelength units) */
    {
      void *buf;
      int32_t len;
      if (!fr_alloc(file, &buf, &len))
        goto err;
      if (len != (int32_t)(3 * N * 8))
      {
        free(buf);
        goto err;
      }
      memcpy(ctx->geometry.x_center, (double *)buf, N * sizeof(double));
      memcpy(ctx->geometry.y_center, (double *)buf + N, N * sizeof(double));
      memcpy(ctx->geometry.z_center, (double *)buf + 2 * N, N * sizeof(double));
      free(buf);
    }

    /* Rec 3: SI[N], BI[N], ALP[N] (= cab, x-direction cosine) */
    {
      void *buf;
      int32_t len;
      if (!fr_alloc(file, &buf, &len))
        goto err;
      if (len != (int32_t)(3 * N * 8))
      {
        free(buf);
        goto err;
      }
      memcpy(ctx->geometry.half_len, (double *)buf, N * sizeof(double));
      memcpy(ctx->geometry.radius, (double *)buf + N, N * sizeof(double));
      memcpy(ctx->geometry.dir_cos_x, (double *)buf + 2 * N, N * sizeof(double));
      free(buf);
    }

    /* Rec 4: BET[N] (= sab), SALP[N] */
    {
      void *buf;
      int32_t len;
      if (!fr_alloc(file, &buf, &len))
        goto err;
      if (len != (int32_t)(2 * N * 8))
      {
        free(buf);
        goto err;
      }
      memcpy(ctx->geometry.dir_cos_y, (double *)buf, N * sizeof(double));
      memcpy(ctx->geometry.dir_cos_z, (double *)buf + N, N * sizeof(double));
      free(buf);
    }

    /* Rec 5: ICON1[N], ICON2[N] (int32_t in file) */
    {
      int32_t *tmp = (int32_t *)malloc(2 * (size_t)N * sizeof(int32_t));
      if (!tmp)
        goto err;
      if (!fr1(file, tmp, (int32_t)(2 * N * 4)))
      {
        free(tmp);
        goto err;
      }
      for (int i = 0; i < N; i++)
        ctx->geometry.seg_end1_conn[i] = (int)tmp[i];
      for (int i = 0; i < N; i++)
        ctx->geometry.seg_end2_conn[i] = (int)tmp[N + i];
      free(tmp);
    }

    /* Rec 6: ITAG[N] (int32_t in file) */
    {
      int32_t *tmp = (int32_t *)malloc((size_t)N * sizeof(int32_t));
      if (!tmp)
        goto err;
      if (!fr1(file, tmp, (int32_t)(N * 4)))
      {
        free(tmp);
        goto err;
      }
      for (int i = 0; i < N; i++)
        ctx->geometry.tag_nums[i] = (int)tmp[i];
      free(tmp);
    }

    /* Skip optional ZARRAY record if loads were saved */
    if (NLOAD > 0)
    {
      if (!fr_skip(file))
        goto err;
    }

    /* Convert coordinates and lengths: wavelengths -> metres */
    for (int i = 0; i < N; i++)
    {
      ctx->geometry.x_center[i] *= WLAM;
      ctx->geometry.y_center[i] *= WLAM;
      ctx->geometry.z_center[i] *= WLAM;
      ctx->geometry.half_len[i] *= WLAM;
      ctx->geometry.radius[i] *= WLAM;
    }

    /* Reconstruct wire endpoints from midpoint + direction cosines */
    for (int i = 0; i < N; i++)
    {
      double hs = ctx->geometry.half_len[i] * 0.5;
      ctx->geometry.end1_x[i] = ctx->geometry.x_center[i] - hs * ctx->geometry.dir_cos_x[i];
      ctx->geometry.end1_y[i] = ctx->geometry.y_center[i] - hs * ctx->geometry.dir_cos_y[i];
      ctx->geometry.end1_z[i] = ctx->geometry.z_center[i] - hs * ctx->geometry.dir_cos_z[i];
      ctx->geometry.end2_x[i] = ctx->geometry.x_center[i] + hs * ctx->geometry.dir_cos_x[i];
      ctx->geometry.end2_y[i] = ctx->geometry.y_center[i] + hs * ctx->geometry.dir_cos_y[i];
      ctx->geometry.end2_z[i] = ctx->geometry.z_center[i] + hs * ctx->geometry.dir_cos_z[i];
      ctx->geometry.card_nums[i] = -1; /* sourced from NGF file */
    }
  }

  /* ---------- Record 7: matrix blocking parameters ---------- */
  int32_t ICASE, IMAT;
  int NEQ, NPEQ;
  {
    int32_t rec7[8];
    if (!fr1(file, rec7, 32))
      goto err;
    ICASE = rec7[0];
    IMAT = rec7[7];
    /* NBLOKS=rec7[1], NPBLK=rec7[2], NLAST=rec7[3], NBLSYM=rec7[4],
       NPSYM=rec7[5], NLSYM=rec7[6] — not needed beyond matrix size */
  }
  NEQ = (int)(N + 2 * M);
  NPEQ = (int)(NP + 2 * MP);
  if (NPEQ == 0)
    NPEQ = NEQ; /* guard against unset NP in older files */

  /* Skip optional patch coefficient record (IPERF == 2) */
  if (IPERF == 2)
  {
    if (!fr_skip(file))
      goto err;
  }

  /* Skip optional symmetry submatrix (NOP > 1) */
  if (NPEQ > 0 && NEQ / NPEQ > 1)
  {
    if (!fr_skip(file))
      goto err;
  }

  /* ---------- Record 8: IP[NEQ] + COM[100] — read and discard ---------- */
  if (!fr_skip(file))
    goto err;

  /* ---------- Record 9: CM matrix ----------
   * IOUT = NEQ * NPEQ for ICASE <= 2; use IMAT as fallback for out-of-core. */
  int IOUT = (ICASE <= 2) ? NEQ * NPEQ : (int)IMAT;
  if (IOUT <= 0 || IOUT > 100000000)
    goto err;

  complex double *mat = (complex double *)malloc((size_t)IOUT * sizeof(complex double));
  if (!mat)
    goto err;
  if (!fr1(file, mat, (int32_t)(IOUT * 16)))
  {
    free(mat);
    goto err;
  }

  /* Install NGF state */
  if (ctx->ngf_cm != NULL)
    free(ctx->ngf_cm);
  ctx->ngf_cm = mat;
  ctx->ngf_n_segs = (int)N;
  ctx->ngf_neq = NEQ;
  ctx->ngf_fmhz = FMHZ;
  ctx->has_ngf = true;

  /* Update geometry bookkeeping */
  ctx->geometry.num_segs = (int)N;
  ctx->geometry.num_segs_sym = (NP > 0) ? (int)NP : (int)N;
  ctx->geometry.num_patches = (int)M;
  ctx->geometry.num_patches_sym = (int)MP;
  ctx->geometry.wavelength = WLAM;
  ctx->geometry.symmetry_flag = (int)IPSYM;
  ctx->geometry.num_segs_and_patches = (int)(N + M);
  ctx->geometry.num_segs_2xpatches = (int)(N + 2 * M);
  ctx->geometry.num_segs_3xpatches = (int)(N + 3 * M);

  /* Restore ground and frequency parameters from the NGF */
  ctx->gnd.has_ground = (int)KSYMP;
  ctx->gnd.is_perfect = (int)IPERF;
  ctx->gnd.num_radials = (int)NRADL;
  ctx->gnd.screen_wire_len = SCRWLT;
  ctx->gnd.screen_wire_radius = SCRWRT;
  ctx->save.ground_epsr = EPSR;
  ctx->save.ground_sigma = SIG;
  ctx->save.freq_mhz = FMHZ;

  return true;

err:
  add_error(ctx, &ctx->errors,
            "Failed to read NGF/WGF file (not a valid Fortran NEC-2 unformatted "
            "file, or file is truncated or corrupted)",
            FATAL);
  return false;
}

/******************************************************************************
 * Writes the header area and comment cards to the standard NEC output file.
 *
 */
static void write_header(const context_t *ctx, const deck_t *deck, FILE *file)
{
  if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL)
  {
    fprintf(file, "\n\n\n"
                  "                                 *********************************************\n"
                  "\n"
                  "                                    NUMERICAL ELECTROMAGNETICS CODE (onec)\n"
                  "\n"
                  "                                 *********************************************\n");
  }
  else
  {
    fprintf(file, "\n\n\n"
                  "                              "
                  " __________________________________________\n"
                  "                              "
                  "|                                          |\n"
                  "                              "
                  "|  NUMERICAL ELECTROMAGNETICS CODE (onec)  |\n"
                  "                              "
                  "|__________________________________________|\n");
  }

  if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL)
  {
    fprintf(file, "\n\n\n\n"
                  "                                     "
                  "- - - - COMMENTS - - - -\n\n\n");
  }
  else
  {
    fprintf(file, "\n\n\n"
                  "                               "
                  "---------------- COMMENTS ----------------\n");
  }

  // write header comments to output file (CM/CE cards in the comment section only)
  int cstart = (deck->comment_start >= 0) ? deck->comment_start : 0;
  int cend = (deck->comment_end >= 0) ? deck->comment_end : deck->geometry_start - 1;
  for (int i = cstart; i <= cend && i < deck->num_cards; i++)
  {
    card_t *card = &deck->cards[i];
    if ((strcmp(card->card_code, "CM") == 0 || strcmp(card->card_code, "CE") == 0) &&
        card->comment)
    {
      // Strip leading whitespace from comment (NEC format includes space after CM)
      const char *comment_text = card->comment;
      while (*comment_text && isspace((unsigned char)*comment_text))
        comment_text++;
      
      // Skip if comment is empty after stripping whitespace
      if (*comment_text == '\0')
        continue;
      
      if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL)
      {
        // Fortran format: pad comments to 101 characters total (26 spaces + text + pad to 101)
        char comment_line[102];
        int comment_len = strlen(comment_text);
        if (comment_len > 75) comment_len = 75;
        snprintf(comment_line, sizeof(comment_line), "                          %-75s", comment_text);
        fprintf(file, "%s\n", comment_line);
      }
      else
      {
        fprintf(file, "                              %s\n", card->comment);
      }
    }
  }

  if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL)
  {
    /* Original Fortran format: 2 padded blank lines (101 spaces each) */
    fprintf(file, "                                                                                                     \n"
                  "                                                                                                     \n");
  }
}

/******************************************************************************
 * write_structure()
 *
 * Writes the structure section of the nec2 output, which is based on
 * the input geometry cards and the resulting geometry structure.
 *
 * In the original NEC code, writing the geometry description took place
 * during the reading of the geometry cards. With each card it would also
 * build out the "data" array of geometry, so things like the total number
 * of wires, segments and patches were being updated as it went. This
 * allowed it to print out a line of data for each card that had things like
 * the starting and ending segment, the tag number, or the total number of
 * wires so far.
 *
 * OpenNEC processes the entire deck before one might call this code. This
 * means that those bits of data being processed during the read in NEC are
 * not available and have to be saved out to the cards, which is why they
 * have slots for the tag and the segment numbers they span. The geometry
 * structure also includes a new array containing the card number that
 * generated those set of segments. This way the original can can be found
 * without having to rely on tag numbers, which are optional.
 *
 */
static int write_structure(context_t *ctx, const deck_t *deck, FILE *file)
{
  card_t card;
  int geo_card_num;
  int num_wires = 0;
  /* int num_patches = 0; */

  int ix, iy, iz;

  // these are used to match various codes in the cards to text output
  char ifx[2] = {'*', 'X'}, ify[2] = {'*', 'Y'}, ifz[2] = {'*', 'Z'}; // reflection axes
  // char ipt[4] = { 'P', 'R', 'T', 'Q' };

  // print the header
  if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL)
  {
    fprintf(file, "\n\n\n\n"
                  "                                 "
                  "- - - STRUCTURE SPECIFICATION - - -\n"
                  "\n"
                  "                                     "
                  "COORDINATES MUST BE INPUT IN\n"
                  "                                     "
                  "METERS OR BE SCALED TO METERS\n"
                  "                                     "
                  "BEFORE STRUCTURE INPUT IS ENDED\n");
  }
  else
  {
    fprintf(file, "\n\n\n"
                  "                               "
                  "-------- STRUCTURE SPECIFICATION --------\n"
                  "                                     "
                  "COORDINATES MUST BE INPUT IN\n"
                  "                                     "
                  "METERS OR BE SCALED TO METERS\n"
                  "                                     "
                  "BEFORE STRUCTURE INPUT IS ENDED\n");
  }

  if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL)
  {
    fprintf(ctx->output_fp, "\n\n"
                            "  WIRE                                                                               NO. OF    FIRST  LAST     TAG\n"
                            "  NO.        X1         Y1         Z1          X2         Y2         Z2      RADIUS   SEG.     SEG.   SEG.     NO.");
  }
  else
  {
    fprintf(ctx->output_fp, "\n"
                            "  WIRE                                           "
                            "                                      SEG FIRST  LAST  TAG\n"
                            "   NO.        X1         Y1         Z1         X2      "
                            "   Y2         Z2       RADIUS   NO. SEG.   SEG.  NO.");
  }

  for (int i = deck->geometry_start; i <= deck->geometry_end; i++)
  {
    card = deck->cards[i];

    // for onec...
    if (card.ignore)
      continue;

    // if this card is an XT, print it and stop everything
    if (strcmp(card.card_code, "XT") == 0)
    {
      fprintf(ctx->output_fp, "\nOpenNEC: Exiting after an XT command.\n");
      break;
    }

    // convert the card code to a number
    for (geo_card_num = 0; geo_card_num < NUM_GEOMETRY_CODES; geo_card_num++)
    {
      if (strncmp(card.card_code, geometry_codes[geo_card_num], 2) == 0)
        break;
    }

    // switch on the number
    switch (geo_card_num)
    {

    case 0: // GW card, a wire
      num_wires++;
      if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL)
      {
        char sf1[12], sf2[12], sf3[12], sf4[12], sf5[12], sf6[12], sf7[12];
        format_coord(sf1, sizeof(sf1), card.f[1], "%11.5f");
        format_coord(sf2, sizeof(sf2), card.f[2], "%11.5f");
        format_coord(sf3, sizeof(sf3), card.f[3], "%11.5f");
        format_coord(sf4, sizeof(sf4), card.f[4], "%11.5f");
        format_coord(sf5, sizeof(sf5), card.f[5], "%11.5f");
        format_coord(sf6, sizeof(sf6), card.f[6], "%11.5f");
        format_coord(sf7, sizeof(sf7), card.f[7], "%11.5f");
        fprintf(ctx->output_fp, "\n"
                                " %5d%s%s%s %s%s%s%s  %5d    %5d %5d   %5d",
                num_wires, sf1, sf2, sf3, sf4, sf5, sf6, sf7,
                card.num_segments, card.start_segment, card.end_segment, card.tag);
      }
      else
      {
        char sf1[12], sf2[12], sf3[12], sf4[12], sf5[12], sf6[12], sf7[12];
        format_coord(sf1, sizeof(sf1), card.f[1], "%10.5f");
        format_coord(sf2, sizeof(sf2), card.f[2], "%10.5f");
        format_coord(sf3, sizeof(sf3), card.f[3], "%10.5f");
        format_coord(sf4, sizeof(sf4), card.f[4], "%10.5f");
        format_coord(sf5, sizeof(sf5), card.f[5], "%10.5f");
        format_coord(sf6, sizeof(sf6), card.f[6], "%10.5f");
        format_coord(sf7, sizeof(sf7), card.f[7], "%10.5f");
        fprintf(ctx->output_fp, "\n"
                                " %5d %s %s %s %s"
                                " %s %s %s %5d %5d %5d %4d",
                num_wires, sf1, sf2, sf3, sf4, sf5, sf6, sf7,
                card.num_segments, card.start_segment, card.end_segment, card.tag);
      }
      break;

    case 1: // GX card, reflection or rotation
      // decode the flags stored in the I2 value on the card
      iy = card.i[2] / 10;
      iz = card.i[2] - iy * 10;
      ix = iy / 10;
      iy = iy - ix * 10;

      if (ix != 0)
        ix = 1;
      if (iy != 0)
        iy = 1;
      if (iz != 0)
        iz = 1;

      fprintf(ctx->output_fp,
              "\n      STRUCTURE REFLECTED ALONG THE AXES %c %c %c"
              " - TAGS INCREMENTED BY %d",
              ifx[ix], ify[iy], ifz[iz], card.i[1]);
      break;

    case 3: // GS card, scale structure dimensions
      fprintf(ctx->output_fp,
              "\n     STRUCTURE SCALED BY FACTOR: %10.5f", card.f[1]);
      break;

    case 4: // GE card, nothing to do
      break;

    case 5: // GM card, move/copy existing structure
      fprintf(ctx->output_fp,
              "\n     THE STRUCTURE HAS BEEN MOVED, MOVE DATA CARD IS:\n"
              "   %3d %5d %10.5f %10.5f %10.5f %10.5f %10.5f %10.5f %10.5f",
              card.i[1], card.i[2], card.f[1], card.f[2], card.f[3], card.f[4], card.f[5], card.f[6], card.f[7]);
      break;

    case 6: // SP card, generate single surface patch
      /* num_patches++; */
      fprintf(ctx->output_fp, "\n"
                              " %5d%c %10.5f %10.5f %10.5f %10.5f %10.5f %10.5f",
              card.i[1], 'R', card.f[1], card.f[2], card.f[3], card.f[4], card.f[5], card.f[6]);
      break;

    case 7: // SM card, multiple-patch surface
            //        i1= data.m+1;
            //        fprintf( ctx->output_fp, "\n"
            //                " %5d%c %10.5f %11.5f %11.5f %11.5f %11.5f %11.5f"
            //                "     SURFACE - %d BY %d PATCHES",
            //                i1, ipt[1], xw1, yw1, zw1, xw2, yw2, zw2, itg, ns );
            //

      break;

    case 8: // GA card, arc
      num_wires++;
      fprintf(ctx->output_fp, "\n"
                              " %5d ARC RADIUS: %9.5f  FROM: %8.3f TO: %8.3f DEGREES"
                              "       %11.5f %5d %5d %5d %4d",
              num_wires, card.f[1], card.f[2], card.f[3], card.f[4],
              card.tag, card.start_segment, card.end_segment, card.i[1]);

      // FIXME: this looks wrong, last input
      break;

    case 9: // SC card, does nothing
      break;

    case 10: // GH card, generate helix
      num_wires++;
      fprintf(ctx->output_fp, "\n"
                              " %5d HELIX STRUCTURE - SPACING OF TURNS: %8.3f AXIAL"
                              " LENGTH: %8.3f  %8.3f %5d %5d %5d %4d\n      "
                              " RADIUS X1:%8.3f Y1:%8.3f X2:%8.3f Y2:%8.3f ",
              num_wires, card.f[1], card.f[2], card.f[7], card.tag, card.start_segment, card.end_segment,
              card.i[1], card.f[3], card.f[4], card.f[5], card.f[6]);
      break;

    } /* switch on the card type */
  } /* for loop over cards */

  // and now a final report on the cards
  if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL)
  {
    fprintf(ctx->output_fp, "\n\n"
                            "   TOTAL SEGMENTS USED=   %d     NO. SEG. IN A SYMMETRIC CELL=   %d     SYMMETRY FLAG=  %d",
            ctx->geometry.num_segs, ctx->geometry.num_segs_sym, ctx->geometry.symmetry_flag);
  }
  else
  {
    fprintf(ctx->output_fp, "\n\n"
                            "     TOTAL SEGMENTS USED: %d   SEGMENTS IN A"
                            " SYMMETRIC CELL: %d   SYMMETRY FLAG: %d",
            ctx->geometry.num_segs, ctx->geometry.num_segs_sym, ctx->geometry.symmetry_flag);
  }

  if (ctx->geometry.num_patches > 0)
    fprintf(ctx->output_fp, "\n"
                            "       TOTAL PATCHES USED: %d   PATCHES"
                            " IN A SYMMETRIC CELL: %d",
            ctx->geometry.num_patches, ctx->geometry.num_patches_sym);

  int iseg = (ctx->geometry.num_segs + ctx->geometry.num_patches) / (ctx->geometry.num_segs_sym + ctx->geometry.num_patches_sym);
  if (iseg != 1)
  {
    /*** may be error condition?? ***/
    if (ctx->geometry.symmetry_flag == 0)
    {
      add_error(ctx, &ctx->errors, "ERROR: IPSYM=0 IN CONECT()", FATAL);
      return -1;
    }

    if (ctx->geometry.symmetry_flag < 0)
      fprintf(ctx->output_fp,
              "\n  STRUCTURE HAS %d FOLD ROTATIONAL SYMMETRY\n", iseg);
    else
    {
      int ic = iseg / 2;
      if (iseg == 8)
        ic = 3;
      fprintf(ctx->output_fp,
              "\n  STRUCTURE HAS %d PLANES OF SYMMETRY\n", ic);
    } /* if(ctx->geometry.symmetry_flag < 0 ) */
  } /* if( iseg != 1) */

  /* Output MULTIPLE WIRE JUNCTIONS section (always present when N > 0) */
  if (ctx->geometry.num_segs > 0)
  {
    if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL)
    {
      fprintf(ctx->output_fp, "\n\n\n"
                              "         - MULTIPLE WIRE JUNCTIONS -\n"
                              " JUNCTION    SEGMENTS  (- FOR END 1, + FOR END 2)\n"
                              "  NONE\n");
    }
    else
    {
      fprintf(ctx->output_fp, "\n\n\n"
                              "        -------- MULTIPLE WIRE JUNCTIONS --------\n"
                              "  JUNCTION           SEGMENTS  (- FOR END 1, + FOR END 2)\n"
                              "   NONE");
    }
  }

  // Output any informational messages collected during geometry processing
  for (int i = 0; i < ctx->outputs.num_messages; i++)
  {
    fprintf(ctx->output_fp, "%s\n", ctx->outputs.messages[i]);
  }

  return 0;
} /* write_structure() */

/******************************************************************************
 * write_segments()
 *
 * Writes the segment data section of the nec2 output.
 *
 */
static int write_segments(context_t *ctx, const deck_t *deck, FILE *file)
{
  // exit now if there's no segments
  if (ctx->geometry.num_segs == 0)
    return 0;

  if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL)
  {
    fprintf(ctx->output_fp, "\n\n\n\n"
                            "                                 "
                            "- - - - SEGMENTATION DATA - - - -\n"
                            "\n"
                            "                                        "
                            "COORDINATES IN METERS\n"
                            "\n"
                            "                         "
                            "I+ AND I- INDICATE THE SEGMENTS BEFORE AND AFTER I\n"
                            "\n\n"
                            "  SEG.   COORDINATES OF SEG. CENTER     SEG.     ORIENTATION ANGLES    WIRE    CONNECTION DATA   TAG\n"
                            "  NO.       X         Y         Z       LENGTH     ALPHA     BETA      RADIUS    I-   I    I+    NO.");
  }
  else
  {
    fprintf(ctx->output_fp, "\n\n\n"
                            "                              "
                            " ---------- SEGMENTATION DATA ----------\n"
                            "                                       "
                            " COORDINATES IN METERS\n"
                            "                           "
                            " I+ AND I- INDICATE THE SEGMENTS BEFORE AND AFTER I\n");
    fprintf(ctx->output_fp, "\n"
                            "   SEG    COORDINATES OF SEGM CENTER     SEGM    ORIENTATION"
                            " ANGLES    WIRE    CONNECTION DATA   TAG\n"
                            "   NO.       X         Y         Z      LENGTH     ALPHA     "
                            " BETA    RADIUS    I-     I    I+   NO.");
  }

  double xw1, yw1, zw1;
  double xw2, yw2;

  for (int i = 0; i < ctx->geometry.num_segs; i++)
  {
    xw1 = ctx->geometry.end2_x[i] - ctx->geometry.end1_x[i];
    yw1 = ctx->geometry.end2_y[i] - ctx->geometry.end1_y[i];
    zw1 = ctx->geometry.end2_z[i] - ctx->geometry.end1_z[i];
    ctx->geometry.x_center[i] = (ctx->geometry.end1_x[i] + ctx->geometry.end2_x[i]) / 2.0;
    ctx->geometry.y_center[i] = (ctx->geometry.end1_y[i] + ctx->geometry.end2_y[i]) / 2.0;
    ctx->geometry.z_center[i] = (ctx->geometry.end1_z[i] + ctx->geometry.end2_z[i]) / 2.0;
    xw2 = xw1 * xw1 + yw1 * yw1 + zw1 * zw1;
    yw2 = sqrt(xw2);
    yw2 = (xw2 / yw2 + yw2) * .5;
    ctx->geometry.half_len[i] = yw2;
    ctx->geometry.dir_cos_x[i] = xw1 / yw2;
    ctx->geometry.dir_cos_y[i] = yw1 / yw2;
    xw2 = zw1 / yw2;

    if (xw2 > 1.0)
      xw2 = 1.0;
    if (xw2 < -1.0)
      xw2 = -1.0;

    ctx->geometry.dir_cos_z[i] = xw2;
    xw2 = asin(xw2) * TD;
    yw2 = atan2(yw1, xw1) * TD;

    if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL)
    {
      char sx[12], sy[12], sz[12];
      format_coord(sx, sizeof(sx), ctx->geometry.x_center[i], "%10.5f");
      format_coord(sy, sizeof(sy), ctx->geometry.y_center[i], "%10.5f");
      format_coord(sz, sizeof(sz), ctx->geometry.z_center[i], "%10.5f");
      fprintf(ctx->output_fp, "\n %5d%s%s%s%10.5f %10.5f%10.5f%10.5f %5d%5d%5d  %5d",
              i + 1, sx, sy, sz, ctx->geometry.half_len[i],
              xw2, yw2, ctx->geometry.radius[i],
              ctx->geometry.seg_end1_conn[i], i + 1, ctx->geometry.seg_end2_conn[i], ctx->geometry.tag_nums[i]);
    }
    else
    {
      char sx[12], sy[12], sz[12];
      format_coord(sx, sizeof(sx), ctx->geometry.x_center[i], "%9.4f");
      format_coord(sy, sizeof(sy), ctx->geometry.y_center[i], "%9.4f");
      format_coord(sz, sizeof(sz), ctx->geometry.z_center[i], "%9.4f");
      fprintf(ctx->output_fp, "\n"
                              " %5d %s %s %s %9.4f"
                              " %9.4f %9.4f %9.4f %5d %5d %5d %5d",
              i + 1, sx, sy, sz, ctx->geometry.half_len[i], 
              xw2, yw2,
              ctx->geometry.radius[i], ctx->geometry.seg_end1_conn[i], i + 1, ctx->geometry.seg_end2_conn[i], ctx->geometry.tag_nums[i]);
    }

    if (ctx->plot.plot_type == 1)
      fprintf(ctx->plot_fp, "%12.4E %12.4E %12.4E "
                            "%12.4E %12.4E %12.4E %12.4E %5d %5d %5d\n",
              ctx->geometry.x_center[i], ctx->geometry.y_center[i], ctx->geometry.z_center[i], ctx->geometry.half_len[i], xw2, yw2,
              ctx->geometry.radius[i], ctx->geometry.seg_end1_conn[i], i + 1, ctx->geometry.seg_end2_conn[i]);

    if ((ctx->geometry.half_len[i] <= 1.e-20) || (ctx->geometry.radius[i] <= 0.0))
    {
      add_error(ctx, &ctx->errors, "SEGMENT DATA ERROR", FATAL);
      return -1;
    }

  } /* for( i = 0; i < data.n; i++ ) */

  fprintf(ctx->output_fp, "\n");
  return 0;
} /* write_segments */

/******************************************************************************
 * write_patches()
 *
 * writes the patch data section of the nec2 output.
 *
 */
static void write_patches(const context_t *ctx, const deck_t *deck, FILE *file)
{
  // exit now if there's no patches
  if (ctx->geometry.num_patches == 0)
    return;

  fprintf(ctx->output_fp, "\n\n\n"
                          "                                   "
                          " --------- SURFACE PATCH DATA ---------\n"
                          "                                            "
                          " COORDINATES IN METERS\n\n"
                          " PATCH      COORD. OF PATCH CENTER           UNIT NORMAL VECTOR      "
                          " PATCH           COMPONENTS OF UNIT TANGENT VECTORS\n"
                          "  No:       X          Y          Z          X        Y        Z      "
                          " AREA         X1       Y1       Z1        X2       Y2      Z2");

  double xw1, yw1, zw1;
  for (int i = 0; i < ctx->geometry.num_patches; i++)
  {
    xw1 = (ctx->geometry.patch_t1y[i] * ctx->geometry.patch_t2z[i] - ctx->geometry.patch_t1z[i] * ctx->geometry.patch_t2y[i]) * ctx->geometry.patch_normal_z[i];
    yw1 = (ctx->geometry.patch_t1z[i] * ctx->geometry.patch_t2x[i] - ctx->geometry.patch_t1x[i] * ctx->geometry.patch_t2z[i]) * ctx->geometry.patch_normal_z[i];
    zw1 = (ctx->geometry.patch_t1x[i] * ctx->geometry.patch_t2y[i] - ctx->geometry.patch_t1y[i] * ctx->geometry.patch_t2x[i]) * ctx->geometry.patch_normal_z[i];

    fprintf(ctx->output_fp, "\n"
                            " %4d %10.5f %10.5f %10.5f  %8.4f %8.4f %8.4f"
                            " %10.5f  %8.4f %8.4f %8.4f  %8.4f %8.4f %8.4f",
            i + 1, ctx->geometry.patch_x_center[i], ctx->geometry.patch_y_center[i], ctx->geometry.patch_z_center[i], xw1, yw1, zw1, ctx->geometry.patch_area[i],
            ctx->geometry.patch_t1x[i], ctx->geometry.patch_t1y[i], ctx->geometry.patch_t1z[i], ctx->geometry.patch_t2x[i], ctx->geometry.patch_t2y[i], ctx->geometry.patch_t2z[i]);
  } /* for( i = 0; i < data.m; i++ ) */
}

/**
 * Write input cards echo to output file
 * Echoes FR, TL, LD, EX, RP, and other control cards from the current batch.
 * The batch is defined by batch_start and batch_end (inclusive).
 * If batch_end points to EN or XT, that card is included as the final card.
 *
 * @param file Output file pointer
 * @param deck The deck containing all cards
 * @param batch_start First card index of this batch (inclusive)
 * @param batch_end Last card index of this batch (inclusive)
 * @param card_number_offset Starting card number for this batch
 */
/****************************************************************************
 * write_input_cards_excluding_end()
 *
 * Like write_input_cards but skips EN and NX cards (which are output separately).
 */
static void write_input_cards_excluding_end(FILE *file, const context_t *ctx, const deck_t *deck, int batch_start, int batch_end, int card_number_offset)
{
  if (file == NULL || ctx == NULL || deck == NULL)
  {
    return;
  }

  fprintf(file, "\n\n\n\n\n\n");

  /* Iterate through cards in this batch only, skipping EN and NX cards. */
  int card_number = card_number_offset;
  for (int i = batch_start; i <= batch_end && i < deck->num_cards; i++)
  {
    card_t *card = &deck->cards[i];

    /* Skip EN and NX cards - they're output separately at the end */
    if (strncmp(card->card_code, "EN", 2) == 0 || strncmp(card->card_code, "NX", 2) == 0)
    {
      continue;
    }

    /* Check for XT card - echo it as final card of batch */
    if (strncmp(card->card_code, "XT", 2) == 0)
    {
      card_number++;
      fprintf(file, "  DATA CARD No: %3d %s", card_number, card->card_code);
      fprintf(file, " %3d", card->i[1]);
      for (int j = 2; j <= 4; j++)
      {
        fprintf(file, " %5d", card->i[j]);
      }
      for (int j = 1; j <= 6; j++)
      {
        fprintf(file, " %12.5E", card->f[j]);
      }
      fprintf(file, "\n");
      fprintf(file, "\nOpenNEC: Exiting after an XT command.\n");
      continue;
    }

    /* Only echo control cards (skip geometry and comment cards) */
    if (strncmp(card->card_code, "FR", 2) == 0 ||
        strncmp(card->card_code, "EX", 2) == 0 ||
        strncmp(card->card_code, "LD", 2) == 0 ||
        strncmp(card->card_code, "TL", 2) == 0 ||
        strncmp(card->card_code, "NT", 2) == 0 ||
        strncmp(card->card_code, "RP", 2) == 0 ||
        strncmp(card->card_code, "GN", 2) == 0 ||
        strncmp(card->card_code, "EK", 2) == 0 ||
        strncmp(card->card_code, "KH", 2) == 0 ||
        strncmp(card->card_code, "NE", 2) == 0 ||
        strncmp(card->card_code, "NH", 2) == 0 ||
        strncmp(card->card_code, "PT", 2) == 0 ||
        strncmp(card->card_code, "PQ", 2) == 0 ||
        strncmp(card->card_code, "CP", 2) == 0 ||
        strncmp(card->card_code, "GD", 2) == 0 ||
        strncmp(card->card_code, "WG", 2) == 0 ||
        strncmp(card->card_code, "XQ", 2) == 0)
    {

      card_number++;

      /* Output in exact NEC format: card number, card code, 4 ints, 7 floats */
      if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL)
      {
        fprintf(file, " ***** DATA CARD NO. %2d   %s", card_number, card->card_code);
      }
      else
      {
        fprintf(file, "  DATA CARD No: %3d %s", card_number, card->card_code);
      }

      /* Output 4 integer fields */
      fprintf(file, " %3d", card->i[1]);
      for (int j = 2; j <= 4; j++)
      {
        fprintf(file, " %5d", card->i[j]);
      }

      /* Output 7 float fields in scientific notation */
      for (int j = 1; j <= 6; j++)
      {
        fprintf(file, " %12.5E", card->f[j]);
      }

      fprintf(file, "\n");
    }

    else if (strncmp(card->card_code, "CM", 2) == 0 || strncmp(card->card_code, "CE", 2) == 0)
    {
      if (card->comment)
      {
        fprintf(file, "                              %s\n", card->comment);
      }
    }
  }

  fprintf(file, "\n\n");
}

/******************************************************************************
 * write_end_cards()
 *
 * Outputs EN and NX cards as separate batches at the end of the output,
 * each with their own DATA CARD No: line.
 */
void write_end_cards(FILE *file, const deck_t *deck)
{
  if (file == NULL || deck == NULL)
  {
    return;
  }

  int en_card_found = 0;
  card_t last_rp_card = {0};
  int found_rp = 0;
  int card_number = 0;
  
  /* Count control cards to number the EN card correctly, and find the last RP card */
  for (int i = 0; i < deck->num_cards; i++)
  {
    card_t *card = &deck->cards[i];
    
    /* Check if EN card exists */
    if (strncmp(card->card_code, "EN", 2) == 0)
    {
      en_card_found = 1;
    }
    
    /* Track last RP card for implicit EN parameters */
    if (strncmp(card->card_code, "RP", 2) == 0)
    {
      found_rp = 1;
      last_rp_card = *card;
    }
    
    /* Count all control cards (including EN/NX) */
    if (strncmp(card->card_code, "FR", 2) == 0 ||
        strncmp(card->card_code, "EX", 2) == 0 ||
        strncmp(card->card_code, "LD", 2) == 0 ||
        strncmp(card->card_code, "TL", 2) == 0 ||
        strncmp(card->card_code, "NT", 2) == 0 ||
        strncmp(card->card_code, "RP", 2) == 0 ||
        strncmp(card->card_code, "EN", 2) == 0 ||
        strncmp(card->card_code, "NX", 2) == 0 ||
        strncmp(card->card_code, "GN", 2) == 0 ||
        strncmp(card->card_code, "EK", 2) == 0 ||
        strncmp(card->card_code, "KH", 2) == 0 ||
        strncmp(card->card_code, "NE", 2) == 0 ||
        strncmp(card->card_code, "NH", 2) == 0 ||
        strncmp(card->card_code, "PT", 2) == 0 ||
        strncmp(card->card_code, "PQ", 2) == 0 ||
        strncmp(card->card_code, "CP", 2) == 0 ||
        strncmp(card->card_code, "GD", 2) == 0 ||
        strncmp(card->card_code, "WG", 2) == 0 ||
        strncmp(card->card_code, "XQ", 2) == 0 ||
        strncmp(card->card_code, "XT", 2) == 0)
    {
      card_number++;
    }
  }
  
  /* If no explicit EN card was found, output an implicit EN card */
  if (!en_card_found)
  {
    card_number++;  /* Increment to get the correct EN card number */
    fprintf(file, "\n");
    
    if (found_rp)
    {
      /* Use parameters from the last RP card */
      fprintf(file, " ***** DATA CARD NO. %2d   EN   %d   %d    %d  %d", card_number, 
              last_rp_card.i[1], last_rp_card.i[2], last_rp_card.i[3], last_rp_card.i[4]);
      
      /* Output float fields from the last RP card */
      for (int j = 1; j <= 6; j++)
      {
        fprintf(file, " %12.5E", last_rp_card.f[j]);
      }
      
      fprintf(file, "\n");
    }
    else
    {
      /* Default EN card with all zeros */
      fprintf(file, " ***** DATA CARD NO. %2d   EN   0     0     0     0  0.00000E+00  0.00000E+00  0.00000E+00  0.00000E+00  0.00000E+00  0.00000E+00\n", card_number);
    }
  }
}

/******************************************************************************
 * write_frequency_data
 *
 * Writes the frequency in MHz and wavelength in meters, plus integration
 * method information. This matches the NEC2 output format.
 */
static void write_frequency_data(FILE *file, const context_t *ctx)
{
  const output_format_spec_t *fmt = get_format(ctx);
  
  if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL)
  {
    /* Original Fortran format with spaces around FREQUENCY */
    fprintf(file, "\n\n"
                  "                                 "
                  "- - - - - - FREQUENCY - - - - - -\n"
                  "\n"
                  "                                    "
                  "%s%11.4E %s\n"
                  "                                    "
                  "%s%11.4E %s",
            fmt->frequency_label, ctx->save.freq_mhz, fmt->freq_units,
            fmt->wavelength_label, ctx->geometry.wavelength, fmt->length_units);

    fprintf(file, "\n\n\n\n\n"
                  "                    "
                  "APPROXIMATE INTEGRATION EMPLOYED FOR SEGMENTS MORE THAN  %6.3f WAVELENGTHS APART",
            ctx->dataj.k_half_len);
  }
  else
  {
    /* NEC2C format */
    fprintf(file, "\n\n"
                  "                               "
                  "%s%s%s\n"
                  "                                "
                  "%s%11.4E %s\n"
                  "                                "
                  "%s%11.4E %s",
            fmt->header_separator, "FREQUENCY", fmt->header_separator,
            fmt->frequency_label, ctx->save.freq_mhz, fmt->freq_units,
            fmt->wavelength_label, ctx->geometry.wavelength, fmt->length_units);

    fprintf(file, "\n\n"
                  "                        "
                  "APPROXIMATE INTEGRATION EMPLOYED FOR SEGMENTS \n"
                  "                        "
                  "THAT ARE MORE THAN %.3f WAVELENGTHS APART",
            ctx->dataj.k_half_len);
  }

  if (ctx->dataj.use_extended_kernel == 1)
  {
    fprintf(file, "\n\n"
                  "                    "
                  "THE EXTENDED THIN WIRE KERNEL WILL BE USED\n");
  }
}

/******************************************************************************
 * write_loading_data
 *
 * Writes the structure impedance loading section header.
 * The actual loading details are printed by apply_impedance_loading() in calculations.c
 * as it processes the loading cards.
 */
static void write_loading_data(FILE *file, const context_t *ctx)
{
  if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL)
  {
    fprintf(file, "\n\n\n"
                  "                               "
                  "- - - STRUCTURE IMPEDANCE LOADING - - -\n\n");
  }
  else
  {
    fprintf(file, "\n\n\n"
                  "                          "
                  "------ STRUCTURE IMPEDANCE LOADING ------\n\n");
  }

  if (ctx->zload.num_loads == 0)
  {
    fprintf(file, "\n"
                  "                                 "
                  "THIS STRUCTURE IS NOT LOADED");
    return;
  }

  // Print the loading data header (from apply_impedance_loading() function)
  if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL)
  {
    fprintf(file, "\n"
                  "       LOCATION          RESISTANCE   INDUCTANCE  CAPACITANCE   "
                  "    IMPEDANCE (OHMS)     CONDUCTIVITY    TYPE\n"
                  "    ITAG FROM THRU          OHMS        HENRYS       FARADS      "
                  "  REAL      IMAGINARY    MHOS/METER\n");
  }
  else
  {
    fprintf(file, "\n"
                  "  LOCATION        RESISTANCE  INDUCTANCE  CAPACITANCE   "
                  "  IMPEDANCE (OHMS)   CONDUCTIVITY  CIRCUIT\n"
                  "  ITAG FROM THRU     OHMS       HENRYS      FARADS     "
                  "  REAL     IMAGINARY   MHOS/METER      TYPE\n");
  }

  // Print the stored loading entries
  for (int i = 0; i < ctx->loading_outputs.count; i++)
  {
    loading_output_t *entry = &ctx->loading_outputs.entries[i];
    const output_format_spec_t *fmt = get_format(ctx);
    
    if (strcmp(entry->type, "WIRE") == 0)
    {
      // Special format for WIRE entries to match prnt output exactly
      // Use "ALL" or "0" for tag 0 based on format
      if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL)
      {
        // Fortran format: original NEC-2D
        // Format:      ALL<84 spaces>3.6900E+07     WIRE  (with 2 trailing spaces)
        if (entry->tag == 0 && strcmp(fmt->loading_all_tag, "ALL") == 0)
        {
          fprintf(file, "\n     ALL%80s%11.4E     WIRE  ",
                  "", entry->conductivity);
        }
        else
        {
          fprintf(file, "\n%5d%77s%11.4E     WIRE  ",
                  entry->tag, "", entry->conductivity);
        }
      }
      else
      {
        // nec2c format
        if (entry->tag == 0 && strcmp(fmt->loading_all_tag, "ALL") == 0)
        {
          fprintf(file, "\n  ALL%69s%11.4E     WIRE  ",
                  "", entry->conductivity);
        }
        else
        {
          fprintf(file, "\n%5d%72s%11.4E     WIRE  ",
                  entry->tag, "", entry->conductivity);
        }
      }
    }
    else if (strcmp(entry->type, "FIXED IMPEDANCE") == 0)
    {
      /* LD type 4 — fixed complex impedance: display R in REAL column and X in
       * IMAGINARY column, leaving OHMS/HENRYS/FARADS blank.  nec2c output:
       *   ITAG FROM THRU   (blank×3)   REAL   IMAGINARY   (blank MHOS)   TYPE
       * Each numeric column is 11 chars wide (%11.4E or 11 blank spaces).   */
      char col_real[12], col_imag[12];
      if (fabs(entry->f1) > 1.0e-20)
          snprintf(col_real, sizeof(col_real), "%11.4E", entry->f1);
      else
          snprintf(col_real, sizeof(col_real), "%11s", "");
      if (fabs(entry->f2) > 1.0e-20)
          snprintf(col_imag, sizeof(col_imag), "%11.4E", entry->f2);
      else
          snprintf(col_imag, sizeof(col_imag), "%11s", "");
      fprintf(file, "\n%5d%5d%5d%11s%11s%11s%s%s%15s FIXED IMPEDANCE ",
              entry->tag, entry->tagf, entry->tagt,
              "", "", "",        /* OHMS, HENRYS, FARADS — blank */
              col_real, col_imag,
              "");               /* MHOS/METER — blank */
    }
    else
    {
      // General format for other loading types (SERIES, PARALLEL, etc.)
      fprintf(file, "\n%6d%6d%6d%44.4E%-6s",
              entry->tag, entry->tagf, entry->tagt,
              entry->conductivity, entry->type);
    }
  }
}

/******************************************************************************
 * write_environment_data
 *
 * Writes the antenna environment section (free space, perfect ground, or
 * finite ground with parameters).
 */
static void write_environment_data(FILE *file, const context_t *ctx)
{
  if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL)
  {
    fprintf(file, "\n\n\n"
                  "                                  "
                  "- - - ANTENNA ENVIRONMENT - - -");
  }
  else
  {
    fprintf(file, "\n\n\n"
                  "                            "
                  "-------- ANTENNA ENVIRONMENT --------");
  }

  if (ctx->gnd.has_ground == 1)
  {
    fprintf(file, "\n\n"
                  "                                            "
                  "FREE SPACE");
  }
  else
  {
    if (ctx->gnd.is_perfect == 1)
    {
      fprintf(file, "\n\n"
                    "                                            "
                    "PERFECT GROUND");
    }
    else
    {
      // Radial wire ground screen
      if (ctx->gnd.num_radials != 0)
      {
        fprintf(file, "\n\n"
                      "                                            "
                      "RADIAL WIRE GROUND SCREEN\n"
                      "                                            "
                      "%d WIRES\n"
                      "                            "
                      "WIRE LENGTH: %8.2f METERS\n"
                      "                            "
                      "WIRE RADIUS: %10.3E METERS",
                ctx->gnd.num_radials, ctx->save.screen_wire_len, ctx->save.screen_wire_radius);

        fprintf(file, "\n"
                      "                            "
                      "MEDIUM UNDER SCREEN -");
      }

      // Ground type
      if (ctx->gnd.is_perfect != 2)
      {
        fprintf(file, "\n"
                      "                            "
                      "FINITE GROUND - REFLECTION COEFFICIENT APPROXIMATION");
      }
      else
      {
        fprintf(file, "\n"
                      "                            "
                      "FINITE GROUND - SOMMERFELD SOLUTION");
      }

      // Ground parameters
      complex double epsc = cmplx(ctx->save.ground_epsr, -ctx->save.ground_sigma * ctx->geometry.wavelength * 59.96);
      fprintf(file, "\n"
                    "                            "
                    "RELATIVE DIELECTRIC CONST: %.3f\n"
                    "                            "
                    "CONDUCTIVITY: %10.3E MHOS/METER\n"
                    "                            "
                    "COMPLEX DIELECTRIC CONSTANT: %11.4E%+11.4Ej",
              ctx->save.ground_epsr, ctx->save.ground_sigma, creal(epsc), cimag(epsc));
    }
  }
}

/******************************************************************************
 * write_matrix_timing
 *
 * Writes the matrix fill and factor timing information.
 */
static void write_matrix_timing(FILE *file, const context_t *ctx)
{
  const output_format_spec_t *fmt = get_format(ctx);
  
  fprintf(file, "\n\n\n"
                "                                "
                "%s\n"
                "\n"
                "                        ",
          fmt->matrix_sep);
  
  if (fmt->use_seconds_for_timing)
  {
    fprintf(file, fmt->matrix_fill_format, ctx->mat_fill_time);
    fprintf(file, "  ");
    fprintf(file, fmt->matrix_factor_format, ctx->mat_factor_time);
  }
  else
  {
    fprintf(file, fmt->matrix_fill_format, (int)(ctx->mat_fill_time * 1000.0));
    fprintf(file, "  ");
    fprintf(file, fmt->matrix_factor_format, (int)(ctx->mat_factor_time * 1000.0));
  }
}

/******************************************************************************
 * write_network_data
 *
 * Writes the network data section showing transmission lines and network
 * connections between segments.
 */
static void write_network_data(FILE *file, const context_t *ctx)
{
  if (ctx->netcx.num_networks == 0)
  {
    return; // No network data to write
  }

  fprintf(file, "\n\n\n"
                "                                            "
                "---------- NETWORK DATA ----------");

  int itmp1 = ctx->netcx.net_types[0];
  int itmp3 = 0;
  const char *pnet[3] = {"  ", "NON-CROSSED", "CROSSED"};

  for (int i = 0; i < 2; i++)
  {
    if (itmp1 == 3)
      itmp1 = 2;

    if (itmp1 == 2)
    {
      fprintf(file, "\n"
                    "  -- FROM -  --- TO --      TRANSMISSION LINE       "
                    " --------- SHUNT ADMITTANCES (MHOS) ---------   LINE\n"
                    "  TAG   SEG  TAG   SEG    IMPEDANCE      LENGTH    "
                    " ----- END ONE -----      ----- END TWO -----   TYPE\n"
                    "  No:   No:  No:   No:         OHMS      METERS     "
                    " REAL      IMAGINARY      REAL      IMAGINARY");
    }
    else if (itmp1 == 1)
    {
      fprintf(file, "\n"
                    "  -- FROM -  --- TO --            --------"
                    " ADMITTANCE MATRIX ELEMENTS (MHOS) ---------\n"
                    "  TAG   SEG  TAG   SEG   ----- (ONE,ONE) ------  "
                    " ----- (ONE,TWO) -----   ----- (TWO,TWO) -------\n"
                    "  No:   No:  No:   No:      REAL      IMAGINARY     "
                    " REAL     IMAGINARY       REAL      IMAGINARY");
    }

    for (int j = 0; j < ctx->netcx.num_networks; j++)
    {
      int itmp2 = ctx->netcx.net_types[j];

      if ((itmp2 / itmp1) != 1)
      {
        itmp3 = itmp2;
      }
      else
      {
        int itmp4 = ctx->netcx.net_seg1[j];
        int itmp5 = ctx->netcx.net_seg2[j];
        int idx4 = itmp4 - 1;
        int idx5 = itmp5 - 1;

        if ((itmp2 >= 2) && (ctx->netcx.y11_imag[j] <= 0.0))
        {
          double xx = ctx->geometry.x_center[idx5] - ctx->geometry.x_center[idx4];
          double yy = ctx->geometry.y_center[idx5] - ctx->geometry.y_center[idx4];
          double zz = ctx->geometry.z_center[idx5] - ctx->geometry.z_center[idx4];
          ctx->netcx.y11_imag[j] = ctx->geometry.wavelength * sqrt(xx * xx + yy * yy + zz * zz);
        }

        fprintf(file, "\n"
                      " %4d %5d %4d %5d  %11.4E %11.4E  "
                      "%11.4E %11.4E  %11.4E %11.4E  %s",
                ctx->geometry.tag_nums[idx4], itmp4,
                ctx->geometry.tag_nums[idx5], itmp5,
                ctx->netcx.y11_real[j], ctx->netcx.y11_imag[j],
                ctx->netcx.y12_real[j], ctx->netcx.y12_imag[j],
                ctx->netcx.y22_real[j], ctx->netcx.y22_imag[j],
                pnet[itmp2 - 1]);
      }
    }

    if (itmp3 == 0)
      break;

    itmp1 = itmp3;
  }
}

/******************************************************************************
 * write_matrix_asymmetry
 *
 * Writes the maximum and RMS relative asymmetry of the driving point
 * admittance matrix. This data is computed during network solution.
 */
static void write_matrix_asymmetry(FILE *file, const context_t *ctx)
{
  // Only write if asymmetry check was performed and data exists
  if (ctx->netcx.check_asymmetry == 0 || ctx->netcx.max_asymmetry == 0.0)
  {
    return;
  }

  fprintf(file, "\n\n"
                "   MAXIMUM RELATIVE ASYMMETRY OF THE DRIVING POINT ADMITTANCE\n"
                "   MATRIX IS %10.3E FOR SEGMENTS %d AND %d\n"
                "   RMS RELATIVE ASYMMETRY IS %10.3E",
          ctx->netcx.max_asymmetry, ctx->netcx.nteq_asym, ctx->netcx.ntsc_asym, ctx->netcx.rms_asymmetry);
}

/******************************************************************************
 * write_network_excitation
 *
 * Writes structure excitation data at network connection points, including
 * voltage, current, impedance, admittance, and power for each connection.
 */
static void write_network_excitation(FILE *file, const context_t *ctx)
{
  if (ctx->netcx.nexc == 0 || ctx->netcx.print_net_data != 0)
  {
    return; // No excitation data or printing suppressed
  }

  fprintf(file, "\n\n\n"
                "                          "
                "--------- STRUCTURE EXCITATION DATA AT NETWORK CONNECTION POINTS --------");

  fprintf(file, "\n"
                "  TAG   SEG       VOLTAGE (VOLTS)          CURRENT (AMPS)        "
                " IMPEDANCE (OHMS)       ADMITTANCE (MHOS)     POWER\n"
                "  No:   No:     REAL      IMAGINARY     REAL      IMAGINARY    "
                " REAL      IMAGINARY     REAL      IMAGINARY   (WATTS)");

  for (int i = 0; i < ctx->netcx.nexc; i++)
  {
    fprintf(file, "\n"
                  " %4d %5d %11.4E %11.4E %11.4E %11.4E"
                  " %11.4E %11.4E %11.4E %11.4E %11.4E",
            ctx->netcx.exc_tag[i], ctx->netcx.exc_seg[i],
            creal(ctx->netcx.exc_v[i]), cimag(ctx->netcx.exc_v[i]),
            creal(ctx->netcx.exc_i[i]), cimag(ctx->netcx.exc_i[i]),
            creal(ctx->netcx.exc_z[i]), cimag(ctx->netcx.exc_z[i]),
            creal(ctx->netcx.exc_y[i]), cimag(ctx->netcx.exc_y[i]),
            ctx->netcx.exc_pwr[i]);
  }
}

/******************************************************************************
 * write_antenna_input_parameters
 *
 * Writes antenna input parameters at source segments, including voltage,
 * current, impedance, admittance, and power.
 */
static void write_antenna_input_parameters(FILE *file, const context_t *ctx)
{
  if (ctx->netcx.ninp == 0)
  {
    return; // No input data to write
  }

  if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL)
  {
    fprintf(file, "\n\n\n"
                  "                                          "
                  "- - - ANTENNA INPUT PARAMETERS - - -");

    fprintf(file, "\n"
                  "   TAG   SEG.    VOLTAGE (VOLTS)         "
                  "CURRENT (AMPS)         IMPEDANCE (OHMS)        "
                  "ADMITTANCE (MHOS)      POWER\n"
                  "   NO.   NO.    REAL        IMAG.       "
                  "REAL        IMAG.       REAL        IMAG.       "
                  "REAL        IMAG.     (WATTS)");

    for (int i = 0; i < ctx->netcx.ninp; i++)
    {
      fprintf(file, "\n"
                    " %5d %5d %11.5E %11.5E %11.5E%11.5E %11.5E %11.5E %11.5E%11.5E %11.5E",
              ctx->netcx.inp_tag[i], ctx->netcx.inp_seg[i],
              creal(ctx->netcx.inp_v[i]), cimag(ctx->netcx.inp_v[i]),
              creal(ctx->netcx.inp_i[i]), cimag(ctx->netcx.inp_i[i]),
              creal(ctx->netcx.inp_z[i]), cimag(ctx->netcx.inp_z[i]),
              creal(ctx->netcx.inp_y[i]), cimag(ctx->netcx.inp_y[i]),
              ctx->netcx.inp_pwr[i]);
    }
  }
  else
  {
    fprintf(file, "\n\n\n"
                  "                        "
                  "--------- ANTENNA INPUT PARAMETERS ---------");

    fprintf(file, "\n"
                  "  TAG   SEG       VOLTAGE (VOLTS)         "
                  "CURRENT (AMPS)         IMPEDANCE (OHMS)    "
                  "    ADMITTANCE (MHOS)     POWER\n"
                  "  No:   No:     REAL      IMAGINARY"
                  "     REAL      IMAGINARY     REAL      "
                  "IMAGINARY    REAL       IMAGINARY   (WATTS)");

    for (int i = 0; i < ctx->netcx.ninp; i++)
    {
      fprintf(file, "\n"
                    " %4d %5d %11.4E %11.4E %11.4E %11.4E"
                    " %11.4E %11.4E %11.4E %11.4E %11.4E",
              ctx->netcx.inp_tag[i], ctx->netcx.inp_seg[i],
              creal(ctx->netcx.inp_v[i]), cimag(ctx->netcx.inp_v[i]),
              creal(ctx->netcx.inp_i[i]), cimag(ctx->netcx.inp_i[i]),
              creal(ctx->netcx.inp_z[i]), cimag(ctx->netcx.inp_z[i]),
              creal(ctx->netcx.inp_y[i]), cimag(ctx->netcx.inp_y[i]),
              ctx->netcx.inp_pwr[i]);
    }
  }
}

/******************************************************************************
 * write_currents
 *
 * Writes current distribution for all segments, including coordinates,
 * segment length, and current magnitude and phase.
 */
static void write_currents(FILE *file, const context_t *ctx)
{
  if (ctx->geometry.num_segs == 0)
  {
    return; // No segments to write
  }

  if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL)
  {
    fprintf(file, "\n\n\n"
                  "                             "
                  "- - - CURRENTS AND LOCATION - - -\n"
                  "\n"
                  "                                 "
                  "DISTANCES IN WAVELENGTHS");

    fprintf(file, "\n\n"
                  "  SEG.  TAG    COORD. OF SEG. CENTER     SEG.            - - - CURRENT (AMPS) - - -\n"
                  "  NO.   NO.     X        Y        Z      LENGTH     REAL        IMAG.       MAG.        PHASE");

    for (int i = 0; i < ctx->geometry.num_segs; i++)
    {
      complex double curi = ctx->crnt.surface_cur[i] * ctx->geometry.wavelength;
      double cmag = cabs(curi);
      double ph = carg(curi) * TD; // Convert to degrees (TD = 57.29577951)

      char sx[12], sy[12], sz[12];
      format_coord(sx, sizeof(sx), ctx->geometry.x_center[i], "%8.4f");
      format_coord(sy, sizeof(sy), ctx->geometry.y_center[i], "%8.4f");
      format_coord(sz, sizeof(sz), ctx->geometry.z_center[i], "%8.4f");

      fprintf(file, "\n"
                    " %5d %4d %s %s %s %8.5f  %11.4E %11.4E %11.4E %8.3f",
              i + 1, ctx->geometry.tag_nums[i],
              sx, sy, sz,
              ctx->geometry.half_len[i],
              creal(curi), cimag(curi), cmag, ph);
    }
  }
  else
  {
    fprintf(file, "\n\n\n"
                  "                           "
                  "-------- CURRENTS AND LOCATION --------\n"
                  "                                  "
                  "DISTANCES IN WAVELENGTHS");

    fprintf(file, "\n\n"
                  "   SEG  TAG    COORDINATES OF SEGM CENTER     SEGM"
                  "    ------------- CURRENT (AMPS) -------------\n"
                  "   No:  No:       X         Y         Z      LENGTH"
                  "     REAL      IMAGINARY    MAGN        PHASE");

    for (int i = 0; i < ctx->geometry.num_segs; i++)
    {
      complex double curi = ctx->crnt.surface_cur[i] * ctx->geometry.wavelength;
      double cmag = cabs(curi);
      double ph = carg(curi) * TD; // Convert to degrees (TD = 57.29577951)

      char sx[12], sy[12], sz[12];
      format_coord(sx, sizeof(sx), ctx->geometry.x_center[i], "%9.4f");
      format_coord(sy, sizeof(sy), ctx->geometry.y_center[i], "%9.4f");
      format_coord(sz, sizeof(sz), ctx->geometry.z_center[i], "%9.4f");

      fprintf(file, "\n"
                    " %5d %4d %s %s %s %8.5f %11.4E %11.4E %11.4E %9.3f",
              i + 1, ctx->geometry.tag_nums[i],
              sx, sy, sz,
              ctx->geometry.half_len[i],
              creal(curi), cimag(curi), cmag, ph);
    }
  }
}

/******************************************************************************
 * write_power_budget
 *
 * Writes the power budget showing input power, radiated power, structure
 * loss, network loss, and efficiency.
 */
static void write_power_budget(FILE *file, const context_t *ctx)
{
  // Only write for standard radiation pattern types
  if ((ctx->fpat.excitation_type != 0) && (ctx->fpat.excitation_type != 5))
  {
    return;
  }

  double tmp1 = ctx->netcx.power_in - ctx->netcx.power_net_loss - ctx->fpat.ohmic_loss;
  double tmp2 = 100.0 * tmp1 / ctx->netcx.power_in;

  if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL)
  {
    fprintf(file, "\n\n\n"
                  "                                        "
                  "- - - POWER BUDGET - - -\n"
                  "                               "
                  "INPUT POWER   = %11.4E Watts\n"
                  "                               "
                  "RADIATED POWER= %11.4E Watts\n"
                  "                               "
                  "STRUCTURE LOSS= %11.4E Watts\n"
                  "                               "
                  "NETWORK LOSS  = %11.4E Watts\n"
                  "                               "
                  "EFFICIENCY    = %7.2f Percent",
            ctx->netcx.power_in, tmp1, ctx->fpat.ohmic_loss, ctx->netcx.power_net_loss, tmp2);
  }
  else
  {
    fprintf(file, "\n\n\n"
                  "                               "
                  "---------- POWER BUDGET ---------\n"
                  "                               "
                  "INPUT POWER   = %11.4E Watts\n"
                  "                               "
                  "RADIATED POWER= %11.4E Watts\n"
                  "                               "
                  "STRUCTURE LOSS= %11.4E Watts\n"
                  "                               "
                  "NETWORK LOSS  = %11.4E Watts\n"
                  "                               "
                  "EFFICIENCY    = %7.2f Percent",
            ctx->netcx.power_in, tmp1, ctx->fpat.ohmic_loss, ctx->netcx.power_net_loss, tmp2);
  }
}

/******************************************************************************
 * write_radiation_pattern_header
 *
 * Writes the radiation pattern section header and column headers.
 */
static void write_radiation_pattern_header(FILE *file, const context_t *ctx)
{
  char *igtp[2] = {"----- POWER GAINS ----- ", "--- DIRECTIVE GAINS ---"};
  char *igax[4] = {" MAJOR", " MINOR", " VERTC", " HORIZ"};

  // Check if radiation pattern was calculated
  if (ctx->rpat.num_points == 0 || ctx->rpat.points == NULL)
  {
    return;
  }

  /* Write ground parameters if applicable */
  if (ctx->gnd.far_field_type > 1)
  {
    fprintf(file, "\n\n\n"
                  "                               "
                  "------ FAR FIELD GROUND PARAMETERS ------\n\n");

    if (ctx->gnd.far_field_type > 3)
    {
      fprintf(file, "\n"
                    "                               "
                    "--- RADIAL WIRE GROUND SCREEN ---\n"
                    "                               "
                    "NUM OF WIRES= %d\n"
                    "                               "
                    "WIRE LENGTH= %8.2f METERS\n"
                    "                               "
                    "WIRE RADIUS= %10.3E METERS",
              ctx->gnd.num_radials, ctx->save.screen_wire_len, ctx->save.screen_wire_radius);
    }

    if (ctx->gnd.far_field_type != 4 && strlen(ctx->rpat.ground_cliff_type) > 0)
    {
      fprintf(file, "\n"
                    "                               "
                    "--- %s CLIFF ---\n"
                    "                               "
                    "EDGE DISTANCE= %9.2f METERS\n"
                    "                               "
                    "       HEIGHT= %9.2f METERS\n"
                    "                               "
                    "--- SECOND MEDIUM ---\n"
                    "                               "
                    "RELATIVE DIELECTRIC CONST= %10.3f\n"
                    "                               "
                    "      GROUND CONDUCTIVITY= %10.3f MHOS",
              ctx->rpat.ground_cliff_type, ctx->fpat.cliff_dist, ctx->fpat.cliff_height,
              ctx->fpat.epsr2, ctx->fpat.sigma2);
    }
  }

  /* Write main header */
  if (ctx->gnd.far_field_type == 1)
  {
    fprintf(file, "\n\n\n"
                  "                             "
                  "------- RADIATED FIELDS NEAR GROUND --------\n\n"
                  "    ------- LOCATION -------     --- E(THETA) ---    "
                  " ---- E(PHI) ----    --- E(RADIAL) ---\n"
                  "      RHO    PHI        Z           MAG    PHASE     "
                  "    MAG    PHASE        MAG     PHASE\n"
                  "    METERS DEGREES    METERS      VOLTS/M DEGREES   "
                  "   VOLTS/M DEGREES     VOLTS/M  DEGREES");
  }
  else
  {
    int itmp1 = 2 * ctx->fpat.pol_axis;
    int itmp2 = itmp1 + 1;

    if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL)
    {
      fprintf(file, "\n\n\n"
                    "                               "
                    "- - - RADIATION PATTERNS - - -\n");
    }
    else
    {
      fprintf(file, "\n\n\n"
                    "                             "
                    "---------- RADIATION PATTERNS -----------\n");
    }

    if (ctx->fpat.range >= 1.0e-20)
    {
      fprintf(file, "\n"
                    "                             "
                    "RANGE: %13.6E METERS\n"
                    "                             "
                    "EXP(-JKR)/R: %12.5E AT PHASE: %7.2f DEGREES\n",
              ctx->fpat.range, ctx->rpat.exrm, ctx->rpat.exra);
    }

    if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL)
    {
      /* Original Fortran format headers */
      fprintf(file, "\n"
                    "  - - ANGLES - -           - POWER GAINS -       - - - POLARIZATION - - -    - - - E(THETA) - - -    - - - E(PHI) - - -\n"
                    "  THETA     PHI        VERT.   HOR.    TOTAL      AXIAL     TILT   SENSE     MAGNITUDE    PHASE      MAGNITUDE    PHASE \n"
                    " DEGREES  DEGREES       DB      DB      DB        RATIO     DEG.              VOLTS/M    DEGREES      VOLTS/M    DEGREES");
    }
    else
    {
      /* nec2c format headers */
      fprintf(file, "\n"
                    " ---- ANGLES -----     %23s      ---- POLARIZATION ----  "
                    " ---- E(THETA) ----    ----- E(PHI) ------\n"
                    "  THETA      PHI      %6s   %6s    TOTAL       AXIAL    "
                    "  TILT  SENSE   MAGNITUDE    PHASE    MAGNITUDE     PHASE\n"
                    " DEGREES   DEGREES        DB       DB       DB       RATIO  "
                    " DEGREES            VOLTS/M   DEGREES     VOLTS/M   DEGREES",
              igtp[ctx->fpat.gain_type], igax[itmp1], igax[itmp2]);
    }
  }
}

/******************************************************************************
 * write_radiation_pattern_data
 *
 * Writes the computed radiation pattern data for each theta/phi point.
 * Data includes gains, polarization, and E-field components.
 */
static void write_radiation_pattern_data(FILE *file, const context_t *ctx)
{
  char *hpol[3] = {"LINEAR", "RIGHT ", "LEFT  "};
  double tmp5, tmp6;

  if (ctx->rpat.num_points == 0 || ctx->rpat.points == NULL)
  {
    return;
  }

  /* Write data for each point */
  for (int i = 0; i < ctx->rpat.num_points; i++)
  {
    rpat_point_t *pt = &ctx->rpat.points[i];

    if (ctx->gnd.far_field_type == 1)
    {
      /* Near field output */
      fprintf(file, "\n"
                    " %9.2f %7.2f %9.2f  %11.4E %7.2f  %11.4E %7.2f  %11.4E %7.2f",
              ctx->fpat.range, pt->phi, pt->theta,
              pt->ethm, pt->etha, pt->ephm, pt->epha, pt->erdm, pt->erda);
    }
    else
    {
      /* Far field output */
      if (ctx->fpat.pol_axis != 1)
      {
        tmp5 = pt->gnmj;
        tmp6 = pt->gnmn;
      }
      else
      {
        tmp5 = pt->gnv;
        tmp6 = pt->gnh;
      }

      if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL)
      {
        /* Original Fortran NEC-2 format (from FORMAT statement #42) */
        /* FORMAT(1X,F7.2,F9.2,3X,3F8.2,F11.5,F9.2,2X,A6,2(1P,E15.5,0P,F9.2)) */
        fprintf(file, "\n"
                      " %7.2f%9.2f   %8.2f%8.2f%8.2f%11.5f%9.2f  %6s%15.5E%9.2f%15.5E%9.2f",
                pt->theta, pt->phi, tmp5, tmp6, pt->gtot, pt->axrat,
                pt->tilta, hpol[pt->pol_sense >= 0 && pt->pol_sense <= 2 ? pt->pol_sense : 0],
                pt->ethm, pt->etha, pt->ephm, pt->epha);
      }
      else
      {
        /* Modern nec2c format */
        fprintf(file, "\n"
                      " %7.2f %9.2f  %8.2f %8.2f %8.2f %11.4f"
                      " %9.2f %6s %11.4E %9.2f %11.4E %9.2f",
                pt->theta, pt->phi, tmp5, tmp6, pt->gtot, pt->axrat,
                pt->tilta, hpol[pt->pol_sense >= 0 && pt->pol_sense <= 2 ? pt->pol_sense : 0],
                pt->ethm, pt->etha, pt->ephm, pt->epha);
      }
    }
  }
}

/******************************************************************************
 * write_average_power_gain
 *
 * Writes the average power gain over the specified solid angle.
 */
static void write_average_power_gain(FILE *file, const context_t *ctx)
{
  if (ctx->fpat.avg_power_flag == 0)
  {
    return;
  }

  fprintf(file, "\n\n\n"
                "  AVERAGE POWER GAIN: %11.4E - SOLID ANGLE"
                " USED IN AVERAGING: (%+7.4f)*PI STERADIANS",
          ctx->rpat.pint, ctx->rpat.solid_angle);
}

/******************************************************************************
 * write_normalized_gain
 *
 * Writes the normalized gain table if requested.
 */
static void write_normalized_gain(FILE *file, const context_t *ctx)
{
  char *igntp[5] = {" MAJOR AXIS", "  MINOR AXIS",
                    "    VERTICAL", "  HORIZONTAL", "       TOTAL "};

  if (ctx->fpat.normalize_gain == 0 || ctx->rpat.num_points == 0)
  {
    return;
  }

  int itmp1 = ctx->fpat.normalize_gain - 1;

  fprintf(file, "\n\n\n"
                "                             "
                " ---------- NORMALIZED GAIN ----------\n"
                "                                      %6s GAIN\n"
                "                                  "
                " NORMALIZATION FACTOR: %.2f db\n\n"
                "    ---- ANGLES ----                ---- ANGLES ----"
                "                ---- ANGLES ----\n"
                "    THETA      PHI        GAIN      THETA      PHI  "
                "      GAIN      THETA      PHI       GAIN\n"
                "   DEGREES   DEGREES        DB     DEGREES   DEGREES "
                "       DB     DEGREES   DEGREES       DB",
          igntp[itmp1], ctx->rpat.gmax);

  /* Print normalized gain in three columns */
  int itmp2 = ctx->rpat.num_points;
  int itmp3 = (itmp2 + 2) / 3;
  int itmp4 = itmp3 * 3 - itmp2;
  int idx1 = itmp3;
  int idx2 = 2 * itmp3;

  if (itmp4 == 2)
    idx2--;

  for (int i = 0; i < itmp3; i++)
  {
    rpat_point_t *pt1 = &ctx->rpat.points[i];
    double gain1;

    switch (ctx->fpat.normalize_gain)
    {
    case 1:
      gain1 = pt1->gnmj;
      break;
    case 2:
      gain1 = pt1->gnmn;
      break;
    case 3:
      gain1 = pt1->gnv;
      break;
    case 4:
      gain1 = pt1->gnh;
      break;
    case 5:
      gain1 = pt1->gtot;
      break;
    default:
      gain1 = pt1->gtot;
      break;
    }
    gain1 -= ctx->rpat.gmax;

    /* Check if we need fewer than 3 columns on the last row */
    if ((i + 1) == itmp3 && itmp4 != 0)
    {
      if (itmp4 != 2 && idx1 < ctx->rpat.num_points)
      {
        rpat_point_t *pt2 = &ctx->rpat.points[idx1];
        double gain2;
        switch (ctx->fpat.normalize_gain)
        {
        case 1:
          gain2 = pt2->gnmj;
          break;
        case 2:
          gain2 = pt2->gnmn;
          break;
        case 3:
          gain2 = pt2->gnv;
          break;
        case 4:
          gain2 = pt2->gnh;
          break;
        case 5:
          gain2 = pt2->gtot;
          break;
        default:
          gain2 = pt2->gtot;
          break;
        }
        gain2 -= ctx->rpat.gmax;
        fprintf(file, "\n"
                      " %9.2f %9.2f %9.2f   %9.2f %9.2f %9.2f   ",
                pt1->theta, pt1->phi, gain1, pt2->theta, pt2->phi, gain2);
      }
      else
      {
        fprintf(file, "\n"
                      " %9.2f %9.2f %9.2f   ",
                pt1->theta, pt1->phi, gain1);
      }
      break;
    }

    /* Print all three columns */
    if (idx1 < ctx->rpat.num_points && idx2 < ctx->rpat.num_points)
    {
      rpat_point_t *pt2 = &ctx->rpat.points[idx1];
      rpat_point_t *pt3 = &ctx->rpat.points[idx2];
      double gain2, gain3;

      switch (ctx->fpat.normalize_gain)
      {
      case 1:
        gain2 = pt2->gnmj;
        gain3 = pt3->gnmj;
        break;
      case 2:
        gain2 = pt2->gnmn;
        gain3 = pt3->gnmn;
        break;
      case 3:
        gain2 = pt2->gnv;
        gain3 = pt3->gnv;
        break;
      case 4:
        gain2 = pt2->gnh;
        gain3 = pt3->gnh;
        break;
      case 5:
        gain2 = pt2->gtot;
        gain3 = pt3->gtot;
        break;
      default:
        gain2 = pt2->gtot;
        gain3 = pt3->gtot;
        break;
      }
      gain2 -= ctx->rpat.gmax;
      gain3 -= ctx->rpat.gmax;

      fprintf(file, "\n"
                    " %9.2f %9.2f %9.2f   %9.2f %9.2f %9.2f   %9.2f %9.2f %9.2f",
              pt1->theta, pt1->phi, gain1,
              pt2->theta, pt2->phi, gain2,
              pt3->theta, pt3->phi, gain3);
    }

    idx1++;
    idx2++;
  }
}

/******************************************************************************
 * write_near_field_data
 *
 * Writes the near electric or magnetic field results accumulated in
 * ctx->nfr by compute_near_field().  No-op if no points were recorded.
 */
static void write_near_field_data(FILE *file, const context_t *ctx)
{
  if (ctx->nfr.num_points == 0 || ctx->nfr.points == NULL)
    return;

  if (ctx->nfr.nfeh != 1)
  {
    fprintf(file, "\n\n\n"
            "                             "
            "-------- NEAR ELECTRIC FIELDS --------\n"
            "     ------- LOCATION -------     ------- EX ------    ------- EY ------    ------- EZ ------\n"
            "      X         Y         Z       MAGNITUDE   PHASE    MAGNITUDE   PHASE    MAGNITUDE   PHASE\n"
            "    METERS    METERS    METERS     VOLTS/M  DEGREES    VOLTS/M   DEGREES     VOLTS/M  DEGREES");
  }
  else
  {
    fprintf(file, "\n\n\n"
            "                                   "
            "-------- NEAR MAGNETIC FIELDS ---------\n\n"
            "     ------- LOCATION -------     ------- HX ------    ------- HY ------    ------- HZ ------\n"
            "      X         Y         Z       MAGNITUDE   PHASE    MAGNITUDE   PHASE    MAGNITUDE   PHASE\n"
            "    METERS    METERS    METERS      AMPS/M  DEGREES      AMPS/M  DEGREES      AMPS/M  DEGREES");
  }

  for (int i = 0; i < ctx->nfr.num_points; i++)
  {
    near_field_point_t *pt = &ctx->nfr.points[i];
    double tmp1 = cabs(pt->ex);
    double tmp2 = complex_angle_deg(ctx, pt->ex);
    double tmp3 = cabs(pt->ey);
    double tmp4 = complex_angle_deg(ctx, pt->ey);
    double tmp5 = cabs(pt->ez);
    double tmp6 = complex_angle_deg(ctx, pt->ez);
    fprintf(file, "\n"
            " %9.4f %9.4f %9.4f  %11.4E %7.2f  %11.4E %7.2f  %11.4E %7.2f",
            pt->xob, pt->yob, pt->zob, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6);
  }
}

/******************************************************************************
 * write_near_field_plot
 *
 * Writes near-field data to the plot file (ctx->plot_fp) if PT/PQ plot output
 * was requested.  No-op if no data, no plot file, or iplp1 != 2.
 */
static void write_near_field_plot(const context_t *ctx)
{
  if (ctx->nfr.num_points == 0 || ctx->nfr.points == NULL)
    return;
  if (ctx->plot.plot_type != 2 || ctx->plot_fp == NULL)
    return;

  for (int i = 0; i < ctx->nfr.num_points; i++)
  {
    near_field_point_t *pt = &ctx->nfr.points[i];

    double xxx;
    if (ctx->plot.plot_gain_type < 0)
      xxx = pt->xob;
    else if (ctx->plot.plot_gain_type == 0)
      xxx = pt->yob;
    else
      xxx = pt->zob;

    double tmp1 = cabs(pt->ex);
    double tmp2 = complex_angle_deg(ctx, pt->ex);
    double tmp3 = cabs(pt->ey);
    double tmp4 = complex_angle_deg(ctx, pt->ey);
    double tmp5 = cabs(pt->ez);
    double tmp6 = complex_angle_deg(ctx, pt->ez);

    if (ctx->plot.plot_axis == 2)
    {
      switch (ctx->plot.plot_component)
      {
        case 1:
          fprintf(ctx->plot_fp, "%12.4E %12.4E %12.4E\n", xxx, tmp1, tmp2);
          break;
        case 2:
          fprintf(ctx->plot_fp, "%12.4E %12.4E %12.4E\n", xxx, tmp3, tmp4);
          break;
        case 3:
          fprintf(ctx->plot_fp, "%12.4E %12.4E %12.4E\n", xxx, tmp5, tmp6);
          break;
        case 4:
          fprintf(ctx->plot_fp, "%12.4E %12.4E %12.4E %12.4E %12.4E %12.4E %12.4E\n",
                  xxx, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6);
      }
    }
    else if (ctx->plot.plot_axis == 1)
    {
      switch (ctx->plot.plot_component)
      {
        case 1:
          fprintf(ctx->plot_fp, "%12.4E %12.4E %12.4E\n",
                  xxx, creal(pt->ex), cimag(pt->ex));
          break;
        case 2:
          fprintf(ctx->plot_fp, "%12.4E %12.4E %12.4E\n",
                  xxx, creal(pt->ey), cimag(pt->ey));
          break;
        case 3:
          fprintf(ctx->plot_fp, "%12.4E %12.4E %12.4E\n",
                  xxx, creal(pt->ez), cimag(pt->ez));
          break;
        case 4:
          fprintf(ctx->plot_fp, "%12.4E %12.4E %12.4E %12.4E %12.4E %12.4E %12.4E\n",
                  xxx, creal(pt->ex), cimag(pt->ex),
                  creal(pt->ey), cimag(pt->ey),
                  creal(pt->ez), cimag(pt->ez));
      }
    }
  }
}

/******************************************************************************
 * write_footer
 *
 * Writes the footer with total run time.
 */
void write_footer(FILE *file, const context_t *ctx, const deck_t *deck)
{
  /* Output end cards (EN/NX) and implicit EN if needed */
  write_end_cards(file, deck);

  // Calculate and output total runtime
  if (ctx != NULL)
  {
    double current_time;
    get_time_ms(ctx, &current_time);
    double elapsed_ms = current_time - ctx->start_time;
    
    if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL)
    {
      /* Original Fortran format: " RUN TIME = XXX.XXX" (in seconds) */
      fprintf(file, "\n RUN TIME = %9.3f\n", elapsed_ms / 1000.0);
    }
    else
    {
      /* nec2c format: "  TOTAL RUN TIME: X msec" */
      fprintf(file, "\n  TOTAL RUN TIME: %.0f msec\n", elapsed_ms);
    }
  }
}

/* end of output.c */
