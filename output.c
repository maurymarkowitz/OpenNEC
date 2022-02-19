/******************************************************************************
 * output.c
 *
 * output.c contains a number of routines that write data from the deck to
 * various types of files. This includes the main output file in write_nec_out
 * which attempts to match the format of the nec2c .out files as closely as
 * possible.
 *
 * OpenNEC adds functions for writing the decks themselves, in .onec format.
 * in addition to allowing a deck to be created in code in code and then
 * written, these can also be used as a way to fix problems in existing files,
 * like split lines or non-standard comment markers and such, simply load up
 * the deck and then save it again.
 *
 *****************************************************************************/

#include "opennec.h"
#include "shared.h"
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
void write_deck_nec(Deck *deck, FILE *file, int remove_inline_comments)
{
  Card *card;
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
    if(isExtension(card)) continue;

    // for comment cards with the CM or CE *in the header*, simply export the card
    if(i <= deck->geometry_start && (strcmp(card->card_code, "CM") == 0 || strcmp(card->card_code, "CE") == 0)) {
      fprintf(file, "%s%s", deck->cards[i].card_code, deck->cards[i].comment);
      fputc('\n', file);
    }
    // for comment cards with other headers, only export if the option is on
    if(isComment(card)) {
      fprintf(file, "%s%s", deck->cards[i].card_code, deck->cards[i].comment);
      fputc('\n', file);
    }

    // for geometry and command cards, start with the code
    fputs(card->card_code, file);
    
    // get the number of fields for this sort of card
    MAX_INTS = max_int_fields(card);
    MAX_FLTS = max_flt_fields(card);

    // int and float fields
    if(isControl(card) || isGeometry(card)) {
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
 * differences in ordering of options, spacing and separators being stripped,
 * etc. This is by design.
 *
 */
void write_deck_onec(Deck *deck, FILE *file)
{
  Card *card;
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
    if(isComment(card)) {
      fputs(card->card_code, file);
      fputs(card->comment, file);
      fputc('\n', file);
      continue;
    }
    
    // the ONEC cards like SY are also generally simple
    if(isExtension(card)) {
      if(strcmp(card->extn_code, "") != 0) {
        fputs(card->extn_code, file);
      }
      fputs(card->card_code, file);
      
      KeyValue *head = card->formulas;
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
    if(isControl(card) || isGeometry(card)) {
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
          KeyValue *form = card->formulas;
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
          KeyValue *pair = card->extensns;
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
void write_nec_output(Deck *deck, FILE *file)
{
  write_header(deck, file);
  write_structure(deck, file);
}


/******************************************************************************
 * write_headers()
 *
 * writes the header area and comments to the standard NEC output file
 *
 */
void write_header(Deck *deck, FILE *file)
{
  fprintf( output_fp,  "\n\n\n"
          "                              "
          " __________________________________________\n"
          "                              "
          "|                                          |\n"
          "                              "
          "|  NUMERICAL ELECTROMAGNETICS CODE (nec2c) |\n"
          "                              "
          "|   Translated to 'C' in Double Precision  |\n"
          "                              "
          "|__________________________________________|\n" );
  
  fprintf( output_fp, "\n\n\n"
          "                               "
          "---------------- COMMENTS ----------------\n" );
  
  // write comments to output file
  for(int i = deck->comment_start; i <= deck->comment_end; i++) {
    fprintf( output_fp,
            "               %s\n",
            deck->cards[i].comment);
  }
}



/******************************************************************************
 * write_structure()
 *
 * Writes the structure section of the nec2 output, which is based on
 * the input geometry cards
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
 * have slots for the tag and the segment numbers they span. This has the
 * advantage of also allowing the segments to be associated with the card
 * that made them at any time, so that extensions can be looked up, etc.
 *
 *
 */
void write_structure(Deck *deck, FILE *file)
{
  Card card;
  int geo_card_num;
  int num_wires = 0;
  int num_patches = 0;

  int ix, iy, iz;
  
  // these are used to match various codes in the cards to text output
  char ifx[2] = { '*', 'X' }, ify[2] = { '*','Y' }, ifz[2] = { '*','Z' }; // reflection axes
  char ipt[4] = { 'P', 'R', 'T', 'Q' };
  
  // print the header
  fprintf(file, "\n\n\n"
          "                               "
          "-------- STRUCTURE SPECIFICATION ---------\n"
          "                                     "
          "COORDINATES MUST BE INPUT IN\n"
          "                                     "
          "METERS OR BE SCALED TO METERS\n"
          "                                     "
          "BEFORE STRUCTURE INPUT IS ENDED\n");
  
  fprintf(output_fp, "\n"
          "  WIRE                                           "
          "                                      SEG FIRST  LAST  TAG\n"
          "   NO.        X1         Y1         Z1         X2      "
          "   Y2         Z2       RADIUS   NO. SEG.   SEG.  NO.");
  
  for(int i = deck->geometry_start; i <= deck->geometry_end; i++) {
    card = deck->cards[i];
    
    // convert the card code to a number
    for(geo_card_num = 0; geo_card_num < NUM_GEOMETRY_CODES; geo_card_num++) {
      if( strncmp(deck->cards[i].card_code, geometry_codes[geo_card_num], 2) == 0)
        break;
    }
    
    // switch on the number
    switch(geo_card_num) {
        
      case 0: // GW card, a wire
        num_wires++;
        fprintf(output_fp, "\n"
          " %5d  %10.5f %10.5f %10.5f %10.5f"
          " %10.5f %10.5f %10.5f %5d %5d %5d %4d",
                num_wires, card.f[1], card.f[2], card.f[3], card.f[4], card.f[5], card.f[6], card.f[7],
                card.tag, card.start_segment, card.end_segment, card.tag);
        break;
        
      case 1: // GX card, reflection or rotation
        // decode the flags stored in the I2 value on the card
        iy = card.i[2] / 10;
        iz = card.i[2] - iy*10;
        ix = iy / 10;
        iy = iy - ix*10;

        if(ix != 0)
          ix = 1;
        if(iy != 0)
          iy = 1;
        if(iz != 0)
          iz = 1;
        
        fprintf(output_fp,
                "\n      STRUCTURE REFLECTED ALONG THE AXES %c %c %c"
                " - TAGS INCREMENTED BY %d",
                ifx[ix], ify[iy], ifz[iz], card.i[1] );
        break;
        
      case 3: // GS card, scale structure dimensions
        fprintf(output_fp,
                "\n     STRUCTURE SCALED BY FACTOR: %10.5f", card.f[1] );
        break;
        
      case 4: // GE card, nothing to do
        break;
        
      case 5: // GM card, move/copy existing structure
        fprintf(output_fp,
                "\n     THE STRUCTURE HAS BEEN MOVED, MOVE DATA CARD IS:\n"
                "   %3d %5d %10.5f %10.5f %10.5f %10.5f %10.5f %10.5f %10.5f",
                card.i[1], card.i[2], card.f[1], card.f[2], card.f[3], card.f[4], card.f[5], card.f[6], card.f[7]);
        break;
        
      case 6: // SP card, generate single surface patch
        num_patches++;
        fprintf( output_fp, "\n"
                " %5d%c %10.5f %10.5f %10.5f %10.5f %10.5f %10.5f",
                card.i[1], card.i[i-1], card.f[1], card.f[2], card.f[3], card.f[4], card.f[5], card.f[6]);
        break;
        
      case 7: // SM card, multiple-patch surface
//        i1= data.m+1;
//        fprintf( output_fp, "\n"
//                " %5d%c %10.5f %11.5f %11.5f %11.5f %11.5f %11.5f"
//                "     SURFACE - %d BY %d PATCHES",
//                i1, ipt[1], xw1, yw1, zw1, xw2, yw2, zw2, itg, ns );
//
        
        break;
        
      case 8: // GA card, wire arc
        num_wires++;
        fprintf( output_fp, "\n"
                " %5d ARC RADIUS: %9.5f  FROM: %8.3f TO: %8.3f DEGREES"
                "       %11.5f %5d %5d %5d %4d",
                num_wires, card.f[1], card.f[2], card.f[3], card.f[4], card.tag, card.start_segment, card.end_segment, card.i[1]);
        break;
        
      case 9: // SC card
        
        break;
        
      case 10: // GH card, generate helix */
        num_wires++;
        fprintf( output_fp, "\n"
                " %5d HELIX STRUCTURE - SPACING OF TURNS: %8.3f AXIAL"
                " LENGTH: %8.3f  %8.3f %5d %5d %5d %4d\n      "
                " RADIUS X1:%8.3f Y1:%8.3f X2:%8.3f Y2:%8.3f ",
                num_wires, card.f[1], card.f[2], card.f[7], card.tag, card.start_segment, card.end_segment,
                card.i[1], card.f[3], card.f[4], card.f[6], card.f[6]);
        break;
        
    } /* switch on the card type */
  } /* for loop over cards */
  
  // and now a final report on the cards
  fprintf( output_fp, "\n\n"
          "     TOTAL SEGMENTS USED: %d   SEGMENTS IN A"
          " SYMMETRIC CELL: %d   SYMMETRY FLAG: %d",
          geometry.n, geometry.np, geometry.ipsym );
  
  if(geometry.m > 0)
    fprintf( output_fp,  "\n"
            "       TOTAL PATCHES USED: %d   PATCHES"
            " IN A SYMMETRIC CELL: %d",  geometry.m, geometry.mp );
  
  int iseg = (geometry.n + geometry.m) / (geometry.np + geometry.mp);
  if(iseg != 1)  {
    /*** may be error condition?? ***/
    if(geometry.ipsym == 0) {
      fprintf( output_fp,
              "\n  ERROR: IPSYM=0 IN CONECT()" );
      stop(-1);
    }
  } /* if( iseg == 1) */

  if(geometry.ipsym < 0)
    fprintf( output_fp,
            "\n  STRUCTURE HAS %d FOLD ROTATIONAL SYMMETRY\n", iseg );
  else {
    int ic = iseg / 2;
    if(iseg == 8)
      ic = 3;
    fprintf( output_fp,
            "\n  STRUCTURE HAS %d PLANES OF SYMMETRY\n", ic );
  } /* if( data.ipsym < 0 ) */


}

/******************************************************************************
 * write_segments()
 *
 * writes the segment data section of the nec2 output.
 *
 */
void write_segments(Deck * deck, FILE *file)
{
  // exit now if there's no segments
  if(geometry.n == 0) return;

      fprintf(output_fp, "\n\n\n"
              "                              "
              " ---------- SEGMENTATION DATA ----------\n"
              "                                       "
              " COORDINATES IN METERS\n"
              "                           "
              " I+ AND I- INDICATE THE SEGMENTS BEFORE AND AFTER I\n");
  
      fprintf(output_fp, "\n"
              "   SEG    COORDINATES OF SEGM CENTER     SEGM    ORIENTATION"
              " ANGLES    WIRE    CONNECTION DATA   TAG\n"
              "   No:       X         Y         Z      LENGTH     ALPHA     "
              " BETA    RADIUS    I-     I    I+   No:");
  
  double xw1, yw1, zw1;
  double xw2, yw2;
  
  for(int i = 0; i < geometry.n; i++) {
    xw1 = geometry.x2[i] - geometry.x1[i];
    yw1 = geometry.y2[i] - geometry.y1[i];
    zw1 = geometry.z2[i] - geometry.z1[i];
    geometry.x[i] = (geometry.x1[i] + geometry.x2[i]) / 2.0;
    geometry.y[i] = (geometry.y1[i] + geometry.y2[i]) / 2.0;
    geometry.z[i] = (geometry.z1[i] + geometry.z2[i]) / 2.0;
    xw2 = xw1* xw1 + yw1* yw1 + zw1* zw1;
    yw2 = sqrt(xw2);
    yw2 = (xw2 / yw2 + yw2)*.5;
    geometry.si[i] = yw2;
    geometry.cab[i] = xw1 / yw2;
    geometry.sab[i] = yw1 / yw2;
    xw2 = zw1 / yw2;
    
    if(xw2 > 1.0)
      xw2 = 1.0;
    if(xw2 < -1.0)
      xw2 = -1.0;
    
    geometry.salp[i] = xw2;
    xw2 = asin(xw2) * TD;
    yw2 = atan2(yw1, xw1) * TD;
    
    fprintf(output_fp, "\n"
            " %5d %9.4f %9.4f %9.4f %9.4f"
            " %9.4f %9.4f %9.4f %5d %5d %5d %5d",
            i + 1, geometry.x[i], geometry.y[i], geometry.z[i], geometry.si[i], xw2, yw2,
            geometry.bi[i], geometry.icon1[i], i + 1, geometry.icon2[i], geometry.tag_nums[i]);
    
    if(plot.iplp1 == 1)
      fprintf(plot_fp, "%12.4E %12.4E %12.4E "
              "%12.4E %12.4E %12.4E %12.4E %5d %5d %5d\n",
              geometry.x[i], geometry.y[i], geometry.z[i], geometry.si[i], xw2, yw2,
              geometry.bi[i], geometry.icon1[i], i + 1, geometry.icon2[i]);
    
    if((geometry.si[i] <= 1.e-20) || (geometry.bi[i] <= 0.0)) {
      fprintf(output_fp, "\n SEGMENT DATA ERROR");
      stop(-1);
    }
    
  } /* for( i = 0; i < data.n; i++ ) */
} /* write_segments */

/******************************************************************************
 * write_patches()
 *
 * writes the patch data section of the nec2 output.
 *
 */
void write_patches(Deck * deck, FILE *file)
{
  // exit now if there's no patches
  if (geometry.m == 0) return;
  
    fprintf(output_fp, "\n\n\n"
            "                                   "
            " --------- SURFACE PATCH DATA ---------\n"
            "                                            "
            " COORDINATES IN METERS\n\n"
            " PATCH      COORD. OF PATCH CENTER           UNIT NORMAL VECTOR      "
            " PATCH           COMPONENTS OF UNIT TANGENT VECTORS\n"
            "  No:       X          Y          Z          X        Y        Z      "
            " AREA         X1       Y1       Z1        X2       Y2      Z2");
  
  double xw1, yw1, zw1;
  for(int i = 0; i < geometry.m; i++) {
    xw1 = (geometry.t1y[i] * geometry.t2z[i] - geometry.t1z[i] * geometry.t2y[i])* geometry.psalp[i];
    yw1 = (geometry.t1z[i] * geometry.t2x[i] - geometry.t1x[i] * geometry.t2z[i])* geometry.psalp[i];
    zw1 = (geometry.t1x[i] * geometry.t2y[i] - geometry.t1y[i] * geometry.t2x[i])* geometry.psalp[i];
    
    fprintf(output_fp, "\n"
            " %4d %10.5f %10.5f %10.5f  %8.4f %8.4f %8.4f"
            " %10.5f  %8.4f %8.4f %8.4f  %8.4f %8.4f %8.4f",
            i + 1, geometry.px[i], geometry.py[i], geometry.pz[i], xw1, yw1, zw1, geometry.pbi[i],
            geometry.t1x[i], geometry.t1y[i], geometry.t1z[i], geometry.t2x[i], geometry.t2y[i], geometry.t2z[i]);
  } /* for( i = 0; i < data.m; i++ ) */
}
