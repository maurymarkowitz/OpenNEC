/******************************************************************************
 * report_output.c
 *
 * report_output.c contains various work methods that print bits and pieces
 * of the report structure.
 *
 *****************************************************************************/

#include "internals.h"
#include "output.h"
#include "report_output.h"
#include <stdint.h>
#include <math.h>

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
        .matrix_fill_format = "FILL=%9.3f SEC.,",
        .matrix_factor_format = "FACTOR=%9.3f SEC.",
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


/**
 * Write input cards echo to output file [DEPRECATED - BATCH MODE ONLY]
 * Echoes FR, TL, LD, EX, RP, and other control cards from the current batch.
 * The batch is defined by batch_start and batch_end (inclusive).
 * If batch_end points to EN or XT, that card is included as the final card.
 *
 * DEPRECATED: This function is part of the legacy batch processing system.
 * It is only called from control.c (run_simulation). The active sequential
 * processing pathway in reporting.c does not use this function.
 *
 * @param file Output file pointer
 * @param deck The deck containing all cards
 * @param batch_start First card index of this batch (inclusive)
 * @param batch_end Last card index of this batch (inclusive)
 * @param card_number_offset Starting card number for this batch
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
  write_comments(ctx, deck, file);
  write_structure(ctx, deck, file);
  write_segments(ctx, deck, file);
}

/******************************************************************************
 * Writes the header area to the standard NEC output file.
 *
 * these are lines 231 through 242 in the original nec2c code
 */
void write_header(const context_t *ctx, const deck_t *deck, FILE *file)
{
  // sanity, don't actually need the deck here, but for consistancy...
  if (!ctx || !deck || !file) return;

  if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL)
  {
    fprintf(file, "\n\n\n"
                  "                                 *********************************************\n"
                  "\n"
                  "                                  NUMERICAL ELECTROMAGNETICS CODE (onec %s)\n"
                  "\n"
                  "                                 *********************************************\n", VERSION_STRING);
  }
  else
  {
    fprintf(file, "\n\n\n"
                  "                            "
                  " ______________________________________________\n"
                  "                            "
                  "|                                              |\n"
                  "                            "
                  "| NUMERICAL ELECTROMAGNETICS CODE (onec %s) |\n"
                  "                            "
                  "|     Translated to 'C' (double precision)     |\n"
                  "                            "
                  "|______________________________________________|\n", VERSION_STRING);
  }
}

/******************************************************************************
 * Writes the comment cards section
 * 
 * Writes only the CM and CE cards at the top of the deck, comments found
 * deeper in the deck are not written to the output file.
 * 
 * In nec2c, the section is only written if there are comment cards in the deck,
 * but that is not the case in fortran, where the header is written every time
 *
 */
