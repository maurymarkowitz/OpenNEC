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

/* Forward declarations for internal write functions */
static void write_header(const nec_context_t *ctx, const deck_t *deck, FILE *pfile);
static int write_structure(nec_context_t *ctx, const deck_t *deck, FILE *pfile);
static int write_segments(nec_context_t *ctx, const deck_t *deck, FILE *pfile);
static void write_patches(const nec_context_t *ctx, const deck_t *deck, FILE *pfile);
static void write_input_cards(FILE *file, const deck_t *deck, int batch_start, int batch_end, int card_number_offset);
static void write_frequency_data(FILE *file, const nec_context_t *ctx);
static void write_loading_data(FILE *file, const nec_context_t *ctx);
static void write_environment_data(FILE *file, const nec_context_t *ctx);
static void write_matrix_timing(FILE *file, const nec_context_t *ctx);
static void write_network_data(FILE *file, const nec_context_t *ctx);
static void write_matrix_asymmetry(FILE *file, const nec_context_t *ctx);
static void write_network_excitation(FILE *file, const nec_context_t *ctx);
static void write_antenna_input_parameters(FILE *file, const nec_context_t *ctx);
static void write_coupling_data(nec_context_t *ctx);
static void write_currents(FILE *file, const nec_context_t *ctx);
static void write_power_budget(FILE *file, const nec_context_t *ctx);
static void write_radiation_pattern_header(FILE *file, const nec_context_t *ctx);
static void write_radiation_pattern_data(FILE *file, const nec_context_t *ctx);
static void write_average_power_gain(FILE *file, const nec_context_t *ctx);
static void write_normalized_gain(FILE *file, const nec_context_t *ctx);
static void write_footer(FILE *file, const nec_context_t *ctx, const deck_t *deck);

/******************************************************************************
 * write_deck_nec
 *
 * Writes a deck in the original NEC2 format. This strips out any
 * extensions like SY, replaces formulas and variables with their
 * numeric values, and optionally strips out any inline or in-deck
 * comments. With this last option turned off, the deck is compatible
 * with nec2c, with it turned on, it is the original NEC2 format.
 *
 */
