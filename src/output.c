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

#include "opennec.h"
#include <stdio.h>
#include <string.h>

/******************************************************************************
 * write_deck_nec
 *
 * Writes a deck in the original NEC2 format. This strips out any
 * extensions like SY, replaces formulas and variables with their
 * numeric values, and optionally strips out any inline or in-deck
 * comments. With this last option turned off, the deck is compatible
 * with nec2c, with it turned on, it is the original NEC2 format.
 *
 * TODO: need to calculate all float values and run any conversions
 *       to base units before exporting!
 *
 * TODO: move this to an export.c
 *
 */
void write_deck_nec(nec_context_t *ctx, deck_t *deck, FILE *file, int remove_inline_comments)
{
  card_t *card;
  int MAX_INTS, MAX_FLTS;
  
  for(int i = 0; i < deck->num_cards; i++) {
    card = &deck->cards[i];
    
    // if we are past the end of the deck, just write out the whole string
    if(i > deck->deck_end) {
      fputs(card->card_str, file);
      fputc('\n', file);
      continue;
    }
    
    // skip extension cards in pure NEC files
    if(is_extension(card)) continue;

    // for comment cards with the CM or CE *in the header*, simply export the card
    if(i <= deck->geometry_start && (strcmp(card->card_code, "CM") == 0 || strcmp(card->card_code, "CE") == 0)) {
      fprintf(file, "%s%s", deck->cards[i].card_code, deck->cards[i].comment);
      fputc('\n', file);
    }
    // for comment cards with other headers, only export if the option is on
    if(is_comment(card)) {
      fprintf(file, "%s%s", deck->cards[i].card_code, deck->cards[i].comment);
      fputc('\n', file);
    }

    // for geometry and command cards, start with the code
    fputs(card->card_code, file);
    
    // get the number of fields for this sort of card
    MAX_INTS = max_int_fields(card);
    MAX_FLTS = max_flt_fields(card);

    // int and float fields
    if(is_control(card) || is_geometry(card)) {
      for(int j = 0; j <= card->ints_used && j <= MAX_INTS; j++) {
        fprintf(file, " %d", card->i[j]);
      }
      for(int j = 0; j <= card->flts_used && j <= MAX_FLTS; j++) {
        fprintf(file, " %G", card->f[j]);
      }
      // close the line
      fputc('\n', file);
    } /* if command or geometry */
  } /* for over cards */
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
void write_deck_onec(nec_context_t *ctx, deck_t *deck, FILE *file)
{
  card_t *card;
  int MAX_FLTS, MAX_INTS;
  
  for(int i = 0; i < deck->num_cards; i++) {
    card = &deck->cards[i];
    
    // if we are past the EN at the end of the deck, write out the whole string
    if(i > deck->deck_end) {
      fputs(card->card_str, file);
      fputc('\n', file);
      continue;
    }

    // comment cards care also easy
    if(is_comment(card)) {
      fputs(card->card_code, file);
      fputs(card->comment, file);
      fputc('\n', file);
      continue;
    }
    
    // the ONEC cards like SY are also generally simple
    if(is_extension(card)) {
      if(strcmp(card->extn_code, "") != 0) {
        fputs(card->extn_code, file);
      }
      fputs(card->card_code, file);
      
      key_value_t *head = card->formulas;
      while(head != NULL) {
        // whitespace and the key
        fputs(" ", file);
        fputs(head->key, file);
        // use the separator they used, or default to = because it's likely an SY
        if(strcmp(&head->separator, "") != 0) {
          fputc(head->separator, file);
        } else {
          fputc('=', file);
        }
        // now the value
        fputs(head->value, file);
        
        // move to the next pair, adding a comment if there is another
        head = head->next;
        if(head != NULL) fputs(",", file);
      }
      // is there also a comment?
      if(card->comment != NULL && strlen(card->comment) > 0) {
        fputs(" !", file); // this means we always convert to ! comments
        fputs(card->comment, file);
      }
      fputc('\n', file);
      continue;
    }
    
    // all the rest of the cards have multiple parts to put together
    
    // start with the card code
    fputs(card->card_code, file);

    // get the number of fields for this sort of card
    MAX_INTS = max_int_fields(card);
    MAX_FLTS = max_flt_fields(card);

    // int fields depending on the card type
    if(is_control(card) || is_geometry(card)) {
      // there is one special case in the integers, if it is a GX card the second
      // integer has to writen as a three digit number
      if(strcmp(card->card_code, "GX") == 0) {
        fputc(' ', file);
        fprintf(file, " %d", card->i[1]);
        fputc(' ', file);
        fprintf(file, " %3d", card->i[2]);
      }
      // other cards might have a formula
      else {
        for(int j = 0; j <= card->ints_used && j <= MAX_INTS; j++) {
          // if this field has an inline formula, write it
          if(card->int_form_inline[j]) {
            fputc(' ', file);
            fputs(unit_codes[card->units[j]], file);
          }
          // otherwise write the number itself
          else {
            fprintf(file, " %d", card->i[j]);
          }
        }
      }
      
      // floats are a number or a formula
      for(int j = 0; j <= card->flts_used && j <= MAX_FLTS; j++) {
        // if this field uses hash as the measurement type, write it first
        if(card->units[j] == 8) {
          fputc('#', file);
        }

        // if this field has an inline formula, write it
        if(card->flt_form_inline[j]) {
          fputc(' ', file);
          fputs(unit_codes[card->units[j]], file);
        }
        // otherwise write the number itself
        else {
          fprintf(file, " %G", card->f[j]);
        }
        
        // if there is any other measurment type, add it at the end
        if(card->units[j] != 0 && card->units[j] != 8) {
          fputs(unit_codes[card->units[j]], file);
        }
      }

      // the basic NEC fields are output, now see if there's anything after that
      bool hasComment = (card->comment != NULL && strlen(card->comment) > 0);
      
      bool hasOnec = false;
      if(card->ignore) hasOnec = true;
      if(card->extensns != NULL ) hasOnec = true;
      if(card->formulas != NULL ) hasOnec = true;
      
      // if we found anything, print the comment marker found on this
      // card, the global one in the deck, or the onec default, !
      if(hasComment || hasOnec) {
        fputc(' ', file);
        if(strlen(card->extn_code) > 0) {
          fputs(card->extn_code, file);
        } else if(deck->cmt_code != 0 ){
          fputc(deck->cmt_code, file);
        } else {
          fputc('!', file);
        }
      }
      
      // if we have *only* a comment, just print that and we're done,
      // otherwise we have to export the fields one by one
      if(hasComment && !hasOnec) {
        fputs(card->comment, file);
      } else {
        if(card->ignore) {
          fputs(" ignore:true", file);
        }
        // formulas next - only the ones that aren't inline
        if(card->formulas != NULL) {
          key_value_t *form = card->formulas;
          while(form != NULL) {
            fputc(' ', file);
            fputs(form->key, file);
            fputc('=', file);
            fputs(form->value, file);
            form = form->next;
          }
        }
        // any other key/value pairs
        if(card->extensns != NULL) {
          key_value_t *pair = card->extensns;
          while(pair != NULL) {
            fputc(' ', file);
            fputs(pair->key, file);
            fputc('=', file);
            fputs(pair->value, file);
            pair = pair->next;
          }
        }
        // and then finally the comment which has to be at the end of the line
        if(card->comment != NULL && strlen(card->comment) > 0) {
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
 * write_nec_output()
 *
 * Writes a standard NEC-style output file, using various work functions.
 *
 */
void write_nec_output(nec_context_t *ctx, deck_t *deck, FILE *file)
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
  write_currents(file, ctx);
  write_power_budget(file, ctx);
  write_radiation_pattern_header(file, ctx);
  write_radiation_pattern_data(file, ctx);
  write_average_power_gain(file, ctx);
  write_normalized_gain(file, ctx);
  write_footer(file, ctx, deck);
}

/******************************************************************************
 * write_greens_matrix()
 *
 * Writes a simple Greens Function file (NGF v1) containing the interaction
 * matrix (cm) as computed prior to factorization, along with basic metadata
 * and segment center coordinates. Format is human-readable text.
 *
 * Header:
 *   # OpenNEC NGF v1
 *   frequency_mhz: <fmhz>
 *   wave_number_k: <2*pi/wavelength>
 *   segments: <ctx->geometry.n>
 *   equations: <nrow>
 *
 * Segment centers:
 *   SEG i x y z len radius
 *
 * Matrix entries:
 *   CM i j re im
 */
void write_greens_matrix(FILE *file, nec_context_t *ctx, int nrow, complex double *cm)
{
  if (!file || !cm || nrow <= 0) return;

  double fmhz = ctx->save.fmhz;
  double wlam = ctx->geometry.wlam; // wavelength in meters
  double k = (wlam > 0.0) ? (2.0 * PI / wlam) : 0.0;

  fprintf(file, "# OpenNEC NGF v1\n");
  fprintf(file, "frequency_mhz: %.6f\n", fmhz);
  fprintf(file, "wave_number_k: %.9f\n", k);
  fprintf(file, "segments: %d\n", ctx->geometry.n);
  fprintf(file, "equations: %d\n", nrow);

  // Segment centers (if available)
  int nseg = ctx->geometry.n;
  if (nseg > 0 && ctx->geometry.x && ctx->geometry.y && ctx->geometry.z) {
    for (int i = 0; i < nseg; i++) {
      double x = ctx->geometry.x[i];
      double y = ctx->geometry.y[i];
      double z = ctx->geometry.z[i];
      double len = (ctx->geometry.si) ? ctx->geometry.si[i] : 0.0;
      double rad = (ctx->geometry.bi) ? ctx->geometry.bi[i] : 0.0;
      fprintf(file, "SEG %d %.9g %.9g %.9g %.9g %.9g\n", i+1, x, y, z, len, rad);
    }
  }

  // Matrix entries (row-major by (i,j) pairs)
  for (int j = 0; j < nrow; j++) {
    for (int i = 0; i < nrow; i++) {
      complex double v = cm[i + j * nrow];
      fprintf(file, "CM %d %d %.15g %.15g\n", i+1, j+1, creal(v), cimag(v));
    }
  }
}

/******************************************************************************
 * write_headers()
 *
 * Writes the header area and comment cards to the standard NEC output file.
 *
 */
void write_header(nec_context_t *ctx, deck_t *deck, FILE *file)
{
  fprintf( ctx->output_fp,  "\n\n\n"
          "                              "
          " __________________________________________\n"
          "                              "
          "|                                          |\n"
          "                              "
          "|  NUMERICAL ELECTROMAGNETICS CODE (onec)  |\n"
          "                              "
          "|   Translated to 'C' in Double Precision  |\n"
          "                              "
          "|__________________________________________|\n" );
  
  fprintf( ctx->output_fp, "\n\n\n"
          "                               "
          "---------------- COMMENTS ----------------\n" );
  
  // write comments to output file
  for(int i = deck->comment_start; i <= deck->comment_end; i++) {
    fprintf(ctx->output_fp, "                              %s\n", deck->cards[i].comment);
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
int write_structure(nec_context_t *ctx, deck_t *deck, FILE *file)
{
  card_t card;
  int geo_card_num;
  int num_wires = 0;
  /* int num_patches = 0; */

  int ix, iy, iz;
  
  // these are used to match various codes in the cards to text output
  char ifx[2] = { '*', 'X' }, ify[2] = { '*','Y' }, ifz[2] = { '*','Z' }; // reflection axes
  //char ipt[4] = { 'P', 'R', 'T', 'Q' };
  
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
  
  for(int i = deck->geometry_start; i <= deck->geometry_end; i++) {
    card = deck->cards[i];
    
    // for onec...
    if(card.ignore) continue;
    
    // if this card is an XT, print it and stop everything
    if(strcmp(card.card_code, "XT") == 0) {
      fprintf(ctx->output_fp, "\nOpenNEC: Exiting after an XT command.\n" );
      break;
    }

    // convert the card code to a number
    for(geo_card_num = 0; geo_card_num < NUM_GEOMETRY_CODES; geo_card_num++) {
      if(strncmp(card.card_code, geometry_codes[geo_card_num], 2) == 0)
        break;
    }
    
    // switch on the number
    switch(geo_card_num) {
        
      case 0: // GW card, a wire
        num_wires++;
        fprintf(ctx->output_fp, "\n"
          " %5d  %10.5f %10.5f %10.5f %10.5f"
          " %10.5f %10.5f %10.5f %5d %5d %5d %4d",
                num_wires, card.fv[1], card.fv[2], card.fv[3], card.fv[4], card.fv[5], card.fv[6], card.fv[7],
                card.num_segments, card.start_segment, card.end_segment, card.tag);
        break;
        
      case 1: // GX card, reflection or rotation
        // decode the flags stored in the I2 value on the card
        iy = card.iv[2] / 10;
        iz = card.iv[2] - iy*10;
        ix = iy / 10;
        iy = iy - ix*10;

        if(ix != 0)
          ix = 1;
        if(iy != 0)
          iy = 1;
        if(iz != 0)
          iz = 1;
        
        fprintf(ctx->output_fp,
                "\n      STRUCTURE REFLECTED ALONG THE AXES %c %c %c"
                " - TAGS INCREMENTED BY %d",
                ifx[ix], ify[iy], ifz[iz], card.i[1] );
        break;
        
      case 3: // GS card, scale structure dimensions
        fprintf(ctx->output_fp,
                "\n     STRUCTURE SCALED BY FACTOR: %10.5f", card.fv[1] );
        break;
        
      case 4: // GE card, nothing to do
        break;
        
      case 5: // GM card, move/copy existing structure
        fprintf(ctx->output_fp,
                "\n     THE STRUCTURE HAS BEEN MOVED, MOVE DATA CARD IS:\n"
                "   %3d %5d %10.5f %10.5f %10.5f %10.5f %10.5f %10.5f %10.5f",
                card.iv[1], card.iv[2], card.fv[1], card.fv[2], card.fv[3], card.fv[4], card.fv[5], card.fv[6], card.fv[7]);
        break;
        
      case 6: // SP card, generate single surface patch
        /* num_patches++; */
        fprintf( ctx->output_fp, "\n"
                " %5d%c %10.5f %10.5f %10.5f %10.5f %10.5f %10.5f",
                card.iv[1], card.iv[i-1], card.fv[1], card.fv[2], card.fv[3], card.fv[4], card.fv[5], card.fv[6]);
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
        fprintf( ctx->output_fp, "\n"
                " %5d ARC RADIUS: %9.5f  FROM: %8.3f TO: %8.3f DEGREES"
                "       %11.5f %5d %5d %5d %4d",
                num_wires, card.fv[1], card.fv[2], card.fv[3], card.fv[4], card.tag, card.start_segment, card.end_segment, card.iv[1]);
        
        //FIXME: this looks wrong, last input
        break;
        
      case 9: // SC card, does nothing
        break;
        
      case 10: // GH card, generate helix
        num_wires++;
        fprintf( ctx->output_fp, "\n"
                " %5d HELIX STRUCTURE - SPACING OF TURNS: %8.3f AXIAL"
                " LENGTH: %8.3f  %8.3f %5d %5d %5d %4d\n      "
                " RADIUS X1:%8.3f Y1:%8.3f X2:%8.3f Y2:%8.3f ",
                num_wires, card.fv[1], card.fv[2], card.fv[7], card.tag, card.start_segment, card.end_segment,
                card.iv[1], card.fv[3], card.fv[4], card.fv[6], card.fv[6]);
        break;
        
    } /* switch on the card type */
  } /* for loop over cards */
  
  // and now a final report on the cards
  fprintf( ctx->output_fp, "\n\n"
          "     TOTAL SEGMENTS USED: %d   SEGMENTS IN A"
          " SYMMETRIC CELL: %d   SYMMETRY FLAG: %d",
          ctx->geometry.n, ctx->geometry.np, ctx->geometry.ipsym );
  
  if(ctx->geometry.m > 0)
    fprintf( ctx->output_fp,  "\n"
            "       TOTAL PATCHES USED: %d   PATCHES"
            " IN A SYMMETRIC CELL: %d",  ctx->geometry.m, ctx->geometry.mp );
  
  int iseg = (ctx->geometry.n + ctx->geometry.m) / (ctx->geometry.np + ctx->geometry.mp);
  if(iseg != 1)  {
    /*** may be error condition?? ***/
    if(ctx->geometry.ipsym == 0) {
      add_error(ctx, &ctx->errors, "ERROR: IPSYM=0 IN CONECT()", FATAL);
      return -1;
    }

    if(ctx->geometry.ipsym < 0)
      fprintf(ctx->output_fp,
              "\n  STRUCTURE HAS %d FOLD ROTATIONAL SYMMETRY\n", iseg );
    else {
      int ic = iseg / 2;
      if(iseg == 8)
        ic = 3;
      fprintf(ctx->output_fp,
              "\n  STRUCTURE HAS %d PLANES OF SYMMETRY\n", ic );
    } /* if(ctx->geometry.ipsym < 0 ) */
  } /* if( iseg != 1) */
  return 0;
} /* write_structure() */

/******************************************************************************
 * write_segments()
 *
 * Writes the segment data section of the nec2 output.
 *
 */
int write_segments(nec_context_t *ctx, deck_t * deck, FILE *file)
{
  // exit now if there's no segments
  if(ctx->geometry.n == 0) return 0;

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
  
  for(int i = 0; i < ctx->geometry.n; i++) {
    xw1 = ctx->geometry.x2[i] - ctx->geometry.x1[i];
    yw1 = ctx->geometry.y2[i] - ctx->geometry.y1[i];
    zw1 = ctx->geometry.z2[i] - ctx->geometry.z1[i];
    ctx->geometry.x[i] = (ctx->geometry.x1[i] + ctx->geometry.x2[i]) / 2.0;
    ctx->geometry.y[i] = (ctx->geometry.y1[i] + ctx->geometry.y2[i]) / 2.0;
    ctx->geometry.z[i] = (ctx->geometry.z1[i] + ctx->geometry.z2[i]) / 2.0;
    xw2 = xw1 * xw1 + yw1 * yw1 + zw1 * zw1;
    yw2 = sqrt(xw2);
    yw2 = (xw2 / yw2 + yw2)*.5;
    ctx->geometry.si[i] = yw2;
    ctx->geometry.cab[i] = xw1 / yw2;
    ctx->geometry.sab[i] = yw1 / yw2;
    xw2 = zw1 / yw2;
    
    if(xw2 > 1.0)
      xw2 = 1.0;
    if(xw2 < -1.0)
      xw2 = -1.0;
    
    ctx->geometry.salp[i] = xw2;
    xw2 = asin(xw2) * TD;
    yw2 = atan2(yw1, xw1) * TD;
    
    fprintf(ctx->output_fp, "\n"
            " %5d %9.4f %9.4f %9.4f %9.4f"
            " %9.4f %9.4f %9.4f %5d %5d %5d %5d",
            i + 1, ctx->geometry.x[i], ctx->geometry.y[i], ctx->geometry.z[i], ctx->geometry.si[i], xw2, yw2,
            ctx->geometry.bi[i] / fr, ctx->geometry.icon1[i], i + 1, ctx->geometry.icon2[i], ctx->geometry.tag_nums[i]);
    
    if(ctx->plot.iplp1 == 1)
      fprintf(ctx->plot_fp, "%12.4E %12.4E %12.4E "
              "%12.4E %12.4E %12.4E %12.4E %5d %5d %5d\n",
              ctx->geometry.x[i], ctx->geometry.y[i], ctx->geometry.z[i], ctx->geometry.si[i], xw2, yw2,
              ctx->geometry.bi[i], ctx->geometry.icon1[i], i + 1, ctx->geometry.icon2[i]);
    
    if((ctx->geometry.si[i] <= 1.e-20) || (ctx->geometry.bi[i] <= 0.0)) {
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
void write_patches(nec_context_t *ctx, deck_t *deck, FILE *file)
{
  // exit now if there's no patches
  if (ctx->geometry.m == 0) return;
  
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
  for(int i = 0; i < ctx->geometry.m; i++) {
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
void write_input_cards(FILE *file, deck_t *deck, int batch_start, int batch_end, int card_number_offset)
{
    if (file == NULL || deck == NULL) {
        return;
    }

    fprintf(file, "\n\n\n");

    /* Iterate through cards in this batch only. */
    int card_number = card_number_offset;
    for (int i = batch_start; i <= batch_end && i < deck->num_cards; i++) {
        card_t *card = &deck->cards[i];
        
        /* Check for XT card - echo it as final card of batch */
        if (strncmp(card->card_code, "XT", 2) == 0) {
            card_number++;
            fprintf(file, "  DATA CARD No: %3d %s", card_number, card->card_code);
            fprintf(file, " %3d", card->iv[1]);
            for (int j = 2; j <= 4; j++) {
                fprintf(file, " %5d", card->iv[j]);
            }
            for (int j = 1; j <= 6; j++) {
                fprintf(file, " %12.5E", card->fv[j]);
            }
            fprintf(file, "\n");
            fprintf(file, "\nOpenNEC: Exiting after an XT command.\n");
            continue;  // Continue to include XT in batch, don't break
        }
        
        /* Do not echo EN here; EN will be printed at the very end of output */
        if (strncmp(card->card_code, "EN", 2) == 0) {
          continue;
        }
        
        /* Only echo control cards (skip geometry and comment cards, and the EN) */
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
            strncmp(card->card_code, "NX", 2) == 0 ||
            strncmp(card->card_code, "PT", 2) == 0 ||
            strncmp(card->card_code, "PQ", 2) == 0 ||
            strncmp(card->card_code, "CP", 2) == 0 ||
            strncmp(card->card_code, "GD", 2) == 0 ||
            strncmp(card->card_code, "WG", 2) == 0 ||
            strncmp(card->card_code, "XQ", 2) == 0) {
            
            card_number++;
            
            /* Output in exact NEC format: card number, card code, 4 ints, 7 floats */
            fprintf(file, "  DATA CARD No: %3d %s", card_number, card->card_code);
            
            /* Output 4 integer fields */
            fprintf(file, " %3d", card->iv[1]);
            for (int j = 2; j <= 4; j++) {
                fprintf(file, " %5d", card->iv[j]);
            }
            
            /* Output 7 float fields in scientific notation */
            for (int j = 1; j <= 6; j++) {
                fprintf(file, " %12.5E", card->fv[j]);
            }
            
            fprintf(file, "\n");
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
void write_frequency_data(FILE *file, nec_context_t *ctx)
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
        "THAT ARE MORE THAN %.3f WAVELENGTHS APART", ctx->dataj.rkh);
    
    if (ctx->dataj.iexk == 1) {
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
void write_loading_data(FILE *file, nec_context_t *ctx)
{
    fprintf(file, "\n\n\n"
        "                          "
        "------ STRUCTURE IMPEDANCE LOADING ------");
    
    if (ctx->zload.nload == 0) {
        fprintf(file, "\n"
            "                                 "
            "THIS STRUCTURE IS NOT LOADED");
    }
}

/******************************************************************************
 * write_environment_data
 * 
 * Writes the antenna environment section (free space, perfect ground, or
 * finite ground with parameters).
 */
void write_environment_data(FILE *file, nec_context_t *ctx)
{
    fprintf(file, "\n\n\n"
        "                            "
        "-------- ANTENNA ENVIRONMENT --------");
    
    if (ctx->gnd.ksymp == 1) {
        fprintf(file, "\n"
            "                            "
            "FREE SPACE");
    }
    else {
        if (ctx->gnd.iperf == 1) {
            fprintf(file, "\n"
                "                            "
                "PERFECT GROUND");
        }
        else {
            // Radial wire ground screen
            if (ctx->gnd.nradl != 0) {
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
            if (ctx->gnd.iperf != 2) {
                fprintf(file, "\n"
                    "                            "
                    "FINITE GROUND - REFLECTION COEFFICIENT APPROXIMATION");
            }
            else {
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
void write_matrix_timing(FILE *file, nec_context_t *ctx)
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
void write_network_data(FILE *file, nec_context_t *ctx)
{
    if (ctx->netcx.nonet == 0) {
        return;  // No network data to write
    }
    
    fprintf(file, "\n\n\n"
        "                                            "
        "---------- NETWORK DATA ----------");
    
    int itmp1 = ctx->netcx.ntyp[0];
    int itmp3 = 0;
    const char *pnet[3] = {"  ", "NON-CROSSED", "CROSSED"};
    
    for (int i = 0; i < 2; i++) {
        if (itmp1 == 3)
            itmp1 = 2;
        
        if (itmp1 == 2) {
            fprintf(file, "\n"
                "  -- FROM -  --- TO --      TRANSMISSION LINE       "
                " --------- SHUNT ADMITTANCES (MHOS) ---------   LINE\n"
                "  TAG   SEG  TAG   SEG    IMPEDANCE      LENGTH    "
                " ----- END ONE -----      ----- END TWO -----   TYPE\n"
                "  No:   No:  No:   No:         OHMS      METERS     "
                " REAL      IMAGINARY      REAL      IMAGINARY");
        }
        else if (itmp1 == 1) {
            fprintf(file, "\n"
                "  -- FROM -  --- TO --            --------"
                " ADMITTANCE MATRIX ELEMENTS (MHOS) ---------\n"
                "  TAG   SEG  TAG   SEG   ----- (ONE,ONE) ------  "
                " ----- (ONE,TWO) -----   ----- (TWO,TWO) -------\n"
                "  No:   No:  No:   No:      REAL      IMAGINARY     "
                " REAL     IMAGINARY       REAL      IMAGINARY");
        }
        
        for (int j = 0; j < ctx->netcx.nonet; j++) {
            int itmp2 = ctx->netcx.ntyp[j];
            
            if ((itmp2 / itmp1) != 1) {
                itmp3 = itmp2;
            }
            else {
                int itmp4 = ctx->netcx.iseg1[j];
                int itmp5 = ctx->netcx.iseg2[j];
                int idx4 = itmp4 - 1;
                int idx5 = itmp5 - 1;
                
                if ((itmp2 >= 2) && (ctx->netcx.x11i[j] <= 0.0)) {
                    double xx = ctx->geometry.x[idx5] - ctx->geometry.x[idx4];
                    double yy = ctx->geometry.y[idx5] - ctx->geometry.y[idx4];
                    double zz = ctx->geometry.z[idx5] - ctx->geometry.z[idx4];
                    ctx->netcx.x11i[j] = ctx->geometry.wlam * sqrt(xx*xx + yy*yy + zz*zz);
                }
                
                fprintf(file, "\n"
                    " %4d %5d %4d %5d  %11.4E %11.4E  "
                    "%11.4E %11.4E  %11.4E %11.4E  %s",
                    ctx->geometry.tag_nums[idx4], itmp4, 
                    ctx->geometry.tag_nums[idx5], itmp5,
                    ctx->netcx.x11r[j], ctx->netcx.x11i[j], 
                    ctx->netcx.x12r[j], ctx->netcx.x12i[j],
                    ctx->netcx.x22r[j], ctx->netcx.x22i[j], 
                    pnet[itmp2-1]);
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
void write_matrix_asymmetry(FILE *file, nec_context_t *ctx)
{
    // Only write if asymmetry check was performed and data exists
    if (ctx->netcx.masym == 0 || ctx->netcx.asmx == 0.0) {
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
void write_network_excitation(FILE *file, nec_context_t *ctx)
{
    if (ctx->netcx.nexc == 0 || ctx->netcx.nprint != 0) {
        return;  // No excitation data or printing suppressed
    }
    
    fprintf(file, "\n\n\n"
        "                          "
        "--------- STRUCTURE EXCITATION DATA AT NETWORK CONNECTION POINTS --------");
    
    fprintf(file, "\n"
        "  TAG   SEG       VOLTAGE (VOLTS)          CURRENT (AMPS)        "
        " IMPEDANCE (OHMS)       ADMITTANCE (MHOS)     POWER\n"
        "  No:   No:     REAL      IMAGINARY     REAL      IMAGINARY    "
        " REAL      IMAGINARY     REAL      IMAGINARY   (WATTS)");
    
    for (int i = 0; i < ctx->netcx.nexc; i++) {
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
void write_antenna_input_parameters(FILE *file, nec_context_t *ctx)
{
    if (ctx->netcx.ninp == 0) {
        return;  // No input data to write
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
    
    for (int i = 0; i < ctx->netcx.ninp; i++) {
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
void write_currents(FILE *file, nec_context_t *ctx)
{
    if (ctx->geometry.n == 0) {
        return;  // No segments to write
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
    
    for (int i = 0; i < ctx->geometry.n; i++) {
        complex double curi = ctx->crnt.cur[i] * ctx->geometry.wlam;
        double cmag = cabs(curi);
        double ph = carg(curi) * TD;  // Convert to degrees (TD = 57.29577951)
        
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
void write_power_budget(FILE *file, nec_context_t *ctx)
{
    // Only write for standard radiation pattern types
    if ((ctx->fpat.ixtyp != 0) && (ctx->fpat.ixtyp != 5)) {
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
void write_radiation_pattern_header(FILE *file, nec_context_t *ctx)
{
    char *igtp[2] = { "----- POWER GAINS ----- ", "--- DIRECTIVE GAINS ---" };
    char *igax[4] = { " MAJOR", " MINOR", " VERTC", " HORIZ" };
    
    // Check if radiation pattern was calculated
    if (ctx->rpat.num_points == 0 || ctx->rpat.points == NULL) {
        return;
    }
    
    /* Write ground parameters if applicable */
    if (ctx->gnd.ifar > 1) {
        fprintf(file, "\n\n\n"
            "                               "
            "------ FAR FIELD GROUND PARAMETERS ------\n\n");
        
        if (ctx->gnd.ifar > 3) {
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
        
        if (ctx->gnd.ifar != 4 && strlen(ctx->rpat.ground_cliff_type) > 0) {
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
    if (ctx->gnd.ifar == 1) {
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
    else {
        int itmp1 = 2 * ctx->fpat.iax;
        int itmp2 = itmp1 + 1;
        
        fprintf(file, "\n\n\n"
            "                             "
            "---------- RADIATION PATTERNS -----------\n");
        
        if (ctx->fpat.rfld >= 1.0e-20) {
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
void write_radiation_pattern_data(FILE *file, nec_context_t *ctx)
{
    char *hpol[3] = { "LINEAR", "RIGHT ", "LEFT  " };
    double tmp5, tmp6;
    
    if (ctx->rpat.num_points == 0 || ctx->rpat.points == NULL) {
        return;
    }
    
    /* Write data for each point */
    for (int i = 0; i < ctx->rpat.num_points; i++) {
        rpat_point_t *pt = &ctx->rpat.points[i];
        
        if (ctx->gnd.ifar == 1) {
            /* Near field output */
            fprintf(file, "\n"
                " %9.2f %7.2f %9.2f  %11.4E %7.2f  %11.4E %7.2f  %11.4E %7.2f",
                ctx->fpat.rfld, pt->phi, pt->theta,
                pt->ethm, pt->etha, pt->ephm, pt->epha, pt->erdm, pt->erda);
        }
        else {
            /* Far field output */
            if (ctx->fpat.iax != 1) {
                tmp5 = pt->gnmj;
                tmp6 = pt->gnmn;
            }
            else {
                tmp5 = pt->gnv;
                tmp6 = pt->gnh;
            }
            
            fprintf(file, "\n"
                " %7.2f %9.2f  %8.2f %8.2f %8.2f %11.4f"
                " %9.2f %6s %11.4E %9.2f %11.4E %9.2f",
                pt->theta, pt->phi, tmp5, tmp6, pt->gtot, pt->axrat,
                pt->tilta, hpol[pt->pol_sense],
                pt->ethm, pt->etha, pt->ephm, pt->epha);
        }
    }
}

/******************************************************************************
 * write_average_power_gain
 * 
 * Writes the average power gain over the specified solid angle.
 */
void write_average_power_gain(FILE *file, nec_context_t *ctx)
{
    if (ctx->fpat.iavp == 0) {
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
void write_normalized_gain(FILE *file, nec_context_t *ctx)
{
    char *igntp[5] = { " MAJOR AXIS", "  MINOR AXIS",
        "    VERTICAL", "  HORIZONTAL", "       TOTAL " };
    
    if (ctx->fpat.inor == 0 || ctx->rpat.num_points == 0) {
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
    
    for (int i = 0; i < itmp3; i++) {
        rpat_point_t *pt1 = &ctx->rpat.points[i];
        double gain1;
        
        switch (ctx->fpat.inor) {
            case 1: gain1 = pt1->gnmj; break;
            case 2: gain1 = pt1->gnmn; break;
            case 3: gain1 = pt1->gnv; break;
            case 4: gain1 = pt1->gnh; break;
            case 5: gain1 = pt1->gtot; break;
            default: gain1 = pt1->gtot; break;
        }
        gain1 -= ctx->rpat.gmax;
        
        /* Check if we need fewer than 3 columns on the last row */
        if ((i + 1) == itmp3 && itmp4 != 0) {
            if (itmp4 != 2 && idx1 < ctx->rpat.num_points) {
                rpat_point_t *pt2 = &ctx->rpat.points[idx1];
                double gain2;
                switch (ctx->fpat.inor) {
                    case 1: gain2 = pt2->gnmj; break;
                    case 2: gain2 = pt2->gnmn; break;
                    case 3: gain2 = pt2->gnv; break;
                    case 4: gain2 = pt2->gnh; break;
                    case 5: gain2 = pt2->gtot; break;
                    default: gain2 = pt2->gtot; break;
                }
                gain2 -= ctx->rpat.gmax;
                fprintf(file, "\n"
                    " %9.2f %9.2f %9.2f   %9.2f %9.2f %9.2f   ",
                    pt1->theta, pt1->phi, gain1, pt2->theta, pt2->phi, gain2);
            }
            else {
                fprintf(file, "\n"
                    " %9.2f %9.2f %9.2f   ",
                    pt1->theta, pt1->phi, gain1);
            }
            break;
        }
        
        /* Print all three columns */
        if (idx1 < ctx->rpat.num_points && idx2 < ctx->rpat.num_points) {
            rpat_point_t *pt2 = &ctx->rpat.points[idx1];
            rpat_point_t *pt3 = &ctx->rpat.points[idx2];
            double gain2, gain3;
            
            switch (ctx->fpat.inor) {
                case 1: gain2 = pt2->gnmj; gain3 = pt3->gnmj; break;
                case 2: gain2 = pt2->gnmn; gain3 = pt3->gnmn; break;
                case 3: gain2 = pt2->gnv; gain3 = pt3->gnv; break;
                case 4: gain2 = pt2->gnh; gain3 = pt3->gnh; break;
                case 5: gain2 = pt2->gtot; gain3 = pt3->gtot; break;
                default: gain2 = pt2->gtot; gain3 = pt3->gtot; break;
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
 * Note: EN card is now written by write_input_cards as part of the final batch.
 */
void write_footer(FILE *file, nec_context_t *ctx, deck_t *deck)
{
    // Output blank lines before footer
    fprintf(file, "\n\n\n");
    
    // Calculate and output total runtime
    if (ctx != NULL) {
        clock_t end_time = clock();
        double elapsed = ((double)(end_time - ctx->start_time)) / CLOCKS_PER_SEC * 1000.0;  // Convert to milliseconds
        fprintf(file, "\n  TOTAL RUN TIME: %.0f msec", elapsed);
    }

  // At the very end, echo the EN card (if present) as the final data card
  // Compute the card number to assign by counting echoed control cards in the batch
  if (deck != NULL && deck->deck_end >= 0 && deck->deck_end < deck->num_cards) {
    card_t *en = &deck->cards[deck->deck_end];
    if (strncmp(en->card_code, "EN", 2) == 0) {
      int card_number = 0;
      int start = deck->geometry_end + 1;
      int end = deck->deck_end; // exclude EN from count
      for (int i = start; i < end && i < deck->num_cards; i++) {
        card_t *card = &deck->cards[i];
        // count XT (echoed) and control cards that are echoed in write_input_cards
        if (strncmp(card->card_code, "XT", 2) == 0 ||
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
          strncmp(card->card_code, "XQ", 2) == 0) {
          card_number++;
        }
      }
      // EN takes the next card number
      card_number++;
      fprintf(file, "\n  DATA CARD No: %3d %s", card_number, en->card_code);
      fprintf(file, " %3d", en->iv[1]);
      for (int j = 2; j <= 4; j++) {
        fprintf(file, " %5d", en->iv[j]);
      }
      for (int j = 1; j <= 6; j++) {
        fprintf(file, " %12.5E", en->fv[j]);
      }
      fprintf(file, "\n");
    }
  }
}

/* end of output.c */