void write_comments(const context_t *ctx, const deck_t *deck, FILE *file)
{
  // sanity
  if (!ctx || !deck || !file) return;

  // do we have any comments?
  int cstart = (deck->comment_start >= 0) ? deck->comment_start : 0;
  int cend = (deck->comment_end >= 0) ? deck->comment_end : deck->geometry_start - 1;

  if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL)
  {
    // always print the header in original format, even if there are no comments
    fprintf(file, "\n\n\n\n"
                  "                                     "
                  "- - - - COMMENTS - - - -\n\n\n");
  }
  else
  {
    // only print the header if there are comments in the deck
    if (cstart <= cend)
    {
      fprintf(file, "\n\n\n"
                    "                               "
                    "---------------- COMMENTS ----------------\n");
    }
  }

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
    /* Original Fortran format: blank line padded to 101 chars, then blank lines */
    fprintf(file, "%-101s\n\n\n", "");
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
int write_structure(context_t *ctx, const deck_t *deck, FILE *file)
{
  card_t card;
  int geo_card_num;
  int num_wires = 0;
  int num_patches = 0;
  bool last_patch_pending_sc = false;
  double last_patch_xw1 = 0.0, last_patch_yw1 = 0.0, last_patch_zw1 = 0.0;
  double last_patch_xw2 = 0.0, last_patch_yw2 = 0.0, last_patch_zw2 = 0.0;
  int last_patch_segs = 0;
  int last_patch_tag = 0;

  int ix, iy, iz;

  // these are used to match various codes in the cards to text output
  char ifx[2] = {'*', 'X'}, ify[2] = {'*', 'Y'}, ifz[2] = {'*', 'Z'}; // reflection axes
  char ipt[4] = { 'P', 'R', 'T', 'Q' };

  // print the header
  if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL)
  {
    fprintf(file, "\n\n"
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
      last_patch_pending_sc = false;
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
      last_patch_pending_sc = false;
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
      last_patch_pending_sc = false;
      fprintf(ctx->output_fp,
              "\n     STRUCTURE SCALED BY FACTOR: %10.5f", card.f[1]);
      break;

    case 4: // GE card, nothing to do
      last_patch_pending_sc = false;
      break;

    case 5: // GM card, move/copy existing structure
      last_patch_pending_sc = false;
      fprintf(ctx->output_fp,
              "\n     THE STRUCTURE HAS BEEN MOVED, MOVE DATA CARD IS:\n"
              "   %3d %5d %10.5f %10.5f %10.5f %10.5f %10.5f %10.5f %10.5f",
              card.i[1], card.i[2], card.f[1], card.f[2], card.f[3], card.f[4], card.f[5], card.f[6], card.f[7]);
      break;

    case 6: // SP card, generate single surface patch
      num_patches++;
      last_patch_pending_sc = true;
      last_patch_xw1 = card.f[1];
      last_patch_yw1 = card.f[2];
      last_patch_zw1 = card.f[3];
      last_patch_xw2 = card.f[4];
      last_patch_yw2 = card.f[5];
      last_patch_zw2 = card.f[6];
      last_patch_segs = card.i[2];
      last_patch_tag = card.i[1];
      fprintf(ctx->output_fp, "\n"
                              " %5d%c%10.5f%10.5f%10.5f%10.5f%10.5f%10.5f",
              num_patches, ipt[card.i[2]], card.f[1], card.f[2], card.f[3], card.f[4], card.f[5], card.f[6]);
      break;

    case 7: // SM card, multiple-patch surface
      {
        int group_size = card.i[1] * card.i[2];
        int patch_number = num_patches + 1;
        num_patches += group_size;
        last_patch_pending_sc = true;
        last_patch_xw1 = card.f[1];
        last_patch_yw1 = card.f[2];
        last_patch_zw1 = card.f[3];
        last_patch_xw2 = card.f[4];
        last_patch_yw2 = card.f[5];
        last_patch_zw2 = card.f[6];
        last_patch_segs = card.i[2];
        last_patch_tag = card.i[1];
        fprintf(ctx->output_fp, "\n"
                                " %5d%c%10.5f%11.5f%11.5f %11.5f%11.5f%11.5f"
                                "     SURFACE - %3d BY%3d PATCHES",
                patch_number, ipt[1], card.f[1], card.f[2], card.f[3], card.f[4], card.f[5], card.f[6],
                card.i[1], card.i[2]);
      }
      break;

    case 8: // GA card, arc
      last_patch_pending_sc = false;
      num_wires++;
      fprintf(ctx->output_fp, "\n"
                              " %5d ARC RADIUS: %9.5f  FROM: %8.3f TO: %8.3f DEGREES"
                              "       %11.5f %5d %5d %5d %4d",
              num_wires, card.f[1], card.f[2], card.f[3], card.f[4],
              card.tag, card.start_segment, card.end_segment, card.i[1]);

      // FIXME: this looks wrong, last input
      break;

    case 9: // SC card, surface patch continuation
      if (last_patch_pending_sc) {
        double x3 = card.f[1];
        double y3 = card.f[2];
        double z3 = card.f[3];
        double x4 = 0.0;
        double y4 = 0.0;
        double z4 = 0.0;
        if (card.flts_used >= 6) {
          x4 = card.f[4];
          y4 = card.f[5];
          z4 = card.f[6];
        } else if (last_patch_segs == 2 || last_patch_tag > 0) {
          x4 = last_patch_xw1 + x3 - last_patch_xw2;
          y4 = last_patch_yw1 + y3 - last_patch_yw2;
          z4 = last_patch_zw1 + z3 - last_patch_zw2;
        }
        fprintf(ctx->output_fp, "\n      %11.5f%11.5f%11.5f %11.5f%11.5f%11.5f",
                x3, y3, z3, x4, y4, z4);
      }
      break;

    case 10: // GH card, generate helix
      last_patch_pending_sc = false;
      num_wires++;
      if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL)
      {
        fprintf(ctx->output_fp, "\n"
                                " %5d HELIX STRUCTURE-   AXIAL SPACING BETWEEN TURNS = %8.3f"
                                " TOTAL AXIAL LENGTH = %8.3f  %8.3f %5d %5d %5d %4d\n      "
                                " RADIUS OF HELIX =%8.3f X1:%8.3f Y1:%8.3f X2:%8.3f ",
                num_wires, card.f[1], card.f[2], card.f[7], card.tag, card.start_segment, card.end_segment,
                card.i[1], card.f[3], card.f[4], card.f[5], card.f[6]);
      }
      else
      {
        fprintf(ctx->output_fp, "\n"
                                " %5d HELIX STRUCTURE - SPACING OF TURNS: %8.3f AXIAL"
                                " LENGTH: %8.3f  %8.3f %5d %5d %5d %4d\n      "
                                " RADIUS X1:%8.3f Y1:%8.3f X2:%8.3f Y2:%8.3f ",
                num_wires, card.f[1], card.f[2], card.f[7], card.tag, card.start_segment, card.end_segment,
                card.i[1], card.f[3], card.f[4], card.f[5], card.f[6]);
      }
      break;

    } /* switch on the card type */
  } /* for loop over cards */

  // and now a final report on the cards
  if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL)
  {
    for (int i = 0; i < ctx->outputs.num_messages; i++)
    {
      fprintf(ctx->output_fp, "%s", ctx->outputs.messages[i]);
      size_t msg_len = strlen(ctx->outputs.messages[i]);
      if (msg_len == 0 || ctx->outputs.messages[i][msg_len - 1] != '\n')
        fprintf(ctx->output_fp, "\n");
    }

    fprintf(ctx->output_fp, "\n\n"
                            "   TOTAL SEGMENTS USED=%5d     NO. SEG. IN A SYMMETRIC CELL=%5d     SYMMETRY FLAG=%3d",
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
  {
    if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL)
    {
      fprintf(ctx->output_fp, "\n"
                              "   TOTAL PATCHES USED=   %d      NO. PATCHES IN A SYMMET"
                              "RIC CELL=   %d",
              ctx->geometry.num_patches, ctx->geometry.num_patches_sym);
    }
    else
    {
      fprintf(ctx->output_fp, "\n"
                              "     TOTAL PATCHES USED: %d   PATCHES"
                              " IN A SYMMETRIC CELL: %d",
              ctx->geometry.num_patches, ctx->geometry.num_patches_sym);
    }
  }

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
  if (ctx->output_format != OUTPUT_FORMAT_ORIGINAL)
  {
    for (int i = 0; i < ctx->outputs.num_messages; i++)
    {
      fprintf(ctx->output_fp, "%s", ctx->outputs.messages[i]);
      size_t msg_len = strlen(ctx->outputs.messages[i]);
      if (msg_len == 0 || ctx->outputs.messages[i][msg_len - 1] != '\n')
        fprintf(ctx->output_fp, "\n");
    }
  }

  return 0;
} /* write_structure() */

/******************************************************************************
 * write_segments()
 *
 * Writes the segment data section of the nec2 output.
 *
 */
int write_segments(context_t *ctx, const deck_t *deck, FILE *file)
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
  
  /* Print patch data as final part of geometry output block (matching Fortran flow) */
  write_patches(ctx, deck, file);
  
  return 0;
} /* write_segments */

/******************************************************************************
 * write_patches()
 *
 * writes the patch data section of the nec2 output.
 *
 */
void write_patches(const context_t *ctx, const deck_t *deck, FILE *file)
{
  // exit now if there's no patches
  if (ctx->geometry.num_patches == 0)
    return;

  if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL)
  {
    fprintf(ctx->output_fp, "\n\n\n\n"
                            "                                            "
                            "- - - SURFACE PATCH DATA - - -\n"
                            "\n"
                            "                                                 "
                            "COORDINATES IN METERS\n\n"
                            " PATCH     COORD. OF PATCH CENTER       UNIT NORMAL VECTOR      "
                            "PATCH            COMPONENTS OF UNIT TANGENT VECTORS\n"
                            "  NO.      X         Y         Z         X       Y       Z       "
                            "AREA       X1      Y1      Z1       X2      Y2      Z2");
  }
  else
  {
    fprintf(ctx->output_fp, "\n\n\n"
                            "                                   "
                            " --------- SURFACE PATCH DATA ---------\n"
                            "                                            "
                            " COORDINATES IN METERS\n\n"
                            " PATCH     COORD. OF PATCH CENTER           UNIT NORMAL VECTOR      "
                            " PATCH           COMPONENTS OF UNIT TANGENT VECTORS\n"
                            "  No:      X          Y          Z          X        Y        Z      "
                            " AREA         X1       Y1       Z1        X2       Y2      Z2");
  }

  double xw1, yw1, zw1;
  for (int i = 0; i < ctx->geometry.num_patches; i++)
  {
    xw1 = (ctx->geometry.patch_t1y[i] * ctx->geometry.patch_t2z[i] - ctx->geometry.patch_t1z[i] * ctx->geometry.patch_t2y[i]) * ctx->geometry.patch_normal_z[i];
    yw1 = (ctx->geometry.patch_t1z[i] * ctx->geometry.patch_t2x[i] - ctx->geometry.patch_t1x[i] * ctx->geometry.patch_t2z[i]) * ctx->geometry.patch_normal_z[i];
    zw1 = (ctx->geometry.patch_t1x[i] * ctx->geometry.patch_t2y[i] - ctx->geometry.patch_t1y[i] * ctx->geometry.patch_t2x[i]) * ctx->geometry.patch_normal_z[i];

    fprintf(ctx->output_fp, "\n"
                            " %4d%10.5f%10.5f%10.5f%9.4f%8.4f%8.4f"
                            "%10.5f%9.4f%8.4f%8.4f%9.4f%8.4f%8.4f",
            i + 1, ctx->geometry.patch_x_center[i], ctx->geometry.patch_y_center[i], ctx->geometry.patch_z_center[i], xw1, yw1, zw1, ctx->geometry.patch_area[i],
            ctx->geometry.patch_t1x[i], ctx->geometry.patch_t1y[i], ctx->geometry.patch_t1z[i], ctx->geometry.patch_t2x[i], ctx->geometry.patch_t2y[i], ctx->geometry.patch_t2z[i]);
  } /* for( i = 0; i < data.m; i++ ) */
  fprintf(ctx->output_fp, "\n");
}

