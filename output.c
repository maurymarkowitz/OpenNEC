/******************************************************************************
 * output.c
 *
 * output.c contains a number of routines that write data from the
 * deck to various types of files. This includes the main output
 * file in write_nec_out(), which attempts to match the format of
 * the nec2c .out files as closely as possible.
 *
 * OpenNEC adds functions for writing the decks themselves, in
 * .nec or .onec formats. In addition to allowing a deck to be
 * created in code and then written, these can also be used as a
 * way to fix problems in existing files, like split lines or
 * non-standard comment markers and such, simply load up the
 * deck and then save it again.
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
 */
void write_deck_nec(Deck *deck, FILE *file, int remove_inline_comments)
{
  Card *card;
  
  for(int i = 0; i < deck->num_cards; i++) {
    card = &deck->cards[i];
    
    // for comment cards with the CM or CE *in the header*, simply export the card
    if(i <= deck->geometry_start && (strcmp(card->card_code, "CM") == 0 || strcmp(card->card_code, "CE") == 0)) {
      fprintf(file, "%s%s", deck->cards[i].card_code, deck->cards[i].comment);
    }
    // for comment cards with other headers, only export if the option is on
    if(isComment(card)) {
      fprintf(file, "%s%s", deck->cards[i].card_code, deck->cards[i].comment);
    }
    // for geometry,

    
  }
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

    // start with the easy case, basic comment cards
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
      for(int j = 0; j <= card->ints_used && j <= MAX_INTS; j++) {
        fprintf(file, " %d", card->i[j]);
      }
      for(int j = 0; j <= card->flts_used && j <= MAX_FLTS; j++) {
        fprintf(file, " %G", card->f[j]);
      }

      // the basic NEC fields are output, now see if there's anything after that
      bool hasComment = (card->comment != NULL && strlen(card->comment) > 0);
      
      bool hasOnec = false;
      if(card->name != NULL && strlen(card->name) > 0) hasOnec = true;
      if(card->group != NULL && strlen(card->group) > 0) hasOnec = true;
      if(card->ignore) hasOnec = true;
      if(card->invisible) hasOnec = true;
      if(card->pairs != NULL ) hasOnec = true;
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
        // in the field-by-field case, start with the known extensions
        if(card->name != NULL && strlen(card->name) > 0) {
          fputs(" name:", file);
          fputs(card->name, file);
        }
        if(card->group != NULL && strlen(card->group) > 0) {
          fputs(" group:", file);
          fputs(card->group, file);
        }
        if(card->ignore) {
          fputs(" ignore:true", file);
        }
        if(card->invisible) {
          fputs(" invisible:true", file);
        }
        // formulas next
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
        if(card->pairs != NULL) {
          KeyValue *pair = card->pairs;
          while(pair != NULL) {
            fputc(' ', file);
            fputs(pair->key, file);
            fputc('=', file);
            fputs(pair->value, file);
            pair = pair->next;
          }
        }
        // and then finally the comment which has to be at the end
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

/*----------------------------------------------------------------------*/
/* write_structure()
 *
 * writes the structure section of the nec2 output, which is based on
 * the input geometry cards
 *
 */

void write_structure(Deck *deck, FILE *file)
{
 // these are used to match various codes in the cards to text output
  char ifx[2] = { '*', 'X' }, ify[2] = { '*','Y' }, ifz[2] = { '*','Z' };
  char ipt[4] = { 'P', 'R', 'T', 'Q' };

  fprintf(file, "\n\n\n"
  "-------- STRUCTURE SPECIFICATION --------\n"
  "                                     "
  "COORDINATES MUST BE INPUT IN\n"
  "                                     "
  "METERS OR BE SCALED TO METERS\n"
  "                                     "
  "BEFORE STRUCTURE INPUT IS ENDED\n");
  
  fprintf(output_fp, "\n"
          "  WIRE                                           "
          "                                      SEG FIRST  LAST  TAG\n"
          "   No:        X1         Y1         Z1         X2      "
          "   Y2         Z2       RADIUS   No:   SEG   SEG  No:");
  
}

/***---------------------------------------------------------------------
 * write_segments()
 *
 * writes the segment data section of the nec2 output.
 *
 */
void write_segments(Deck * deck, FILE *file)
{
  // exit now if there's no segments
  if(data.n == 0) return;

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

      for(int i = 0; i < data.n; i++) {
        xw1 = data.x2[i] - data.x1[i];
        yw1 = data.y2[i] - data.y1[i];
        zw1 = data.z2[i] - data.z1[i];
        data.x[i] = (data.x1[i] + data.x2[i]) / 2.;
        data.y[i] = (data.y1[i] + data.y2[i]) / 2.;
        data.z[i] = (data.z1[i] + data.z2[i]) / 2.;
        xw2 = xw1* xw1 + yw1* yw1 + zw1* zw1;
        yw2 = sqrt(xw2);
        yw2 = (xw2 / yw2 + yw2)*.5;
        data.si[i] = yw2;
        data.cab[i] = xw1 / yw2;
        data.sab[i] = yw1 / yw2;
        xw2 = zw1 / yw2;
  
        if(xw2 > 1.)
          xw2 = 1.;
        if(xw2 < -1.)
          xw2 = -1.;
  
        data.salp[i] = xw2;
        xw2 = asin(xw2)* TD;
        yw2 = atan2(yw1, xw1)* TD;
  
        fprintf(output_fp, "\n"
                " %5d %9.4f %9.4f %9.4f %9.4f"
                " %9.4f %9.4f %9.4f %5d %5d %5d %5d",
                i + 1, data.x[i], data.y[i], data.z[i], data.si[i], xw2, yw2,
                data.bi[i], data.icon1[i], i + 1, data.icon2[i], data.itag[i]);
  
        if(plot.iplp1 == 1)
          fprintf(plot_fp, "%12.4E %12.4E %12.4E "
                  "%12.4E %12.4E %12.4E %12.4E %5d %5d %5d\n",
                  data.x[i], data.y[i], data.z[i], data.si[i], xw2, yw2,
                  data.bi[i], data.icon1[i], i + 1, data.icon2[i]);
  
        if((data.si[i] <= 1.e-20) || (data.bi[i] <= 0.)) {
          fprintf(output_fp, "\n SEGMENT DATA ERROR");
          stop(-1);
        }
  
      } /* for( i = 0; i < data.n; i++ ) */
} /* write_segments */

/*----------------------------------------------------------------------*/
/* write_patches()
 *
 * writes the patch data section of the nec2 output.
 *
 */
void write_patches(Deck * deck, FILE *file)
{
  // exit now if there's no patches
  if (data.m == 0) return;
  
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
    for(int i = 0; i < data.m; i++) {
      xw1 = (data.t1y[i] * data.t2z[i] - data.t1z[i] * data.t2y[i])* data.psalp[i];
      yw1 = (data.t1z[i] * data.t2x[i] - data.t1x[i] * data.t2z[i])* data.psalp[i];
      zw1 = (data.t1x[i] * data.t2y[i] - data.t1y[i] * data.t2x[i])* data.psalp[i];

      fprintf(output_fp, "\n"
              " %4d %10.5f %10.5f %10.5f  %8.4f %8.4f %8.4f"
              " %10.5f  %8.4f %8.4f %8.4f  %8.4f %8.4f %8.4f",
              i + 1, data.px[i], data.py[i], data.pz[i], xw1, yw1, zw1, data.pbi[i],
              data.t1x[i], data.t1y[i], data.t1z[i], data.t2x[i], data.t2y[i], data.t2z[i]);
    } /* for( i = 0; i < data.m; i++ ) */
}
  
  // case 0
  // output a line for a GW, and optionally a GC following
  //        fprintf(output_fp, "\n"
  //                " %5d  %10.5f %10.5f %10.5f %10.5f"
  //                " %10.5f %10.5f %10.5f %5d %5d %5d %4d",
  //                nwire, xw1, yw1, zw1, xw2, yw2, zw2, rad, ns, i1, i2, itg);
  //if wire radius is zero...
  //
  //  fprintf(output_fp,
  //          "\n  ABOVE WIRE IS TAPERED.  SEGMENT LENGTH RATIO: %9.5f\n"
  //          "                                 "
  //          "RADIUS FROM: %9.5f TO: %9.5f", xs1, ys1, zs1);

  
  //case 1
//  fprintf(output_fp,
//          "\n      STRUCTURE REFLECTED ALONG THE AXES %c %c %c"
//          " - TAGS INCREMENTED BY %d",
//          ifx[ix], ify[iy], ifz[iz], itg);
  
  //case 2
//  fprintf(output_fp,
//          "\n  STRUCTURE ROTATED ABOUT Z-AXIS %d TIMES"
//          " - LABELS INCREMENTED BY %d", ns, itg);

// case 3
//  fprintf(output_fp,
//          "\n     STRUCTURE SCALED BY FACTOR: %10.5f", xw1);

  
  //case 4
//  if(ns != 0) {
//    plot.iplp1 = 1;
//    plot.iplp2 = 1;
//  }
  // print out the wire segment data if there is any
//  if(data.n != 0) {
//    /* Allocate wire buffers */
//    mreq = (size_t)data.n;
//    mreq *= sizeof(double);
//    mem_realloc((void *)&data.si, mreq);
//    mem_realloc((void *)&data.sab, mreq);
//    mem_realloc((void *)&data.cab, mreq);
//    mem_realloc((void *)&data.salp, mreq);
//    mem_realloc((void *)&data.x, mreq);
//    mem_realloc((void *)&data.y, mreq);
//    mem_realloc((void *)&data.z, mreq);
//    fprintf(output_fp, "\n\n\n"
//            "                              "
//            " ---------- SEGMENTATION DATA ----------\n"
//            "                                       "
//            " COORDINATES IN METERS\n"
//            "                           "
//            " I+ AND I- INDICATE THE SEGMENTS BEFORE AND AFTER I\n");
//
//    fprintf(output_fp, "\n"
//            "   SEG    COORDINATES OF SEGM CENTER     SEGM    ORIENTATION"
//            " ANGLES    WIRE    CONNECTION DATA   TAG\n"
//            "   No:       X         Y         Z      LENGTH     ALPHA     "
//            " BETA    RADIUS    I-     I    I+   No:");
//
//    for(i = 0; i < data.n; i++) {
//      xw1 = data.x2[i] - data.x1[i];
//      yw1 = data.y2[i] - data.y1[i];
//      zw1 = data.z2[i] - data.z1[i];
//      data.x[i] = (data.x1[i] + data.x2[i]) / 2.;
//      data.y[i] = (data.y1[i] + data.y2[i]) / 2.;
//      data.z[i] = (data.z1[i] + data.z2[i]) / 2.;
//      xw2 = xw1* xw1 + yw1* yw1 + zw1* zw1;
//      yw2 = sqrt(xw2);
//      yw2 = (xw2 / yw2 + yw2)*.5;
//      data.si[i] = yw2;
//      data.cab[i] = xw1 / yw2;
//      data.sab[i] = yw1 / yw2;
//      xw2 = zw1 / yw2;
//
//      if(xw2 > 1.)
//        xw2 = 1.;
//      if(xw2 < -1.)
//        xw2 = -1.;
//
//      data.salp[i] = xw2;
//      xw2 = asin(xw2)* TD;
//      yw2 = atan2(yw1, xw1)* TD;
//
//      fprintf(output_fp, "\n"
//              " %5d %9.4f %9.4f %9.4f %9.4f"
//              " %9.4f %9.4f %9.4f %5d %5d %5d %5d",
//              i + 1, data.x[i], data.y[i], data.z[i], data.si[i], xw2, yw2,
//              data.bi[i], data.icon1[i], i + 1, data.icon2[i], data.itag[i]);
//
//      if(plot.iplp1 == 1)
//        fprintf(plot_fp, "%12.4E %12.4E %12.4E "
//                "%12.4E %12.4E %12.4E %12.4E %5d %5d %5d\n",
//                data.x[i], data.y[i], data.z[i], data.si[i], xw2, yw2,
//                data.bi[i], data.icon1[i], i + 1, data.icon2[i]);
//
//      if((data.si[i] <= 1.e-20) || (data.bi[i] <= 0.)) {
//        fprintf(output_fp, "\n SEGMENT DATA ERROR");
//        stop(-1);
//      }
//
//    } /* for( i = 0; i < data.n; i++ ) */
//  } /* if( data.n != 0) */
//  // print out the patch data if there is any
//  if (data.m != 0) {
//    fprintf(output_fp, "\n\n\n"
//            "                                   "
//            " --------- SURFACE PATCH DATA ---------\n"
//            "                                            "
//            " COORDINATES IN METERS\n\n"
//            " PATCH      COORD. OF PATCH CENTER           UNIT NORMAL VECTOR      "
//            " PATCH           COMPONENTS OF UNIT TANGENT VECTORS\n"
//            "  No:       X          Y          Z          X        Y        Z      "
//            " AREA         X1       Y1       Z1        X2       Y2      Z2");
//
//    for(i = 0; i < data.m; i++) {
//      xw1 = (data.t1y[i] * data.t2z[i] - data.t1z[i] * data.t2y[i])* data.psalp[i];
//      yw1 = (data.t1z[i] * data.t2x[i] - data.t1x[i] * data.t2z[i])* data.psalp[i];
//      zw1 = (data.t1x[i] * data.t2y[i] - data.t1y[i] * data.t2x[i])* data.psalp[i];
//
//      fprintf(output_fp, "\n"
//              " %4d %10.5f %10.5f %10.5f  %8.4f %8.4f %8.4f"
//              " %10.5f  %8.4f %8.4f %8.4f  %8.4f %8.4f %8.4f",
//              i + 1, data.px[i], data.py[i], data.pz[i], xw1, yw1, zw1, data.pbi[i],
//              data.t1x[i], data.t1y[i], data.t1z[i], data.t2x[i], data.t2y[i], data.t2z[i]);
//    } /* for( i = 0; i < data.m; i++ ) */
//  } /* if( data.m == 0) */
  
  // case 5
//  fprintf(output_fp,
//          "\n     THE STRUCTURE HAS BEEN MOVED, MOVE DATA CARD IS:\n"
//          "   %3d %5d %10.5f %10.5f %10.5f %10.5f %10.5f %10.5f %10.5f",
//          itg, ns, xw1, yw1, zw1, xw2, yw2, zw2, rad);
  
//case 6
  // FIXME: our new code reads all the following SC's as well, we need to do that here
//  fprintf(output_fp, "\n"
//          " %5d%c %10.5f %10.5f %10.5f %10.5f %10.5f %10.5f",
//          i1, ipt[ns - 1], xw1, yw1, zw1, xw2, yw2, zw2);

//  case7
//  fprintf(output_fp, "\n"
//          " %5d%c %10.5f %11.5f %11.5f %11.5f %11.5f %11.5f"
//          "     SURFACE - %d BY %d PATCHES",
//          i1, ipt[1], xw1, yw1, zw1, xw2, yw2, zw2, itg, ns);
//
//
  
  //case 8
//  fprintf(output_fp, "\n"
//          " %5d ARC RADIUS: %9.5f  FROM: %8.3f TO: %8.3f DEGREES"
//          "       %11.5f %5d %5d %5d %4d",
//          nwire, xw1, yw1, zw1, xw2, ns, i1, i2, itg);
  