void write_deck_nec(const nec_context_t *ctx, const deck_t *deck, FILE *file, int remove_inline_comments)
{
  card_t *card;
  int MAX_INTS, MAX_FLTS;

  for (int i = 0; i < deck->num_cards; i++)
  {
    card = &deck->cards[i];

    // if we are past the end of the deck, just write out the whole string
    if (i > deck->deck_end)
    {
      fputs(card->card_str, file);
      fputc('\n', file);
      continue;
    }

    // skip extension cards in pure NEC files
    if (is_extension(card))
      continue;

    // for comment cards with the CM or CE *in the header*, simply export the card
    if (i <= deck->geometry_start && (strcmp(card->card_code, "CM") == 0 || strcmp(card->card_code, "CE") == 0))
    {
      fprintf(file, "%s%s", deck->cards[i].card_code, deck->cards[i].comment);
      fputc('\n', file);
    }
    // for comment cards with other headers, only export if the option is on
    if (is_comment(card))
    {
      fprintf(file, "%s%s", deck->cards[i].card_code, deck->cards[i].comment);
      fputc('\n', file);
    }

    // for geometry and command cards, start with the code
    fputs(card->card_code, file);

    // get the number of fields for this sort of card
    MAX_INTS = max_int_fields(card);
    MAX_FLTS = max_flt_fields(card);

    // int and float fields
    if (is_control(card) || is_geometry(card))
    {
      for (int j = 0; j <= card->ints_used && j <= MAX_INTS; j++)
      {
        // Look up formula for this integer field
        char key[8];
        snprintf(key, sizeof(key), "I%d", j);
        const char *formula = lookup_formula(card, key);

        if (formula != NULL)
        {
          fprintf(file, " %s", formula);
        }
        else
        {
          fprintf(file, " %d", card->i[j]);
        }
      }
      for (int j = 0; j <= card->flts_used && j <= MAX_FLTS; j++)
      {
        // Look up formula for this float field
        char key[8];
        snprintf(key, sizeof(key), "F%d", j);
        const char *formula = lookup_formula(card, key);

        if (formula != NULL)
        {
          fprintf(file, " %s", formula);
        }
        else
        {
          fprintf(file, " %G", card->f[j]);
        }
      }
      // close the line
      fputc('\n', file);
    } /* if command or geometry */
  } /* for over cards */
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
void write_deck_onec(const nec_context_t *ctx, const deck_t *deck, FILE *file)
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
        fputc(' ', file);
        fprintf(file, " %d", card->i[1]);
        fputc(' ', file);
        fprintf(file, " %3d", card->i[2]);
      }
      // other cards might have a formula
      else
      {
        for (int j = 1; j <= card->ints_used && j <= MAX_INTS; j++)
        {
          // Look up formula for this integer field (fields are 1-based: I1..I4)
          char key[8];
          snprintf(key, sizeof(key), "I%d", j);
          const char *formula = lookup_formula(card, key);

          if (formula != NULL)
          {
            fprintf(file, " %s", formula);
          }
          else
          {
            fprintf(file, " %d", card->i[j]);
          }
        }
      }

      // floats are a number or a formula (fields are 1-based: F1..F7)
      for (int j = 1; j <= card->flts_used && j <= MAX_FLTS; j++)
      {
        // Look up formula for this float field
        char key[8];
        snprintf(key, sizeof(key), "F%d", j);
        const char *formula = lookup_formula(card, key);

        if (formula != NULL)
        {
          fprintf(file, " %s", formula);
        }
        else
        {
          fprintf(file, " %G", card->f[j]);
        }
      }

      // the basic NEC fields are output, now see if there's anything after that

      // Compute hasOnec first — needed to decide whether extn_str is safe to use
      // as a plain comment fallback (see comment_text below).
      bool hasOnec = false;
      // only treat ignore as an onec annotation if it's the annotated form (not prefix-commented)
      if (card->ignore && card->cmt_code[0] == '\0')
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
          key_value_t *pair = card->extensns;
          while (pair != NULL)
          {
            fputc(' ', file);
            fputs(pair->key, file);
            fputc('=', file);
            fputs(pair->value, file);
            pair = pair->next;
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
 * Renders the CP (coupling) isolation table accumulated by couple() in
 * calculations.c.  No-op if no coupling rows were recorded.
 */
static void write_coupling_data(nec_context_t *ctx)
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
void write_nec_output(nec_context_t *ctx, const deck_t *deck, FILE *file)
{
  write_header(ctx, deck, file);
  write_structure(ctx, deck, file);
  write_segments(ctx, deck, file);
  write_patches(ctx, deck, file);
  // Write all control cards (for now as single batch - full XQ support pending)
  write_input_cards(file, deck, deck->geometry_end + 1, deck->deck_end, 0);
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
bool write_greens_binary(FILE *file, const nec_context_t *ctx,
                         int neq, const complex double *cm)
{
  if (!file || !cm || neq <= 0)
    return false;

  const int N = ctx->geometry.n;
  const int NP = ctx->geometry.np; /* segs per symmetry copy (= N for no symmetry) */
  const int M = ctx->geometry.m;
  const int MP = ctx->geometry.mp;

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
    AP8(ctx->geometry.wlam);
    AP8(ctx->save.fmhz);
    AP4(ctx->geometry.ipsym);
    AP4(ctx->gnd.ksymp);
    /* Write IPERF <= 1: OpenNEC does not write a patch-coefficient record
     * for finite-ground (iperf==2).  The reader will try to skip that record
     * when IPERF==2 in the header, so cap it at 1 so the skip is never
     * attempted when reading back an OpenNEC-written NGF file.
     * (Fortran-generated .wgf files with IPERF==2 still work because they
     *  contain the patch-coefficient record that the read-side fr_skip needs.) */
    AP4(ctx->gnd.iperf > 1 ? 1 : ctx->gnd.iperf);
    AP4(ctx->gnd.nradl);
    AP8(ctx->save.epsr);
    AP8(ctx->save.sig);
    AP8(ctx->gnd.scrwl);
    AP8(ctx->gnd.scrwr);
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
    if (!fw3(file, ctx->geometry.x, n8,
             ctx->geometry.y, n8,
             ctx->geometry.z, n8))
      return false;

    /* Rec 3: SI[N], BI[N], ALP[N]  (ALP = cab = x-direction cosine) */
    if (!fw3(file, ctx->geometry.si, n8,
             ctx->geometry.bi, n8,
             ctx->geometry.cab, n8))
      return false;

    /* Rec 4: BET[N], SALP[N]  (BET = sab, SALP = salp) */
    if (!fw2(file, ctx->geometry.sab, n8,
             ctx->geometry.salp, n8))
      return false;

    /* Rec 5: ICON1[N], ICON2[N] — OpenNEC stores as int; cast to int32_t */
    {
      int32_t *tmp = (int32_t *)malloc(2 * (size_t)N * sizeof(int32_t));
      if (!tmp)
        return false;
      for (int i = 0; i < N; i++)
        tmp[i] = (int32_t)ctx->geometry.icon1[i];
      for (int i = 0; i < N; i++)
        tmp[N + i] = (int32_t)ctx->geometry.icon2[i];
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

    if (ctx->geometry.ipsym <= 0)
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
bool read_greens_binary(FILE *file, nec_context_t *ctx)
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

    mem_realloc(ctx, (void *)&ctx->geometry.x, nd);
    mem_realloc(ctx, (void *)&ctx->geometry.y, nd);
    mem_realloc(ctx, (void *)&ctx->geometry.z, nd);
    mem_realloc(ctx, (void *)&ctx->geometry.si, nd);
    mem_realloc(ctx, (void *)&ctx->geometry.bi, nd);
    mem_realloc(ctx, (void *)&ctx->geometry.cab, nd);
    mem_realloc(ctx, (void *)&ctx->geometry.sab, nd);
    mem_realloc(ctx, (void *)&ctx->geometry.salp, nd);
    mem_realloc(ctx, (void *)&ctx->geometry.icon1, ni);
    mem_realloc(ctx, (void *)&ctx->geometry.icon2, ni);
    mem_realloc(ctx, (void *)&ctx->geometry.tag_nums, ni);
    mem_realloc(ctx, (void *)&ctx->geometry.card_nums, ni);
    mem_realloc(ctx, (void *)&ctx->geometry.x1, nd);
    mem_realloc(ctx, (void *)&ctx->geometry.y1, nd);
    mem_realloc(ctx, (void *)&ctx->geometry.z1, nd);
    mem_realloc(ctx, (void *)&ctx->geometry.x2, nd);
    mem_realloc(ctx, (void *)&ctx->geometry.y2, nd);
    mem_realloc(ctx, (void *)&ctx->geometry.z2, nd);

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
      memcpy(ctx->geometry.x, (double *)buf, N * sizeof(double));
      memcpy(ctx->geometry.y, (double *)buf + N, N * sizeof(double));
      memcpy(ctx->geometry.z, (double *)buf + 2 * N, N * sizeof(double));
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
      memcpy(ctx->geometry.si, (double *)buf, N * sizeof(double));
      memcpy(ctx->geometry.bi, (double *)buf + N, N * sizeof(double));
      memcpy(ctx->geometry.cab, (double *)buf + 2 * N, N * sizeof(double));
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
      memcpy(ctx->geometry.sab, (double *)buf, N * sizeof(double));
      memcpy(ctx->geometry.salp, (double *)buf + N, N * sizeof(double));
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
        ctx->geometry.icon1[i] = (int)tmp[i];
      for (int i = 0; i < N; i++)
        ctx->geometry.icon2[i] = (int)tmp[N + i];
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
      ctx->geometry.x[i] *= WLAM;
      ctx->geometry.y[i] *= WLAM;
      ctx->geometry.z[i] *= WLAM;
      ctx->geometry.si[i] *= WLAM;
      ctx->geometry.bi[i] *= WLAM;
    }

    /* Reconstruct wire endpoints from midpoint + direction cosines */
    for (int i = 0; i < N; i++)
    {
      double hs = ctx->geometry.si[i] * 0.5;
      ctx->geometry.x1[i] = ctx->geometry.x[i] - hs * ctx->geometry.cab[i];
      ctx->geometry.y1[i] = ctx->geometry.y[i] - hs * ctx->geometry.sab[i];
      ctx->geometry.z1[i] = ctx->geometry.z[i] - hs * ctx->geometry.salp[i];
      ctx->geometry.x2[i] = ctx->geometry.x[i] + hs * ctx->geometry.cab[i];
      ctx->geometry.y2[i] = ctx->geometry.y[i] + hs * ctx->geometry.sab[i];
      ctx->geometry.z2[i] = ctx->geometry.z[i] + hs * ctx->geometry.salp[i];
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
  ctx->geometry.n = (int)N;
  ctx->geometry.np = (NP > 0) ? (int)NP : (int)N;
  ctx->geometry.m = (int)M;
  ctx->geometry.mp = (int)MP;
  ctx->geometry.wlam = WLAM;
  ctx->geometry.ipsym = (int)IPSYM;
  ctx->geometry.npm = (int)(N + M);
  ctx->geometry.np2m = (int)(N + 2 * M);
  ctx->geometry.np3m = (int)(N + 3 * M);

  /* Restore ground and frequency parameters from the NGF */
  ctx->gnd.ksymp = (int)KSYMP;
  ctx->gnd.iperf = (int)IPERF;
  ctx->gnd.nradl = (int)NRADL;
  ctx->gnd.scrwl = SCRWLT;
  ctx->gnd.scrwr = SCRWRT;
  ctx->save.epsr = EPSR;
  ctx->save.sig = SIG;
  ctx->save.fmhz = FMHZ;

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
static void write_header(const nec_context_t *ctx, const deck_t *deck, FILE *file)
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

  fprintf(ctx->output_fp, "\n\n\n"
                          "                               "
                          "---------------- COMMENTS ----------------\n");

  // write header comments to output file (CM/CE cards in the comment section only)
  int cstart = (deck->comment_start >= 0) ? deck->comment_start : 0;
  int cend = (deck->comment_end >= 0) ? deck->comment_end : deck->geometry_start - 1;
  for (int i = cstart; i <= cend && i < deck->num_cards; i++)
  {
    card_t *card = &deck->cards[i];
    if ((strcmp(card->card_code, "CM") == 0 || strcmp(card->card_code, "CE") == 0) &&
        card->comment)
    {
      fprintf(ctx->output_fp, "                              %s\n", card->comment);
    }
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
static int write_structure(nec_context_t *ctx, const deck_t *deck, FILE *file)
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
  fprintf(file, "\n\n\n"
                "                               "
                "-------- STRUCTURE SPECIFICATION --------\n"
                "                                     "
                "COORDINATES MUST BE INPUT IN\n"
                "                                     "
                "METERS OR BE SCALED TO METERS\n"
                "                                     "
                "BEFORE STRUCTURE INPUT IS ENDED\n");

  fprintf(ctx->output_fp, "\n"
                          "  WIRE                                           "
                          "                                      SEG FIRST  LAST  TAG\n"
                          "   NO.        X1         Y1         Z1         X2      "
                          "   Y2         Z2       RADIUS   NO. SEG.   SEG.  NO.");

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
      fprintf(ctx->output_fp, "\n"
                              " %5d  %10.5f %10.5f %10.5f %10.5f"
                              " %10.5f %10.5f %10.5f %5d %5d %5d %4d",
              num_wires, card.f[1], card.f[2], card.f[3], card.f[4], card.f[5], card.f[6], card.f[7],
              card.num_segments, card.start_segment, card.end_segment, card.tag);
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
  fprintf(ctx->output_fp, "\n\n"
                          "     TOTAL SEGMENTS USED: %d   SEGMENTS IN A"
                          " SYMMETRIC CELL: %d   SYMMETRY FLAG: %d",
          ctx->geometry.n, ctx->geometry.np, ctx->geometry.ipsym);

  if (ctx->geometry.m > 0)
    fprintf(ctx->output_fp, "\n"
                            "       TOTAL PATCHES USED: %d   PATCHES"
                            " IN A SYMMETRIC CELL: %d",
            ctx->geometry.m, ctx->geometry.mp);

  int iseg = (ctx->geometry.n + ctx->geometry.m) / (ctx->geometry.np + ctx->geometry.mp);
  if (iseg != 1)
  {
    /*** may be error condition?? ***/
    if (ctx->geometry.ipsym == 0)
    {
      add_error(ctx, &ctx->errors, "ERROR: IPSYM=0 IN CONECT()", FATAL);
      return -1;
    }

    if (ctx->geometry.ipsym < 0)
      fprintf(ctx->output_fp,
              "\n  STRUCTURE HAS %d FOLD ROTATIONAL SYMMETRY\n", iseg);
    else
    {
      int ic = iseg / 2;
      if (iseg == 8)
        ic = 3;
      fprintf(ctx->output_fp,
              "\n  STRUCTURE HAS %d PLANES OF SYMMETRY\n", ic);
    } /* if(ctx->geometry.ipsym < 0 ) */
  } /* if( iseg != 1) */

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
static int write_segments(nec_context_t *ctx, const deck_t *deck, FILE *file)
{
  // exit now if there's no segments
  if (ctx->geometry.n == 0)
    return 0;

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

  double xw1, yw1, zw1;
  double xw2, yw2;

  // Calculate frequency ratio to unscale geometry back to meters
  // The geometry has been scaled by fr = fmhz / CVEL during frequency loop
  // We need to divide by fr to get back to the original meter values
  double fr = ctx->save.fmhz / CVEL;

  for (int i = 0; i < ctx->geometry.n; i++)
  {
    xw1 = ctx->geometry.x2[i] - ctx->geometry.x1[i];
    yw1 = ctx->geometry.y2[i] - ctx->geometry.y1[i];
    zw1 = ctx->geometry.z2[i] - ctx->geometry.z1[i];
    ctx->geometry.x[i] = (ctx->geometry.x1[i] + ctx->geometry.x2[i]) / 2.0;
    ctx->geometry.y[i] = (ctx->geometry.y1[i] + ctx->geometry.y2[i]) / 2.0;
    ctx->geometry.z[i] = (ctx->geometry.z1[i] + ctx->geometry.z2[i]) / 2.0;
    xw2 = xw1 * xw1 + yw1 * yw1 + zw1 * zw1;
    yw2 = sqrt(xw2);
    yw2 = (xw2 / yw2 + yw2) * .5;
    ctx->geometry.si[i] = yw2;
    ctx->geometry.cab[i] = xw1 / yw2;
    ctx->geometry.sab[i] = yw1 / yw2;
    xw2 = zw1 / yw2;

    if (xw2 > 1.0)
      xw2 = 1.0;
    if (xw2 < -1.0)
      xw2 = -1.0;

    ctx->geometry.salp[i] = xw2;
    xw2 = asin(xw2) * TD;
    yw2 = atan2(yw1, xw1) * TD;

    fprintf(ctx->output_fp, "\n"
                            " %5d %9.4f %9.4f %9.4f %9.4f"
                            " %9.4f %9.4f %9.4f %5d %5d %5d %5d",
            i + 1, ctx->geometry.x[i], ctx->geometry.y[i], ctx->geometry.z[i], ctx->geometry.si[i], xw2, yw2,
            ctx->geometry.bi[i] / fr, ctx->geometry.icon1[i], i + 1, ctx->geometry.icon2[i], ctx->geometry.tag_nums[i]);

    if (ctx->plot.iplp1 == 1)
      fprintf(ctx->plot_fp, "%12.4E %12.4E %12.4E "
                            "%12.4E %12.4E %12.4E %12.4E %5d %5d %5d\n",
              ctx->geometry.x[i], ctx->geometry.y[i], ctx->geometry.z[i], ctx->geometry.si[i], xw2, yw2,
              ctx->geometry.bi[i], ctx->geometry.icon1[i], i + 1, ctx->geometry.icon2[i]);

    if ((ctx->geometry.si[i] <= 1.e-20) || (ctx->geometry.bi[i] <= 0.0))
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
static void write_patches(const nec_context_t *ctx, const deck_t *deck, FILE *file)
{
  // exit now if there's no patches
  if (ctx->geometry.m == 0)
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
  for (int i = 0; i < ctx->geometry.m; i++)
  {
    xw1 = (ctx->geometry.t1y[i] * ctx->geometry.t2z[i] - ctx->geometry.t1z[i] * ctx->geometry.t2y[i]) * ctx->geometry.psalp[i];
    yw1 = (ctx->geometry.t1z[i] * ctx->geometry.t2x[i] - ctx->geometry.t1x[i] * ctx->geometry.t2z[i]) * ctx->geometry.psalp[i];
    zw1 = (ctx->geometry.t1x[i] * ctx->geometry.t2y[i] - ctx->geometry.t1y[i] * ctx->geometry.t2x[i]) * ctx->geometry.psalp[i];

    fprintf(ctx->output_fp, "\n"
                            " %4d %10.5f %10.5f %10.5f  %8.4f %8.4f %8.4f"
                            " %10.5f  %8.4f %8.4f %8.4f  %8.4f %8.4f %8.4f",
            i + 1, ctx->geometry.px[i], ctx->geometry.py[i], ctx->geometry.pz[i], xw1, yw1, zw1, ctx->geometry.pbi[i],
            ctx->geometry.t1x[i], ctx->geometry.t1y[i], ctx->geometry.t1z[i], ctx->geometry.t2x[i], ctx->geometry.t2y[i], ctx->geometry.t2z[i]);
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
static void write_input_cards(FILE *file, const deck_t *deck, int batch_start, int batch_end, int card_number_offset)
{
  if (file == NULL || deck == NULL)
  {
    return;
  }

  fprintf(file, "\n\n\n");

  /* Iterate through cards in this batch only. */
  int card_number = card_number_offset;
  for (int i = batch_start; i <= batch_end && i < deck->num_cards; i++)
  {
    card_t *card = &deck->cards[i];

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
      continue; // Continue to include XT in batch, don't break
    }

    /* Only echo control cards (skip geometry and comment cards) */
    if (strncmp(card->card_code, "EN", 2) == 0 ||
        strncmp(card->card_code, "FR", 2) == 0 ||
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
        strncmp(card->card_code, "NX", 2) == 0 ||
        strncmp(card->card_code, "PT", 2) == 0 ||
        strncmp(card->card_code, "PQ", 2) == 0 ||
        strncmp(card->card_code, "CP", 2) == 0 ||
        strncmp(card->card_code, "GD", 2) == 0 ||
        strncmp(card->card_code, "WG", 2) == 0 ||
        strncmp(card->card_code, "XQ", 2) == 0)
    {

      card_number++;

      /* Output in exact NEC format: card number, card code, 4 ints, 7 floats */
      fprintf(file, "  DATA CARD No: %3d %s", card_number, card->card_code);

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

    // print CM and CE comments inline as they are encountered (NEC-4 behavior)
    // TODO: test to see if this is correct placement for NEC-4, or if it has to be
    //       output after the *output* for the control cards, or as it is here, during
    //       the input card echoing.
    // TODO: make this configurable behavior?
    else if (strncmp(card->card_code, "CM", 2) == 0 || strncmp(card->card_code, "CE", 2) == 0)
    {
      if (card->comment)
      {
        fprintf(file, "                              %s\n", card->comment);
      }
    }
  }

  fprintf(file, "\n");
}

/******************************************************************************
 * write_frequency_data
 *
 * Writes the frequency in MHz and wavelength in meters, plus integration
 * method information. This matches the NEC2 output format.
 */
static void write_frequency_data(FILE *file, const nec_context_t *ctx)
{
  fprintf(file, "\n\n"
                "                               "
                "--------- FREQUENCY --------\n"
                "                                "
                "FREQUENCY :%11.4E MHz\n"
                "                                "
                "WAVELENGTH:%11.4E Mtr",
          ctx->save.fmhz,
          ctx->geometry.wlam);

  fprintf(file, "\n\n"
                "                        "
                "APPROXIMATE INTEGRATION EMPLOYED FOR SEGMENTS \n"
                "                        "
                "THAT ARE MORE THAN %.3f WAVELENGTHS APART",
          ctx->dataj.rkh);

  if (ctx->dataj.iexk == 1)
  {
    fprintf(file, "\n"
                  "                        "
                  "THE EXTENDED THIN WIRE KERNEL WILL BE USED");
  }
}

/******************************************************************************
 * write_loading_data
 *
 * Writes the structure impedance loading section header.
 * The actual loading details are printed by load() in calculations.c
 * as it processes the loading cards.
 */
static void write_loading_data(FILE *file, const nec_context_t *ctx)
{
  fprintf(file, "\n\n\n"
                "                          "
                "------ STRUCTURE IMPEDANCE LOADING ------");

  if (ctx->zload.nload == 0)
  {
    fprintf(file, "\n"
                  "                                 "
                  "THIS STRUCTURE IS NOT LOADED");
    return;
  }

  // Print the loading data header (from load() function)
  fprintf(file, "\n"
                "  LOCATION        RESISTANCE  INDUCTANCE  CAPACITANCE   "
                "  IMPEDANCE (OHMS)   CONDUCTIVITY  CIRCUIT\n"
                "  ITAG FROM THRU     OHMS       HENRYS      FARADS     "
                "  REAL     IMAGINARY   MHOS/METER      TYPE");

  // Print the stored loading entries
  for (int i = 0; i < ctx->loading_outputs.count; i++)
  {
    loading_output_t *entry = &ctx->loading_outputs.entries[i];
    if (strcmp(entry->type, "WIRE") == 0)
    {
      // Special format for WIRE entries to match prnt output exactly
      fprintf(file, "\n%5d%72s%11.4E     WIRE  ",
              entry->tag, "", entry->conductivity);
    }
    else
    {
      // General format for other loading types
      fprintf(file, "\n%6d%6d%6d%44.4E%12s",
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
static void write_environment_data(FILE *file, const nec_context_t *ctx)
{
  fprintf(file, "\n\n\n"
                "                            "
                "-------- ANTENNA ENVIRONMENT --------");

  if (ctx->gnd.ksymp == 1)
  {
    fprintf(file, "\n"
                  "                            "
                  "FREE SPACE");
  }
  else
  {
    if (ctx->gnd.iperf == 1)
    {
      fprintf(file, "\n"
                    "                            "
                    "PERFECT GROUND");
    }
    else
    {
      // Radial wire ground screen
      if (ctx->gnd.nradl != 0)
      {
        fprintf(file, "\n"
                      "                            "
                      "RADIAL WIRE GROUND SCREEN\n"
                      "                            "
                      "%d WIRES\n"
                      "                            "
                      "WIRE LENGTH: %8.2f METERS\n"
                      "                            "
                      "WIRE RADIUS: %10.3E METERS",
                ctx->gnd.nradl, ctx->save.scrwlt, ctx->save.scrwrt);

        fprintf(file, "\n"
                      "                            "
                      "MEDIUM UNDER SCREEN -");
      }

      // Ground type
      if (ctx->gnd.iperf != 2)
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
      complex double epsc = cmplx(ctx->save.epsr, -ctx->save.sig * ctx->geometry.wlam * 59.96);
      fprintf(file, "\n"
                    "                            "
                    "RELATIVE DIELECTRIC CONST: %.3f\n"
                    "                            "
                    "CONDUCTIVITY: %10.3E MHOS/METER\n"
                    "                            "
                    "COMPLEX DIELECTRIC CONSTANT: %11.4E%+11.4Ej",
              ctx->save.epsr, ctx->save.sig, creal(epsc), cimag(epsc));
    }
  }
}

/******************************************************************************
 * write_matrix_timing
 *
 * Writes the matrix fill and factor timing information.
 */
static void write_matrix_timing(FILE *file, const nec_context_t *ctx)
{
  fprintf(file, "\n\n\n"
                "                             "
                "---------- MATRIX TIMING ----------\n"
                "                               "
                "FILL: %d msec  FACTOR: %d msec",
          (int)(ctx->mat_fill_time * 1000.0),
          (int)(ctx->mat_factor_time * 1000.0));
}

/******************************************************************************
 * write_network_data
 *
 * Writes the network data section showing transmission lines and network
 * connections between segments.
 */
static void write_network_data(FILE *file, const nec_context_t *ctx)
{
  if (ctx->netcx.nonet == 0)
  {
    return; // No network data to write
  }

  fprintf(file, "\n\n\n"
                "                                            "
                "---------- NETWORK DATA ----------");

  int itmp1 = ctx->netcx.ntyp[0];
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

    for (int j = 0; j < ctx->netcx.nonet; j++)
    {
      int itmp2 = ctx->netcx.ntyp[j];

      if ((itmp2 / itmp1) != 1)
      {
        itmp3 = itmp2;
      }
      else
      {
        int itmp4 = ctx->netcx.iseg1[j];
        int itmp5 = ctx->netcx.iseg2[j];
        int idx4 = itmp4 - 1;
        int idx5 = itmp5 - 1;

        if ((itmp2 >= 2) && (ctx->netcx.x11i[j] <= 0.0))
        {
          double xx = ctx->geometry.x[idx5] - ctx->geometry.x[idx4];
          double yy = ctx->geometry.y[idx5] - ctx->geometry.y[idx4];
          double zz = ctx->geometry.z[idx5] - ctx->geometry.z[idx4];
          ctx->netcx.x11i[j] = ctx->geometry.wlam * sqrt(xx * xx + yy * yy + zz * zz);
        }

        fprintf(file, "\n"
                      " %4d %5d %4d %5d  %11.4E %11.4E  "
                      "%11.4E %11.4E  %11.4E %11.4E  %s",
                ctx->geometry.tag_nums[idx4], itmp4,
                ctx->geometry.tag_nums[idx5], itmp5,
                ctx->netcx.x11r[j], ctx->netcx.x11i[j],
                ctx->netcx.x12r[j], ctx->netcx.x12i[j],
                ctx->netcx.x22r[j], ctx->netcx.x22i[j],
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
static void write_matrix_asymmetry(FILE *file, const nec_context_t *ctx)
{
  // Only write if asymmetry check was performed and data exists
  if (ctx->netcx.masym == 0 || ctx->netcx.asmx == 0.0)
  {
    return;
  }

  fprintf(file, "\n\n"
                "   MAXIMUM RELATIVE ASYMMETRY OF THE DRIVING POINT ADMITTANCE\n"
                "   MATRIX IS %10.3E FOR SEGMENTS %d AND %d\n"
                "   RMS RELATIVE ASYMMETRY IS %10.3E",
          ctx->netcx.asmx, ctx->netcx.nteq_asym, ctx->netcx.ntsc_asym, ctx->netcx.asa);
}

/******************************************************************************
 * write_network_excitation
 *
 * Writes structure excitation data at network connection points, including
 * voltage, current, impedance, admittance, and power for each connection.
 */
static void write_network_excitation(FILE *file, const nec_context_t *ctx)
{
  if (ctx->netcx.nexc == 0 || ctx->netcx.nprint != 0)
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
static void write_antenna_input_parameters(FILE *file, const nec_context_t *ctx)
{
  if (ctx->netcx.ninp == 0)
  {
    return; // No input data to write
  }

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

/******************************************************************************
 * write_currents
 *
 * Writes current distribution for all segments, including coordinates,
 * segment length, and current magnitude and phase.
 */
static void write_currents(FILE *file, const nec_context_t *ctx)
{
  if (ctx->geometry.n == 0)
  {
    return; // No segments to write
  }

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

  // Calculate frequency ratio to convert meters to wavelengths
  // The geometry arrays have been recalculated in meters by write_segments
  // We need to multiply by fr (= 1/wlam) to convert to wavelengths
  double fr = ctx->save.fmhz / CVEL;

  for (int i = 0; i < ctx->geometry.n; i++)
  {
    complex double curi = ctx->crnt.cur[i] * ctx->geometry.wlam;
    double cmag = cabs(curi);
    double ph = carg(curi) * TD; // Convert to degrees (TD = 57.29577951)

    fprintf(file, "\n"
                  " %5d %4d %9.4f %9.4f %9.4f %8.5f %11.4E %11.4E %11.4E %9.3f",
            i + 1, ctx->geometry.tag_nums[i],
            ctx->geometry.x[i] * fr,
            ctx->geometry.y[i] * fr,
            ctx->geometry.z[i] * fr,
            ctx->geometry.si[i] * fr,
            creal(curi), cimag(curi), cmag, ph);
  }
}

/******************************************************************************
 * write_power_budget
 *
 * Writes the power budget showing input power, radiated power, structure
 * loss, network loss, and efficiency.
 */
static void write_power_budget(FILE *file, const nec_context_t *ctx)
{
  // Only write for standard radiation pattern types
  if ((ctx->fpat.ixtyp != 0) && (ctx->fpat.ixtyp != 5))
  {
    return;
  }

  double tmp1 = ctx->netcx.pin - ctx->netcx.pnls - ctx->fpat.ploss;
  double tmp2 = 100.0 * tmp1 / ctx->netcx.pin;

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
          ctx->netcx.pin, tmp1, ctx->fpat.ploss, ctx->netcx.pnls, tmp2);
}

/******************************************************************************
 * write_radiation_pattern_header
 *
 * Writes the radiation pattern section header and column headers.
 */
static void write_radiation_pattern_header(FILE *file, const nec_context_t *ctx)
{
  char *igtp[2] = {"----- POWER GAINS ----- ", "--- DIRECTIVE GAINS ---"};
  char *igax[4] = {" MAJOR", " MINOR", " VERTC", " HORIZ"};

  // Check if radiation pattern was calculated
  if (ctx->rpat.num_points == 0 || ctx->rpat.points == NULL)
  {
    return;
  }

  /* Write ground parameters if applicable */
  if (ctx->gnd.ifar > 1)
  {
    fprintf(file, "\n\n\n"
                  "                               "
                  "------ FAR FIELD GROUND PARAMETERS ------\n\n");

    if (ctx->gnd.ifar > 3)
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
              ctx->gnd.nradl, ctx->save.scrwlt, ctx->save.scrwrt);
    }

    if (ctx->gnd.ifar != 4 && strlen(ctx->rpat.ground_cliff_type) > 0)
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
              ctx->rpat.ground_cliff_type, ctx->fpat.clt, ctx->fpat.cht,
              ctx->fpat.epsr2, ctx->fpat.sig2);
    }
  }

  /* Write main header */
  if (ctx->gnd.ifar == 1)
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
    int itmp1 = 2 * ctx->fpat.iax;
    int itmp2 = itmp1 + 1;

    fprintf(file, "\n\n\n"
                  "                             "
                  "---------- RADIATION PATTERNS -----------\n");

    if (ctx->fpat.rfld >= 1.0e-20)
    {
      fprintf(file, "\n"
                    "                             "
                    "RANGE: %13.6E METERS\n"
                    "                             "
                    "EXP(-JKR)/R: %12.5E AT PHASE: %7.2f DEGREES\n",
              ctx->fpat.rfld, ctx->rpat.exrm, ctx->rpat.exra);
    }

    fprintf(file, "\n"
                  " ---- ANGLES -----     %23s      ---- POLARIZATION ----  "
                  " ---- E(THETA) ----    ----- E(PHI) ------\n"
                  "  THETA      PHI      %6s   %6s    TOTAL       AXIAL    "
                  "  TILT  SENSE   MAGNITUDE    PHASE    MAGNITUDE     PHASE\n"
                  " DEGREES   DEGREES        DB       DB       DB       RATIO  "
                  " DEGREES            VOLTS/M   DEGREES     VOLTS/M   DEGREES",
            igtp[ctx->fpat.ipd], igax[itmp1], igax[itmp2]);
  }
}

/******************************************************************************
 * write_radiation_pattern_data
 *
 * Writes the computed radiation pattern data for each theta/phi point.
 * Data includes gains, polarization, and E-field components.
 */
static void write_radiation_pattern_data(FILE *file, const nec_context_t *ctx)
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

    if (ctx->gnd.ifar == 1)
    {
      /* Near field output */
      fprintf(file, "\n"
                    " %9.2f %7.2f %9.2f  %11.4E %7.2f  %11.4E %7.2f  %11.4E %7.2f",
              ctx->fpat.rfld, pt->phi, pt->theta,
              pt->ethm, pt->etha, pt->ephm, pt->epha, pt->erdm, pt->erda);
    }
    else
    {
      /* Far field output */
      if (ctx->fpat.iax != 1)
      {
        tmp5 = pt->gnmj;
        tmp6 = pt->gnmn;
      }
      else
      {
        tmp5 = pt->gnv;
        tmp6 = pt->gnh;
      }

      fprintf(file, "\n"
                    " %7.2f %9.2f  %8.2f %8.2f %8.2f %11.4f"
                    " %9.2f %6s %11.4E %9.2f %11.4E %9.2f",
              pt->theta, pt->phi, tmp5, tmp6, pt->gtot, pt->axrat,
              pt->tilta, hpol[pt->pol_sense >= 0 && pt->pol_sense <= 2 ? pt->pol_sense : 0],
              pt->ethm, pt->etha, pt->ephm, pt->epha);
    }
  }
}

/******************************************************************************
 * write_average_power_gain
 *
 * Writes the average power gain over the specified solid angle.
 */
static void write_average_power_gain(FILE *file, const nec_context_t *ctx)
{
  if (ctx->fpat.iavp == 0)
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
static void write_normalized_gain(FILE *file, const nec_context_t *ctx)
{
  char *igntp[5] = {" MAJOR AXIS", "  MINOR AXIS",
                    "    VERTICAL", "  HORIZONTAL", "       TOTAL "};

  if (ctx->fpat.inor == 0 || ctx->rpat.num_points == 0)
  {
    return;
  }

  int itmp1 = ctx->fpat.inor - 1;

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

    switch (ctx->fpat.inor)
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
        switch (ctx->fpat.inor)
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

      switch (ctx->fpat.inor)
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
 * write_footer
 *
 * Writes the footer with total run time.
 */
static void write_footer(FILE *file, const nec_context_t *ctx, const deck_t *deck)
{
  (void)deck; // unused — EN is echoed by write_input_cards in sequence

  // Output blank lines before footer
  fprintf(file, "\n\n\n");

  // Calculate and output total runtime
  if (ctx != NULL)
  {
    double current_time;
    nec_get_time_ms(ctx, &current_time);
    double elapsed_ms = current_time - ctx->start_time;
    fprintf(file, "\n  TOTAL RUN TIME: %.0f msec", elapsed_ms);
  }
}

/* end of output.c */