/****************************************************************************
 * write_input_cards_excluding_end() [DEPRECATED - BATCH MODE ONLY]
 *
 * Like write_input_cards but skips EN and NX cards (which are output separately).
 *
 * DEPRECATED: This function is part of the legacy batch processing system.
 * It is only called from write_batch_card_echo and related batch functions.
 * The active sequential processing pathway in reporting.c does not use this.
 */
static void write_input_cards_excluding_end(FILE *file, const context_t *ctx, const deck_t *deck, int batch_start, int batch_end, int card_number_offset)
{
  if (file == NULL || ctx == NULL || deck == NULL)
  {
    return;
  }

  /* First batch (after structure spec) needs more spacing than subsequent batches */
  if (!ctx->batch_cards_echoed)
  {
    fprintf(file, "\n\n\n\n\n\n\n");  // 7 newlines for first batch
  }
  else
  {
    fprintf(file, "\n\n\n\n");  // 4 newlines for subsequent batches
  }

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
 * count_echoed_cards_in_range()
 *
 * Returns the count of control cards that write_input_cards_excluding_end()
 * would echo in the index range [from, to].  Used to compute the correct
 * sequential card number offset for subsequent batches.
 */
static int count_echoed_cards_in_range(const deck_t *deck, int from, int to)
{
  int count = 0;
  for (int i = from; i <= to && i < deck->num_cards; i++)
  {
    card_t *card = &deck->cards[i];
    if (strncmp(card->card_code, "EN", 2) == 0 || strncmp(card->card_code, "NX", 2) == 0)
      continue;
    if (strncmp(card->card_code, "XT", 2) == 0) { count++; continue; }
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
      count++;
  }
  return count;
}

/******************************************************************************
 * write_batch_card_echo() [DEPRECATED - BATCH MODE ONLY]
 *
 * Echoes the control cards for the current batch (ctx->batch_start_card to
 * ctx->batch_end_card) with correct sequential numbering, accounting for all
 * previously echoed batches.  Called before each non-first batch's frequency
 * output to mirror Fortran behavior: each FR/RP pair is echoed immediately
 * before its own computation output rather than all cards being dumped at
 * the top of the file.
 *
 * DEPRECATED: This function is part of the legacy batch processing system.
 * It is only called from control.c (run_simulation). The active sequential
 * processing pathway in reporting.c does not use this function.
 */
void write_batch_card_echo(FILE *file, const context_t *ctx, const deck_t *deck)
{
  if (file == NULL || ctx == NULL || deck == NULL)
    return;

  /* Number cards in this batch sequentially after all previous batches. */
  int offset = count_echoed_cards_in_range(deck, deck->geometry_end + 1,
                                            ctx->batch_start_card - 1);
  
  /* Find the first XQ in the batch to echo only up to there */
  int first_xq_pos = -1;
  for (int i = ctx->batch_start_card; i <= ctx->batch_end_card && i < deck->num_cards; i++) {
    if (strncmp(deck->cards[i].card_code, "XQ", 2) == 0) {
      first_xq_pos = i;
      break;
    }
  }
  
  /* Echo from batch start to first XQ (or to batch_end if no XQ found) */
  int echo_end = (first_xq_pos >= 0) ? first_xq_pos : ctx->batch_end_card;
  write_input_cards_excluding_end(file, ctx, deck,
                                   ctx->batch_start_card, echo_end,
                                   offset);
}

/******************************************************************************
 * write_remaining_execution_cards() [DEPRECATED - BATCH MODE ONLY]
 *
 * Echoes any execution cards (EX/XQ pairs) that come after the first XQ
 * in the batch. Used when multiple executions are processed at the same
 * frequency to match Fortran's behavior of echoing cards before each
 * execution's output.
 *
 * DEPRECATED: This function is part of the legacy batch processing system.
 * It is only called from control.c batch processing functions. The active
 * sequential processing pathway in reporting.c does not use this function.
 */
void write_remaining_execution_cards(FILE *file, const context_t *ctx, const deck_t *deck)
{
  if (file == NULL || ctx == NULL || deck == NULL)
    return;

  /* Find the first XQ in the batch */
  int first_xq_pos = -1;
  for (int i = ctx->batch_start_card; i <= ctx->batch_end_card && i < deck->num_cards; i++) {
    if (strncmp(deck->cards[i].card_code, "XQ", 2) == 0) {
      first_xq_pos = i;
      break;
    }
  }

  /* If there are cards after the first XQ, echo them */
  if (first_xq_pos >= 0 && first_xq_pos < ctx->batch_end_card) {
    /* Count cards echoed so far up to and including first XQ */
    int offset = count_echoed_cards_in_range(deck, deck->geometry_end + 1,
                                              first_xq_pos);
    
    /* Echo from after first XQ to batch end */
    write_input_cards_excluding_end(file, ctx, deck,
                                     first_xq_pos + 1, ctx->batch_end_card,
                                     offset);
  }
}

/******************************************************************************
 * write_end_cards()
 *
 * Outputs EN and NX cards as separate batches at the end of the output,
 * each with their own DATA CARD No: line.
 */
void write_end_cards(FILE *file, const context_t *ctx, const deck_t *deck)
{
  if (file == NULL || deck == NULL)
  {
    return;
  }

  int en_card_found = 0;
  card_t last_rp_card = {0};
  int found_rp = 0;

  /* Count control cards to number the EN/NX cards correctly, and find the last RP card. */
  for (int i = 0; i < deck->num_cards; i++)
  {
    card_t *card = &deck->cards[i];

    if (strncmp(card->card_code, "EN", 2) == 0)
    {
      en_card_found = 1;
    }

    if (strncmp(card->card_code, "RP", 2) == 0)
    {
      found_rp = 1;
      last_rp_card = *card;
    }

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
      /* Nothing to do; this loop only finds the last RP card and determines whether EN exists. */
    }
  }

  int current_card_number = 0;

  for (int i = 0; i < deck->num_cards; i++)
  {
    card_t *card = &deck->cards[i];
    bool is_control_card =
        strncmp(card->card_code, "FR", 2) == 0 ||
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
        strncmp(card->card_code, "XT", 2) == 0;

    if (!is_control_card)
    {
      continue;
    }

    current_card_number++;

    if (strncmp(card->card_code, "EN", 2) == 0)
    {
      fprintf(file, "\n\n\n\n");
      /* Always use the actual EN card's parameters when it exists in the deck */
      if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL)
      {
        fprintf(file, " ***** DATA CARD NO. %2d   EN %3d%6d%6d%6d",
                current_card_number,
                card->i[1], card->i[2], card->i[3], card->i[4]);
      }
      else
      {
        fprintf(file, "  DATA CARD No: %3d EN %3d%6d%6d%6d",
                current_card_number,
                card->i[1], card->i[2], card->i[3], card->i[4]);
      }
      for (int j = 1; j <= 6; j++)
      {
        fprintf(file, " %12.5E", card->f[j]);
      }
      fprintf(file, "\n");
    }
    else if (strncmp(card->card_code, "NX", 2) == 0)
    {
      if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL)
      {
        fprintf(file, " ***** DATA CARD NO. %2d   NX   %d   %d     %d  %d",
                current_card_number,
                card->i[1], card->i[2], card->i[3], card->i[4]);
      }
      else
      {
        fprintf(file, "  DATA CARD No: %3d NX   %d   %d     %d  %d",
                current_card_number,
                card->i[1], card->i[2], card->i[3], card->i[4]);
      }
      for (int j = 1; j <= 6; j++)
      {
        fprintf(file, " %12.5E", card->f[j]);
      }
      fprintf(file, "\n");
    }
  }

  if (!en_card_found)
  {
    current_card_number++;
    if (found_rp)
    {
      if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL)
      {
        fprintf(file, " ***** DATA CARD NO. %2d   EN   %d   %d     %d  %d",
                current_card_number,
                last_rp_card.i[1], last_rp_card.i[2], last_rp_card.i[3], last_rp_card.i[4]);
      }
      else
      {
        fprintf(file, "  DATA CARD No: %3d EN   %d   %d     %d  %d",
                current_card_number,
                last_rp_card.i[1], last_rp_card.i[2], last_rp_card.i[3], last_rp_card.i[4]);
      }
      for (int j = 1; j <= 6; j++)
      {
        fprintf(file, " %12.5E", last_rp_card.f[j]);
      }
      fprintf(file, "\n");
    }
    else // there is an EN card
    {
      if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL)
      {
        fprintf(file, " ***** DATA CARD NO. %2d   EN   0     0     0     0  0.00000E+00  0.00000E+00  0.00000E+00  0.00000E+00  0.00000E+00  0.00000E+00\n",
                current_card_number);
      }
      else
      {
        fprintf(file, "  DATA CARD No: %3d EN   0     0     0     0  0.00000E+00  0.00000E+00  0.00000E+00  0.00000E+00  0.00000E+00  0.00000E+00\n",
                current_card_number);
      }
    }
  }
}

/******************************************************************************
 * write_frequency_data
 *
 * Writes the frequency in MHz and wavelength in meters, plus integration
 * method information. This matches the NEC2 output format.
 */
void write_frequency_data(FILE *file, const context_t *ctx)
{
  const output_format_spec_t *fmt = get_format(ctx);
  const char *frequency_header_prefix = ctx->freq_step_output_written ? "\n\n\n" : "\n\n";
  
  if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL)
  {
    /* Original Fortran format with spaces around FREQUENCY */
    fprintf(file, "%s"
                  "                                 "
                  "- - - - - - FREQUENCY - - - - - -\n"
                  "\n"
                  "                                    "
                  "%s%11.4E %s\n"
                  "                                    "
                  "%s%11.4E %s",
            frequency_header_prefix,
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
    fprintf(file, "%s"
                  "                               "
                  "%s%s%s\n"
                  "                                "
                  "%s%11.4E %s\n"
                  "                                "
                  "%s%11.4E %s",
            frequency_header_prefix,
            fmt->header_separator, "FREQUENCY", fmt->header_separator,
            fmt->frequency_label, ctx->save.freq_mhz, fmt->freq_units,
            fmt->wavelength_label, ctx->geometry.wavelength, fmt->length_units);

    fprintf(file, "\n\n\n"
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
void write_loading_data(FILE *file, const context_t *ctx)
{
  if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL)
  {
    /* Fortran FORMAT 146: (///,...) = 3 slashes = 3 blank lines before the header.
       The preceding APPROX INTEGRATION line has no trailing \n, so:
       \n ends that line, then 3 more \n produce 3 blank lines. */
    fprintf(file, "\n\n\n\n"
                  "                               "
                  "- - - STRUCTURE IMPEDANCE LOADING - - -\n");
  }
  else
  {
    fprintf(file, "\n\n\n\n"
                  "                          "
                  "------ STRUCTURE IMPEDANCE LOADING ------\n");
  }

  if (ctx->zload.num_loads == 0)
  {
    fprintf(file, "\n"
                  "                                   "
                  "THIS STRUCTURE IS NOT LOADED\n");
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
  fprintf(file, "\n");
}

/******************************************************************************
 * write_environment_data
 *
 * Writes the antenna environment section (free space, perfect ground, or
 * finite ground with parameters).
 */
void write_environment_data(FILE *file, const context_t *ctx)
{
  if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL)
  {
    fprintf(file, "\n\n\n"
                  "                                  "
                  "- - - ANTENNA ENVIRONMENT - - -\n\n");
  }
  else
  {
    fprintf(file, "\n\n\n"
                  "                            "
                  "-------- ANTENNA ENVIRONMENT --------\n\n");
  }

  if (ctx->gnd.has_ground == 1)
  {
    fprintf(file, "\n\n"
                  "                                            "
                  "FREE SPACE\n");
  }
  else
  {
    if (ctx->gnd.is_perfect == 1)
    {
      fprintf(file, "\n\n"
                    "                                            "
                    "PERFECT GROUND\n");
    }
    else
    {
      // Radial wire ground screen
      if (ctx->gnd.num_radials != 0)
      {
        if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL)
        {
          fprintf(file, "\n\n"
                        "                                            "
                        "RADIAL WIRE GROUND SCREEN\n"
                        "                                            "
                        "%d WIRES\n"
                        "                            "
                        "WIRE LENGTH= %8.2f METERS\n"
                        "                            "
                        "WIRE RADIUS= %10.3E METERS\n",
                  ctx->gnd.num_radials, ctx->save.screen_wire_len, ctx->save.screen_wire_radius);
        }
        else
        {
          fprintf(file, "\n\n"
                        "                                            "
                        "RADIAL WIRE GROUND SCREEN\n"
                        "                                            "
                        "%d WIRES\n"
                        "                            "
                        "WIRE LENGTH: %8.2f METERS\n"
                        "                            "
                        "WIRE RADIUS: %10.3E METERS\n",
                  ctx->gnd.num_radials, ctx->save.screen_wire_len, ctx->save.screen_wire_radius);
        }

        fprintf(file, "                            "
                      "MEDIUM UNDER SCREEN -\n");
      }

      // Ground type
      if (ctx->gnd.is_perfect != 2)
      {
        if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL)
        {
          fprintf(file, "                                        "
                        "FINITE GROUND.  REFLECTION COEFFICIENT APPROXIMATION\n");
        }
        else
        {
          fprintf(file, "                            "
                        "FINITE GROUND - REFLECTION COEFFICIENT APPROXIMATION\n");
        }
      }
      else
      {
        if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL)
        {
          fprintf(file, "                                        "
                        "FINITE GROUND.  SOMMERFELD SOLUTION\n");
        }
        else
        {
          fprintf(file, "                            "
                        "FINITE GROUND - SOMMERFELD SOLUTION\n");
        }
      }

      // Ground parameters
      complex double epsc = cmplx(ctx->save.ground_epsr, -ctx->save.ground_sigma * ctx->geometry.wavelength * 59.96);
      if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL)
      {
        fprintf(file, "                                        "
                      "RELATIVE DIELECTRIC CONST.= %.3f\n"
                      "                                        "
                      "CONDUCTIVITY=%10.3E MHOS/METER\n"
                      "                                        "
                      "COMPLEX DIELECTRIC CONSTANT= %11.5E%+11.5E\n",
                ctx->save.ground_epsr, ctx->save.ground_sigma, creal(epsc), cimag(epsc));
      }
      else
      {
        fprintf(file, "                            "
                      "RELATIVE DIELECTRIC CONST: %.3f\n"
                      "                            "
                      "CONDUCTIVITY: %10.3E MHOS/METER\n"
                      "                            "
                      "COMPLEX DIELECTRIC CONSTANT: %11.4E%+11.4Ej\n",
                ctx->save.ground_epsr, ctx->save.ground_sigma, creal(epsc), cimag(epsc));
      }
    }
  }
}

/******************************************************************************
 * write_matrix_timing
 *
 * Writes the matrix fill and factor timing information.
 */
void write_matrix_timing(FILE *file, const context_t *ctx)
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
  fprintf(file, "\n");  // Add newline after matrix timing
}

/******************************************************************************
 * write_network_data
 *
 * Writes the network data section showing transmission lines and network
 * connections between segments.
 */
void write_network_data(FILE *file, const context_t *ctx)
{
  if (ctx->netcx.num_networks == 0)
  {
    return; // No network data to write
  }

  fprintf(file, "\n\n\n");

  if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL)
  {
    fprintf(file, "                                            "
                  "- - - NETWORK DATA - - -");
  }
  else
  {
    fprintf(file, "                                            "
                  "---------- NETWORK DATA ----------");
  }

  int itmp1 = ctx->netcx.net_types[0];
  int itmp3 = 0;
  const char *pnet[6] = {"      ", "  ", "STRAIG", "HT", "CROSSE", "D"};

  for (int i = 0; i < 2; i++)
  {
    if (itmp1 == 3)
      itmp1 = 2;

    if (itmp1 == 2)
    {
      if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL)
      {
        fprintf(file, "\n\n"
                      "      - FROM -    - TO -           TRANSMISSION LINE               "
                      "-  -  SHUNT ADMITTANCES (MHOS)  -  -              LINE\n"
                      "      TAG  SEG.   TAG  SEG.      IMPEDANCE      LENGTH            "
                      "- END ONE -                 - END TWO -            TYPE\n"
                      "      NO.   NO.   NO.   NO.         OHMS        METERS         "
                      "REAL          IMAG.         REAL          IMAG.");
      }
      else
      {
        fprintf(file, "\n"
                      "  -- FROM -  --- TO --      TRANSMISSION LINE       "
                      " --------- SHUNT ADMITTANCES (MHOS) ---------   LINE\n"
                      "  TAG   SEG  TAG   SEG    IMPEDANCE      LENGTH    "
                      " ----- END ONE -----      ----- END TWO -----   TYPE\n"
                      "  No:   No:  No:   No:         OHMS      METERS     "
                      " REAL      IMAGINARY      REAL      IMAGINARY");
      }
    }
    else if (itmp1 == 1)
    {
      if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL)
      {
        fprintf(file, "\n\n"
                      "      - FROM -    - TO -           -  -  ADMITTANCE MATRIX ELEMENTS "
                      "(MHOS)  -  -\n"
                      "      TAG  SEG.   TAG  SEG.         (ONE,ONE)                 "
                      "(ONE,TWO)                 (TWO,TWO)\n"
                      "      NO.   NO.   NO.   NO.        REAL      IMAG.      REAL      "
                      "IMAG.      REAL      IMAG.");
      }
      else
      {
        fprintf(file, "\n"
                      "  -- FROM -  --- TO --            --------"
                      " ADMITTANCE MATRIX ELEMENTS (MHOS) ---------\n"
                      "  TAG   SEG  TAG   SEG   ----- (ONE,ONE) ------  "
                      " ----- (ONE,TWO) -----   ----- (TWO,TWO) -------\n"
                      "  No:   No:  No:   No:      REAL      IMAGINARY     "
                      " REAL     IMAGINARY       REAL      IMAGINARY");
      }
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
                      "    %d   %d    %d   %d   %10.4E  %10.4E   "
                      "%10.4E  %10.4E   %10.4E  %10.4E%s%s",
                ctx->geometry.tag_nums[idx4], itmp4,
                ctx->geometry.tag_nums[idx5], itmp5,
                ctx->netcx.y11_real[j], ctx->netcx.y11_imag[j],
                ctx->netcx.y12_real[j], ctx->netcx.y12_imag[j],
                ctx->netcx.y22_real[j], ctx->netcx.y22_imag[j],
                pnet[2*itmp2 - 2], pnet[2*itmp2 - 1]);
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
void write_matrix_asymmetry(FILE *file, const context_t *ctx)
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
void write_network_excitation(FILE *file, const context_t *ctx)
{
  if (ctx->netcx.nexc == 0 || ctx->netcx.print_net_data != 0)
  {
    return; // No excitation data or printing suppressed
  }

  if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL)
  {
    fprintf(file, "\n\n\n"
                  "                           "
                  "- - - STRUCTURE EXCITATION DATA AT NETWORK CONNECTION POINTS - - -\n");

    fprintf(file, "\n"
                  "   TAG   SEG.    VOLTAGE (VOLTS)         "
                  "CURRENT (AMPS)         IMPEDANCE (OHMS)        "
                  "ADMITTANCE (MHOS)      POWER\n"
                  "   NO.   NO.    REAL        IMAG.       "
                  "REAL        IMAG.       REAL        IMAG.       "
                  "REAL        IMAG.     (WATTS)");

    for (int i = 0; i < ctx->netcx.nexc; i++)
    {
      fprintf(file, "\n"
                    " %5d %5d% 12.5E% 12.5E% 12.5E% 12.5E% 12.5E% 12.5E% 12.5E% 12.5E% 12.5E",
              ctx->netcx.exc_tag[i], ctx->netcx.exc_seg[i],
              creal(ctx->netcx.exc_v[i]), cimag(ctx->netcx.exc_v[i]),
              creal(ctx->netcx.exc_i[i]), cimag(ctx->netcx.exc_i[i]),
              creal(ctx->netcx.exc_z[i]), cimag(ctx->netcx.exc_z[i]),
              creal(ctx->netcx.exc_y[i]), cimag(ctx->netcx.exc_y[i]),
              ctx->netcx.exc_pwr[i]);
    }
  }
  else
  {
    fprintf(file, "\n\n\n"
                  "                           "
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
}

/******************************************************************************
 * write_antenna_input_parameters
 *
 * Writes antenna input parameters at source segments, including voltage,
 * current, impedance, admittance, and power.
 */
void write_antenna_input_parameters(FILE *file, const context_t *ctx)
{
  if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL)
  {
    fprintf(file, "\n\n\n"
                  "                                          "
                  "- - - ANTENNA INPUT PARAMETERS - - -\n");

    if (ctx->netcx.ninp > 0) {
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
                      " %5d %5d% 12.5E% 12.5E% 12.5E% 12.5E% 12.5E% 12.5E% 12.5E% 12.5E% 12.5E",
                ctx->netcx.inp_tag[i], ctx->netcx.inp_seg[i],
                creal(ctx->netcx.inp_v[i]), cimag(ctx->netcx.inp_v[i]),
                creal(ctx->netcx.inp_i[i]), cimag(ctx->netcx.inp_i[i]),
                creal(ctx->netcx.inp_z[i]), cimag(ctx->netcx.inp_z[i]),
                creal(ctx->netcx.inp_y[i]), cimag(ctx->netcx.inp_y[i]),
                ctx->netcx.inp_pwr[i]);
      }
    }
  }
  else
  {
    fprintf(file, "\n\n\n"
                  "                        "
                  "--------- ANTENNA INPUT PARAMETERS ---------");

    if (ctx->netcx.ninp > 0) {

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
}

/******************************************************************************
 * write_currents
 *
 * Writes current distribution for all segments, including coordinates,
 * segment length, and current magnitude and phase.
 */
void write_currents(FILE *file, const context_t *ctx)
{
  if (ctx->geometry.num_segs == 0)
  {
    return; // No segments to write
  }

  /* Check PT card control: match Fortran behavior
     IPTFLG = -1: Suppress CURRENTS output completely
     IPTFLG = -2: Output currents (no tag range filtering)
     IPTFLG = 0: Output currents with detailed formatting
     IPTFLG > 0: Radiation pattern mode */
  if (ctx->fpat.currents_pattern_print_control == -1)
  {
    return;  // PT=-1 suppresses current output
  }

  if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL)
  {
    /* For Freq 2+, add one extra newline to shift down by 1 line */
    if (ctx->freq_step_output_written)
    {
      fprintf(file, "\n\n\n\n");
    }
    else
    {
      fprintf(file, "\n\n\n\n");
    }
    fprintf(file, "                             "
                  "- - - CURRENTS AND LOCATION - - -\n"
                  "\n"
                  "                                 "
                  "DISTANCES IN WAVELENGTHS");

    fprintf(file, "\n\n\n"
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
    fprintf(file, "\n");
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
 * write_patch_currents
 *
 * Writes the surface patch currents section (FORMAT 197/198 from Fortran).
 * Only output if there are patches (num_patches > 0).
 * Converts rectangular components (EX, EY, EZ) to tangent vector components
 * (T1 magnitude/phase, T2 magnitude/phase).
 */
void write_patch_currents(FILE *file, const context_t *ctx)
{
  if (ctx->geometry.num_patches == 0)
  {
    return; // No patches to write
  }

  /* Check PT card control: match Fortran behavior
     IPTFLG = -1: Suppress CURRENTS output completely
     IPTFLG = -2: Output currents (no tag range filtering)
     IPTFLG = 0: Output currents with detailed formatting
     IPTFLG > 0: Radiation pattern mode */
  if (ctx->fpat.currents_pattern_print_control == -1)
  {
    return;  // PT=-1 suppresses current output
  }

  if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL)
  {
    /* FORMAT 197: Header for SURFACE PATCH CURRENTS */
    fprintf(file, "\n\n\n\n");
    fprintf(file, "                                         - - - - SURFACE PATCH CURRENTS - - - -\n");
    fprintf(file, "\n");
    fprintf(file, "                                                  DISTANCE IN WAVELENGTHS\n");
    fprintf(file, "                                                  CURRENT IN AMPS/METER\n");
    fprintf(file, "\n");
    fprintf(file, "                            - - SURFACE COMPONENTS - -                   - - - RECTANGULAR COMPONENTS - - -\n");
    fprintf(file, "      PATCH CENTER      TANGENT VECTOR 1   TANGENT VECTOR 2           X                   Y                   Z\n");
    fprintf(file, "     X      Y      Z     MAG.       PHASE   MAG.       PHASE    REAL      IMAG.     REAL      IMAG.     REAL      IMAG. ");

    /* FORMAT 198: Data output for each patch */
    for (int i = 0; i < ctx->geometry.num_patches; i++)
    {
      /* Surface current components in rectangular coordinates (Amps/meter, no wavelength scaling) */
      int surf_cur_idx = ctx->geometry.num_segs + 3 * i;
      complex double ex = ctx->crnt.surface_cur[surf_cur_idx];
      complex double ey = ctx->crnt.surface_cur[surf_cur_idx + 1];
      complex double ez = ctx->crnt.surface_cur[surf_cur_idx + 2];

      /* Convert to tangent vector components */
      complex double eth = ex * ctx->geometry.patch_t1x[i] +
                           ey * ctx->geometry.patch_t1y[i] +
                           ez * ctx->geometry.patch_t1z[i];
      complex double eph = ex * ctx->geometry.patch_t2x[i] +
                           ey * ctx->geometry.patch_t2y[i] +
                           ez * ctx->geometry.patch_t2z[i];

      double ethm = cabs(eth);
      double etha = carg(eth) * TD; // Convert to degrees (TD = 57.29577951)
      double ephm = cabs(eph);
      double epha = carg(eph) * TD;

      /* Patch number on its own line */
      fprintf(file, "\n %4d", i + 1);
      
      /* Data line: patch center (X,Y,Z in wavelengths), T1/T2 mag/phase, rectangular components */
      fprintf(file, "\n %7.3f%7.3f%7.3f%11.4E%8.2f%11.4E%8.2f%10.2E%10.2E%10.2E%10.2E%10.2E%10.2E",
              ctx->geometry.patch_x_center[i],
              ctx->geometry.patch_y_center[i],
              ctx->geometry.patch_z_center[i],
              ethm, etha,
              ephm, epha,
              creal(ex), cimag(ex),
              creal(ey), cimag(ey),
              creal(ez), cimag(ez));
    }
    fprintf(file, "\n");
  }
  else
  {
    /* nec2c format - similar header but with possibly different formatting */
    fprintf(file, "\n\n\n");
    fprintf(file, "                            -------- SURFACE PATCH CURRENTS --------\n");
    fprintf(file, "\n\n");
    fprintf(file, "                           DISTANCE IN WAVELENGTHS\n");
    fprintf(file, "                           CURRENT IN AMPS/METER\n");
    fprintf(file, "\n");
    fprintf(file, "       PATCH CENTER    TANGENT VECTOR 1  TANGENT VECTOR 2        X              Y              Z\n");
    fprintf(file, "     X      Y      Z    MAG        PHASE  MAG        PHASE   REAL     IMAG  REAL     IMAG  REAL     IMAG");

    for (int i = 0; i < ctx->geometry.num_patches; i++)
    {
      int surf_cur_idx = ctx->geometry.num_segs + 3 * i;
      complex double ex = ctx->crnt.surface_cur[surf_cur_idx];
      complex double ey = ctx->crnt.surface_cur[surf_cur_idx + 1];
      complex double ez = ctx->crnt.surface_cur[surf_cur_idx + 2];

      complex double eth = ex * ctx->geometry.patch_t1x[i] +
                           ey * ctx->geometry.patch_t1y[i] +
                           ez * ctx->geometry.patch_t1z[i];
      complex double eph = ex * ctx->geometry.patch_t2x[i] +
                           ey * ctx->geometry.patch_t2y[i] +
                           ez * ctx->geometry.patch_t2z[i];

      double ethm = cabs(eth);
      double etha = carg(eth) * TD;
      double ephm = cabs(eph);
      double epha = carg(eph) * TD;

      fprintf(file, "\n %4d", i + 1);
      fprintf(file, "\n  %7.3f %7.3f %7.3f %10.3E %8.2f %10.3E %8.2f %9.2E %9.2E %9.2E %9.2E %9.2E %9.2E",
              ctx->geometry.patch_x_center[i],
              ctx->geometry.patch_y_center[i],
              ctx->geometry.patch_z_center[i],
              ethm, etha,
              ephm, epha,
              creal(ex), cimag(ex),
              creal(ey), cimag(ey),
              creal(ez), cimag(ez));
    }
  }
}

/******************************************************************************
 * write_power_budget
 *
 * Writes the power budget showing input power, radiated power, structure
 * loss, network loss, and efficiency.
 */
void write_power_budget(FILE *file, const context_t *ctx)
{
  // Only write for standard radiation pattern types
  if ((ctx->fpat.excitation_type != 0) && (ctx->fpat.excitation_type != 5))
  {
    return;  // Skip power budget for non-standard excitation types
  }

  double tmp1 = ctx->netcx.power_in - ctx->netcx.power_net_loss - ctx->fpat.ohmic_loss;
  double tmp2 = 100.0 * tmp1 / ctx->netcx.power_in;

  if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL)
  {
    fprintf(file, "\n\n\n"
                  "                                        "
                  "- - - POWER BUDGET - - -\n"
                  "\n"
                  "                                           "
                  "INPUT POWER   = %10.4E WATTS\n"
                  "                                           "
                  "RADIATED POWER= %10.4E WATTS\n"
                  "                                           "
                  "STRUCTURE LOSS= %10.4E WATTS\n"
                  "                                           "
                  "NETWORK LOSS  = %10.4E WATTS\n"
                  "                                           "
                  "EFFICIENCY    = %6.2f PERCENT\n",
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
void write_radiation_pattern_header(FILE *file, const context_t *ctx)
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
      fprintf(file, "\n\n\n\n"
                    "                                                "
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
void write_radiation_pattern_data(FILE *file, const context_t *ctx)
{
  char *hpol[4] = {"LINEAR", "RIGHT ", "LEFT  ", "      "}; /* 4th entry is blank for no radiation */
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
                pt->tilta, hpol[pt->pol_sense >= 0 && pt->pol_sense <= 3 ? pt->pol_sense : 0],
                pt->ethm, pt->etha, pt->ephm, pt->epha);
      }
      else
      {
        /* Modern nec2c format */
        fprintf(file, "\n"
                      " %7.2f %9.2f  %8.2f %8.2f %8.2f %11.4f"
                      " %9.2f %6s %11.4E %9.2f %11.4E %9.2f",
                pt->theta, pt->phi, tmp5, tmp6, pt->gtot, pt->axrat,
                pt->tilta, hpol[pt->pol_sense >= 0 && pt->pol_sense <= 3 ? pt->pol_sense : 0],
                pt->ethm, pt->etha, pt->ephm, pt->epha);
      }
    }
  }
  /* Fewer trailing newlines for Freq 2+ to balance extra newline in write_currents */
  if (ctx->freq_step_output_written)
  {
    fprintf(file, "\n\n");
  }
  else
  {
    fprintf(file, "\n\n");
  }
}

/******************************************************************************
 * write_average_power_gain
 *
 * Writes the average power gain over the specified solid angle.
 */
void write_average_power_gain(FILE *file, const context_t *ctx)
{
  if (ctx->fpat.avg_power_flag == 0)
  {
    return;
  }

  /* Fewer leading newlines for Freq 2+ to balance extra newline in write_currents */
  if (ctx->freq_step_output_written)
  {
    fprintf(file, "\n\n");
  }
  else
  {
    fprintf(file, "\n\n\n");
  }
  fprintf(file, "  AVERAGE POWER GAIN: %11.4E - SOLID ANGLE"
                " USED IN AVERAGING: (%+7.4f)*PI STERADIANS",
          ctx->rpat.pint, ctx->rpat.solid_angle);
}

/******************************************************************************
 * write_normalized_gain
 *
 * Writes the normalized gain table if requested.
 */
void write_normalized_gain(FILE *file, const context_t *ctx)
{
  char *igntp[5] = {" MAJOR AXIS", "  MINOR AXIS",
                    "    VERTICAL", "  HORIZONTAL", "       TOTAL "};

  if (ctx->fpat.normalize_gain == 0 || ctx->rpat.num_points == 0)
  {
    return;
  }

  int itmp1 = ctx->fpat.normalize_gain - 1;

  if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL)
  {
    fprintf(file, "\n\n\n"
                  "                                   "
                  "- - - - NORMALIZED GAIN - - - -\n"
                  "                                      %6s GAIN\n"
                  "                                   "
                  "NORMALIZATION FACTOR = %.2f DB\n\n"
                  "    - - ANGLES' - -      GAIN      - - ANGLES' - -      GAIN      - - ANGLES' - -      GAIN\n"
                  "    THETA     PHI        DB     THETA     PHI        DB     THETA     PHI        DB\n"
                  "   DEGREES   DEGREES       DEGREES   DEGREES       DEGREES   DEGREES",
            igntp[itmp1], ctx->rpat.gmax);
  }
  else
  {
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
  }

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
void write_near_field_data(FILE *file, const context_t *ctx)
{
  if (ctx->nfr.num_points == 0 || ctx->nfr.points == NULL)
    return;

  if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL)
  {
    if (ctx->nfr.nfeh != 1)
    {
      fprintf(file, "\n\n\n"
              "                                   "
              "- - - NEAR ELECTRIC FIELDS - - -\n\n"
              "            "
              "-  LOCATION  -"              "                     "
              "-  EX  -"                    "               "
              "-  EY  -"                    "               "
              "-  EZ  -\n"
              "        "
              "        X"                  "          Y"    "          Z"    "          MAGNITUDE"    "   PHASE"    "      MAGNITUDE"    "   PHASE"    "      MAGNITUDE"    "   PHASE\n"
              "      METERS"              "     METERS"    "     METERS"    "        VOLTS/M"    "   DEGREES"    "      VOLTS/M"    "   DEGREES"    "      VOLTS/M"    "   DEGREES");
    }
    else
    {
      fprintf(file, "\n\n\n"
              "                                   "
              "- - - NEAR MAGNETIC FIELDS - - -\n\n"
              "            "
              "-  LOCATION  -"              "                     "
              "-  HX  -"                    "               "
              "-  HY  -"                    "               "
              "-  HZ  -\n"
              "        "
              "        X"                  "          Y"    "          Z"    "          MAGNITUDE"    "   PHASE"    "      MAGNITUDE"    "   PHASE"    "      MAGNITUDE"    "   PHASE\n"
              "      METERS"              "     METERS"    "     METERS"    "         AMPS/M"    "   DEGREES"    "       AMPS/M"    "   DEGREES"    "       AMPS/M"    "   DEGREES");
    }
  }
  else
  {
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
    if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL)
    {
      fprintf(file, "\n"
              "  %9.4f  %9.4f  %9.4f    %11.4E  %7.2f   %11.4E  %7.2f   %11.4E  %7.2f",
              pt->xob, pt->yob, pt->zob, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6);
    }
    else
    {
      fprintf(file, "\n"
              " %9.4f %9.4f %9.4f  %11.4E %7.2f  %11.4E %7.2f  %11.4E %7.2f",
              pt->xob, pt->yob, pt->zob, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6);
    }
  }
}

/******************************************************************************
 * write_near_field_plot
 *
 * Writes near-field data to the plot file (ctx->plot_fp) if PT/PQ plot output
 * was requested.  No-op if no data, no plot file, or iplp1 != 2.
 */
void write_near_field_plot(const context_t *ctx)
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
  write_end_cards(file, ctx, deck);

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


/******************************************************************************
 * write_coupling_data()
 *
 * Renders the CP (coupling) isolation table accumulated by compute_coupling() in
 * calculations.c.  No-op if no coupling rows were recorded.
 */
void write_coupling_data(context_t *ctx)
{
  /* Only output header if there is coupling data */
  if (ctx->yparm.num_coupling_rows > 0) {
    fprintf(ctx->output_fp, "\n\n"
                            "                                    - - - ISOLATION DATA - - -\n"
                            "\n"
                            "      - - COUPLING BETWEEN - -        MAXIMUM               - - - FOR MAXIMUM COUPLING - - -\n"
                            "            SEG.              SEG.   COUPLING    LOAD IMPEDANCE (2ND SEG.)       INPUT IMPEDANCE\n"
                            "  TAG/SEG.   NO.    TAG/SEG.   NO.      (DB)        REAL         IMAG.         REAL         IMAG.");

    for (int i = 0; i < ctx->yparm.num_coupling_rows; i++)
    {
      coupling_row_t *r = &ctx->yparm.coupling_rows[i];
      if (!r->is_error)
      {
        fprintf(ctx->output_fp, "\n"
                                " %4d %4d %5d   %4d %4d %5d  %9.3f"
                                "    %12.5E %12.5E   %12.5E %12.5E",
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
    /* Add newline after all coupling rows */
    fprintf(ctx->output_fp, "\n");
  }
  
  /* Clear coupling rows after output so they don't accumulate between frequencies */
  ctx->yparm.num_coupling_rows = 0;
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
  
  /* MATRIX TIMING is output in the original Fortran NEC-2D format */
  if (ctx->output_format == OUTPUT_FORMAT_ORIGINAL)
  {
    write_matrix_timing(file, ctx);
  }
  
  write_network_data(file, ctx);
  write_matrix_asymmetry(file, ctx);
  write_network_excitation(file, ctx);
  write_antenna_input_parameters(file, ctx);
  write_currents(file, ctx);
  write_patch_currents(file, ctx);
  write_power_budget(file, ctx);
  write_coupling_data(ctx);
  write_radiation_pattern_header(file, ctx);
  write_radiation_pattern_data(file, ctx);
  write_average_power_gain(file, ctx);
  write_normalized_gain(file, ctx);
  write_near_field_data(file, ctx);
  write_near_field_plot(ctx);
}

/******************************************************************************
 * write_extra_pattern_output() [DEPRECATED - BATCH MODE ONLY]
 *
 * Writes only the radiation-pattern or near-field section, without repeating
 * the frequency header, loading, timing, or power budget.  Called when a
 * second RP/NE/NH card appears without an intervening FR — mirrors nec2c's
 * igo==4→5→6 path.
 *
 * DEPRECATED: This function is part of the legacy batch processing system.
 * It is only called from control.c (execute_extra_patterns). The active
 * sequential processing pathway in reporting.c does not use this function.
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

/******************************************************************************
 * write_subsequent_excitation_output() [DEPRECATED - BATCH MODE ONLY]
 *
 * Writes only excitation output (antenna input, currents, power, patterns)
 * without the frequency header, loading, or environment sections. Used when
 * processing a subsequent EX card at the same frequency as the previous
 * output — this mirrors Fortran behavior where the FREQUENCY header is
 * output only once per unique frequency, not for each excitation. Also
 * reprints the EX and XQ cards before this output to match Fortran structure.
 *
 * DEPRECATED: This function is part of the legacy batch processing system.
 * It is only called from control.c (run_simulation's batch processing).
 * The active sequential processing pathway in reporting.c does not use
 * this function.
 */
void write_subsequent_excitation_output(FILE *file, context_t *ctx, const deck_t *deck)
{
  /* For the Fortran format, when processing subsequent EX/XQ pairs at the same
     frequency, we re-echo the execution cards that follow the first XQ
     in the batch, matching Fortran's behavior of printing cards before each
     source output. */
  if (file != NULL && ctx != NULL && deck != NULL && ctx->output_format == OUTPUT_FORMAT_ORIGINAL) {
    fprintf(file, "\n\n");
    write_remaining_execution_cards(file, ctx, deck);
  }
  
  write_antenna_input_parameters(file, ctx);
  write_currents(file, ctx);
  write_patch_currents(file, ctx);
  write_power_budget(file, ctx);
  write_coupling_data(ctx);
  write_radiation_pattern_header(file, ctx);
  write_radiation_pattern_data(file, ctx);
  write_average_power_gain(file, ctx);
  write_normalized_gain(file, ctx);
  write_near_field_data(file, ctx);
  write_near_field_plot(ctx);
}

/******************************************************************************
 * write_single_radiation_pattern()
 *
 * Writes a single radiation pattern's header and data for a specific frequency.
 * Called from sequential processing (reporting.c) when multiple RP cards follow
 * an XQ in the same frequency batch. Outputs the pattern without the frequency
 * header, loading, power budget, or other frequency-step sections.
 *
 * This function is used by the active sequential processing pathway in
 * reporting.c to handle multiple radiation patterns per frequency.
 */
void write_single_radiation_pattern(FILE *file, context_t *ctx)
{
  if (file == NULL || ctx == NULL) return;
  
  write_radiation_pattern_header(file, ctx);
  write_radiation_pattern_data(file, ctx);
}
