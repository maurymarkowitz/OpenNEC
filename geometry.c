/*******************************************************************
 * geometry.c
 *
 * geometry.c contains the code that parses the geometry section of
 * the deck and then generates a list of segments, patches, and
 * connections. These are collected into the /data/ structure for
 * the deck.
 *
*******************************************************************/

#include "opennec.h"
#include "shared.h"

/******************************************************************************
 * calculate_geometry
 *
 * calculate_geometry (formerly datagn) is the main routine for creation
 * of geometry data. it reads the geometry cards, prints out various messages
 * and when it reaches the end of the geometry it calls the various worker
 * functions to calculate the segment and patch data. the resulting
 * data, in /data/, can then be used to draw a diagram of the antenna.
 *
 * @param deck Deck structure that will hold the Cards
 * @param errors a list of errors to add to
 *
 */
void calculate_geometry(Deck *deck, Errors *errors)
{
  Card card;
  char *msg = calloc(1, MAX_ERROR_LEN);

	//char gm[3];
  int gm_num; /* geometry card id as a number */
	int num_wires, isct, iphd, i1, i2, itg, iy, iz;
	size_t mreq;
	int ix, i, ns;
	double rad, xs1, xs2, ys1, ys2, zs1, zs2, x4 = 0, y4 = 0, z4 = 0;
	double x3 = 0, y3 = 0, z3 = 0, xw1, xw2, yw1, yw2, zw1, zw2;
	//double dummy;

  // set up the counters
	data.ipsym = 0;
	num_wires = 0;
	data.n = 0;
	data.np = 0;
	data.m = 0;
	data.mp = 0;
	isct = 0;
	iphd = FALSE;	// is this "i printed header"?
  
  // make sure there's cards to process
  // TODO: should this be an error/warning?
  if(deck->num_cards == 0) return;
  if(deck->geometry_start == 0 || deck->geometry_end == 0) return;
  
  // make sure all the formula-based values are up to date
  update_deck_values(deck);
  
  // loop over the geometry section and do the magic...
  for(int i = deck->geometry_start; i < deck->geometry_end; i++) {
    // cache the card
    card = deck->cards[i];
    
    // convert the code into its numeric value so we can switch on it
    //
    // NOTE: not all cards are in the list - continuations like GC are
    //       left off so that they will fall off the end of the switch.
    //       that's because they get read and processed by the code
    //       when working on the card above. exceptions, of course.
    //       in this case the SC cards, where you can have multiple
    //       patches in a list
    for(gm_num = 0; gm_num < NUM_GEOMETRY_CODES; gm_num++) {
      if(strncmp(card.card_code, geometry_codes[gm_num], 2) == 0) break;
    }
    // now read in the values, which are the same for all the cards
    //parse_geometry_card(gm, &itg, &ns, &xw1, &yw1, &zw1, &xw2, &yw2, &zw2, &rad);
    itg = card.i1;
    ns = card.i2;
    xw1 = card.f1;
    yw1 = card.f2;
    zw1 = card.f3;
    xw2 = card.f4;
    yw2 = card.f5;
    zw2 = card.f6;

    // and now the switch. basically all this does is call the appropriate
    // function to insert the segments for that card type
    switch(gm_num) {
        
      case 0: // GW, make a wire
        // the radius could be in the f7 field, or it could be on the next card if its tapered
        if(card.f7 != 0.0) {
          rad = card.f7;
          xs1 = 1.0;
          ys1 = 1.0;
        } else {
          // make sure the next card is a GC, although we should have already done that
          if(strcmp(deck->cards[i + 1].card_code, "GC") != 0) {
            sprintf(msg, "Card %d is a GW with a zero radius, but the next card is not a GC with the tapering info.", i);
            add_error(errors, msg, 1);
            continue;
          }
          // and also that the values in it are valid
          if(deck->cards[i + 1].f2 == 0.0 || deck->cards[i + 1].f3 == 0.0) {
            sprintf(msg, "Card %d is a GC with tapering info for GW in card %d, but there is a zero in Y1 or Z1.", i + 1, i);
            add_error(errors, msg, 1);
            continue;
          }
          // override the original inputs with the ones from the GC
          //read_geometry_card(gm, &ix, &iy, &xs1, &ys1, &zs1, &dummy, &dummy, &dummy, &dummy);
          xs1 = deck->cards[i + 1].f1;  // check this!
          ys1 = deck->cards[i + 1].f2;
          zs1 = deck->cards[i + 1].f3;
          rad = ys1;
          ys1 = pow((zs1 / ys1), (1. / (ns - 1.)));
          
          // move up a card so we don't process it again
          i++;
        }
        
        // update the number of wires and the segment counts
        num_wires++;
        i1 = data.n + 1;
        i2 = data.n + ns;
        
        // now we have all the data, so turn it into segments
        wire(xw1, yw1, zw1, xw2, yw2, zw2, rad, xs1, ys1, ns, itg);
        continue;
        
      case 1: /* "gx" card, reflect structure along x, y, or z axes or rotate to form cylinder.  */
        // the gx puts a single integer value in the I2 slot, and then uses its three digits
        // as bit flags for the x, y and z axes.
        iy = ns / 10;
        iz = ns - iy * 10;
        ix = iy / 10;
        iy = iy - ix * 10;
        
        if(ix != 0) ix = 1;
        if(iy != 0) iy = 1;
        if(iz != 0) iz = 1;
        
        reflect(ix, iy, iz, itg, ns);
        continue;
        
      case 2: /* "gr" card, rotate the structure */
        // I2 is the number of times to duplicate the structure as it rotates
        ix = -1;
        iz = 0;
        iy = 0;
        
        reflect(ix, iy, iz, itg, ns);
        continue;

      case 3: /* "gs" card, scale structure dimensions by factor xw1. */
        scale(xw1);
        continue;
        
      case 4: /* "ge" card, finish off the segments and patches, and calculate everything. */
        // FIXME: it's not clear what this is testing, on a GE card there shouldn't be an ns input
        //  perhaps it is  clearing out the ns from the previous line? but why bother when it's
        //  about to return anyway?
        if(ns != 0) {
          plot.iplp1 = 1;
          plot.iplp2 = 1;
        }
        
        // if we're at the end of the geometry section, we have all the segments
        // so now is an opportune time to connect them together
        // TODO: it seems we could clarify the code by putting the itg in a new function, calculate_ground?
        connect_segments(itg);
        
        // TODO: put the following code into update_segments and update_patches
        
        // and now we calculate various geometry-related data for wires,
        // like the centerpoints and orientation
        if(data.n != 0) {
          /* reallocate the buffers */
          mreq = (size_t)data.n * sizeof(double);
          mem_realloc((void *)&data.si, mreq);
          mem_realloc((void *)&data.sab, mreq);
          mem_realloc((void *)&data.cab, mreq);
          mem_realloc((void *)&data.salp, mreq);
          mem_realloc((void *)&data.x, mreq);
          mem_realloc((void *)&data.y, mreq);
          mem_realloc((void *)&data.z, mreq);
          
          // and run the calcs
          for(i = 0; i < data.n; i++) {
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
            
            if(xw2 > 1.) xw2 = 1.;
            if(xw2 < -1.) xw2 = -1.;
            
            data.salp[i] = xw2;
            xw2 = asin(xw2)* TD;
            yw2 = atan2(yw1, xw1)* TD;
            
            if(data.si[i] <= 1.e-20) {
                sprintf(msg, "The length of segment %d is too small to process.", i);
                add_error(errors, msg, 1);
            }
            if(data.bi[i] <= 0.) {
              sprintf(msg, "The radius of segment %d is too small to process.", i);
              add_error(errors, msg, 1);
            }
          } /* for( i = 0; i < data.n; i++ ) */
        } /* if( data.n != 0) */
        
        // and finally, do the same basic calculations for the patches
        // print out the patch data if there is any
        if (data.m != 0) {
          // don't really need the if, because nothing seems to be realloced
          for(i = 0; i < data.m; i++) {
            xw1 = (data.t1y[i] * data.t2z[i] - data.t1z[i] * data.t2y[i])* data.psalp[i];
            yw1 = (data.t1z[i] * data.t2x[i] - data.t1x[i] * data.t2z[i])* data.psalp[i];
            zw1 = (data.t1x[i] * data.t2y[i] - data.t1y[i] * data.t2x[i])* data.psalp[i];
          } /* for( i = 0; i < data.m; i++ ) */
        } /* if( data.m == 0) */
        
        // update the counters that track the total number of segments and patches
        data.npm = data.n + data.m;
        data.np2m = data.n + 2 * data.m;
        data.np3m = data.n + 3 * data.m;
        // and in this case, we're done
        return;
        
      case 5: /* "gm" card, move structure or reproduce/duplicate original structure in new positions */
        xw1 = xw1 * TA;
        yw1 = yw1 * TA;
        zw1 = zw1 * TA;
        
        // convert the original float value in F7 to int
        int its = (int)(card.f7 + .5);
        
        duplicate(xw1, yw1, zw1, xw2, yw2, zw2, its, ns, itg);
        continue;
        
      case 6: /* "sp" card, generate single new patch or a series of patches with SC */
        i1 = data.m + 1;
        
        // ns in this case is used as an indicator of the patch type
        // the original code bumps it from 0..3 to 1..4, although
        // the reason for this is not clear
        //ns++;
        
        // SP cards have to have a blank in I1, but is this really an error?
        // and if it is, why doesn't it report a problem if rad <> 0?
//        if (itg != 0) {
//          sprintf(msg, "Card %d is a SP, but it has data in I1.", i);
//          add_error(errors, msg, 1);
//        }
        
        // start with the simple case of a simple, single patch
        if(ns == 0) {
          xw2 = xw2 * TA;
          yw2 = yw2 * TA;
          patch(itg, ns, xw1, yw1, zw1, xw2, yw2, zw2, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
        }
        // this is the case where there is going to be one or more SC's following
        else {
          // make sure the next one is an SC
          // TODO: we should test the sanity of the inputs based on the ns
          if(strcmp(deck->cards[i + 1].card_code, "SC") != 0) {
            sprintf(msg, "Card %d is a SP with type %d, but the next card is not an SC, which it needs.", i, ns);
            add_error(errors, msg, 1);
            continue;
          }
          // if it's a triangle we just read one more point from the new card and go...
          if(ns == 2) {
            //read_geometry_card(gm, &ix, &iy, &x3, &y3, &z3, &x4, &y4, &z4, &dummy);
            x3 = deck->cards[i + 1].f1;
            y3 = deck->cards[i + 1].f2;
            z3 = deck->cards[i + 1].f3;
            i++; // skip the SC card next time through the loop
            patch(itg, ns, xw1, yw1, zw1, xw2, yw2, zw2, x3, y3, z3, 0.0, 0.0, 0.0);
          } /* ns == 2 */
          // if it's not a triangle, we have to loop over the following cards
          else {
            x3 = deck->cards[i + 1].f1;
            y3 = deck->cards[i + 1].f2;
            z3 = deck->cards[i + 1].f3;
            x3 = deck->cards[i + 1].f4;
            y3 = deck->cards[i + 1].f5;
            z3 = deck->cards[i + 1].f6;
            i++;
            patch(itg, ns, xw1, yw1, zw1, xw2, yw2, zw2, x3, y3, z3, x4, y4, z4);
            
            // now loop until we run out of following SC's
            while(strcmp(deck->cards[i + 1].card_code, "SC") == 0) {
              // copy the last set of end coords into this set's start coords
              xw1 = x3;
              yw1 = y3;
              zw1 = z3;
              xw2 = x4;
              yw2 = y4;
              zw2 = z4;
              // and then get the next set of end coords
              x3 = deck->cards[i + 1].f1;
              y3 = deck->cards[i + 1].f2;
              z3 = deck->cards[i + 1].f3;
              x3 = deck->cards[i + 1].f4;
              y3 = deck->cards[i + 1].f5;
              z3 = deck->cards[i + 1].f6;
              i++;
              patch(itg, ns, xw1, yw1, zw1, xw2, yw2, zw2, x3, y3, z3, x4, y4, z4);
            } /* while cards are SC's */
          }/* ns > 2 */
        } /* ns > 1 */

//
//        if (ns == 2 || ns == 4) isct = 1;
//
//        if (ns > 1) {
//
//          //read_geometry_card(gm, &ix, &iy, &x3, &y3, &z3, &x4, &y4, &z4, &dummy);
//
//          if ((ns == 2) || (itg > 0)) {
//            x4 = xw1 + x3 - xw2;
//            y4 = yw1 + y3 - yw2;
//            z4 = zw1 + z3 - zw2;
//          }
//
//          fprintf(output_fp, "\n"
//                  "      %11.5f %11.5f %11.5f %11.5f %11.5f %11.5f",
//                  x3, y3, z3, x4, y4, z4);
//

        continue;

      case 7: /* "sm" card, generate multiple-patch surface */
        i1 = data.m + 1;
  
        if(itg < 1 ||ns < 1) {
          sprintf(msg, "Card %d is a SM, but the number of patches in I1 or I2 is too small.", i);
          add_error(errors, msg, 1);
          continue;
        }
        if(strcmp(deck->cards[i + 1].card_code, "SC") != 0) {
          sprintf(msg, "Card %d is a SM, but the next card is not an SC, which it needs.", i);
          add_error(errors, msg, 1);
          continue;
        }
        
        // read the sc and skip it
        x3 = deck->cards[i + 1].f1;
        y3 = deck->cards[i + 1].f2;
        z3 = deck->cards[i + 1].f3;
        i++;
        
        // calculate corner 4
        if(ns == 2 || itg > 0) {
          x4 = xw1 + x3 - xw2;
          y4 = yw1 + y3 - yw2;
          z4 = zw1 + z3 - zw2;
        }
  
        patch(itg, ns, xw1, yw1, zw1, xw2, yw2, zw2, x3, y3, z3, x4, y4, z4);
        continue;

      case 8: /* "ga" card, generate segment data for wire arc */
        num_wires++;
        i1 = data.n + 1;
        i2 = data.n + ns;
  
//        fprintf(output_fp, "\n"
//          " %5d ARC RADIUS: %9.5f  FROM: %8.3f TO: %8.3f DEGREES"
//          "       %11.5f %5d %5d %5d %4d",
//          nwire, xw1, yw1, zw1, xw2, ns, i1, i2, itg);
  
        arc(itg, ns, xw1, yw1, zw1, xw2);
        continue;
        
      case 9:
        

      case 10: /* "gh" card, generate helix */
        num_wires++;
        i1 = data.n + 1;
        i2 = data.n + ns;
  
//        fprintf(output_fp, "\n"
//          " %5d HELIX STRUCTURE - SPACING OF TURNS: %8.3f AXIAL"
//          " LENGTH: %8.3f  %8.3f %5d %5d %5d %4d\n      "
//          " RADIUS X1:%8.3f Y1:%8.3f X2:%8.3f Y2:%8.3f ",
//          nwire, xw1, yw1, rad, ns, i1, i2, itg, zw1, xw2, yw2, zw2);
  
        // convert the original float value in F7 to int
         //its = (int)(card.f7 + .5);

        helix(xw1, yw1, zw1, xw2, yw2, zw2, rad, ns, itg);
        continue;
  
      case 11: /* "gf" card, not supported */
        abort_on_error(-5);

      default: /* error message */
        sprintf(msg, "Geometry card %d has an unknown mnemonic, '%s'.", i, card.card_code);
        add_error(errors, msg, 1);

    } /* switch on card type */
  } /* for over cards */
  
  // free any message we might have made
  free(msg);
} /* calculate_geometry */

    
//
//
//    // ===============================================================================
//
//  /* read geometry data card and branch to */
//  /* section for operation requested */
//  do {
//    //parse_geometry_card(gm, &itg, &ns, &xw1, &yw1, &zw1, &xw2, &yw2, &zw2, &rad);
//
//    /* identify card id mnemonic */
//    for(gm_num = 0; gm_num < NUM_GEOMETRY_CODES; gm_num++)
//
//    // this is the SC card, and sets the "isContinuation" flag
//    // why it doesn't test/set the others, like GC, is not clear
//    if(gm_num != 10)
//      isct = 0;
//
//    switch(gm_num) {
//    case 0: /* "gw" card, generate segment data for straight wire. */
//
//      nwire++;
//      i1 = data.n + 1;
//      i2 = data.n + ns;
//
//      fprintf(output_fp, "\n"
//        " %5d  %10.5f %10.5f %10.5f %10.5f"
//        " %10.5f %10.5f %10.5f %5d %5d %5d %4d",
//        nwire, xw1, yw1, zw1, xw2, yw2, zw2, rad, ns, i1, i2, itg);
//
//      if(rad != 0.0) {
//        xs1 = 1.;
//        ys1 = 1.;
//      }
//      else {
//        // if the radius is zero, the GW has to be followed by a GC containing the tapering data
//        //read_geometry_card(gm, &ix, &iy, &xs1, &ys1, &zs1, &dummy, &dummy, &dummy, &dummy);
//
//        if(strcmp(gm, "GC") != 0) {
//          fprintf(output_fp, "\n  GEOMETRY DATA CARD ERROR");
//          stop(-1);
//        }
//
//        fprintf(output_fp,
//          "\n  ABOVE WIRE IS TAPERED.  SEGMENT LENGTH RATIO: %9.5f\n"
//          "                                 "
//          "RADIUS FROM: %9.5f TO: %9.5f", xs1, ys1, zs1);
//
//        // why is this an error?
//        if((ys1 == 0.0) || (zs1 == 0.0)) {
//          fprintf(output_fp, "\n  GEOMETRY DATA CARD ERROR");
//          stop(-1);
//        }
//
//        rad = ys1;
//        ys1 = pow((zs1 / ys1), (1. / (ns - 1.)));
//      }
//
//      // now we have all the data, so turn it into segments
//      wire(xw1, yw1, zw1, xw2, yw2, zw2, rad, xs1, ys1, ns, itg);
//
//      continue;
//
//    case 1: /* "gx" card, reflect structure along x, y, or z axes or rotate to form cylinder.  */
//
//      iy = ns / 10;
//      iz = ns - iy * 10;
//      ix = iy / 10;
//      iy = iy - ix * 10;
//
//      if(ix != 0)
//        ix = 1;
//      if(iy != 0)
//        iy = 1;
//      if(iz != 0)
//        iz = 1;
//
//      fprintf(output_fp,
//        "\n      STRUCTURE REFLECTED ALONG THE AXES %c %c %c"
//        " - TAGS INCREMENTED BY %d",
//        ifx[ix], ify[iy], ifz[iz], itg);
//
//      reflect(ix, iy, iz, itg, ns);
//
//      continue;
//
//    case 2: /* "gr" card, rotate the structure */
//
//      fprintf(output_fp,
//        "\n  STRUCTURE ROTATED ABOUT Z-AXIS %d TIMES"
//        " - LABELS INCREMENTED BY %d", ns, itg);
//
//      ix = -1;
//      iz = 0;
//      iy = 0;
//
//      reflect(ix, iy, iz, itg, ns);
//
//      continue;
//
//    case 3: /* "gs" card, scale structure dimensions by factor xw1. */
//
//      fprintf(output_fp,
//        "\n     STRUCTURE SCALED BY FACTOR: %10.5f", xw1);
//
//      scale(xw1);
//
//      continue;
//
//    case 4: /* "ge" card, terminate structure geometry input. */
//
//      if(ns != 0) {
//        plot.iplp1 = 1;
//        plot.iplp2 = 1;
//      }
//
//      // if we're at the end of the geometry section, we have all the segments
//      // so now is an opportune time to connect them together
//      connect_segments(itg);
//
//      // print out the wire segment data if there is any
//      if(data.n != 0) {
//        /* Allocate wire buffers */
//        mreq = (size_t)data.n;
//        mreq *= sizeof(double);
//        mem_realloc((void *)&data.si, mreq);
//        mem_realloc((void *)&data.sab, mreq);
//        mem_realloc((void *)&data.cab, mreq);
//        mem_realloc((void *)&data.salp, mreq);
//        mem_realloc((void *)&data.x, mreq);
//        mem_realloc((void *)&data.y, mreq);
//        mem_realloc((void *)&data.z, mreq);
//
//        fprintf(output_fp, "\n\n\n"
//          "                              "
//          " ---------- SEGMENTATION DATA ----------\n"
//          "                                       "
//          " COORDINATES IN METERS\n"
//          "                           "
//          " I+ AND I- INDICATE THE SEGMENTS BEFORE AND AFTER I\n");
//
//        fprintf(output_fp, "\n"
//          "   SEG    COORDINATES OF SEGM CENTER     SEGM    ORIENTATION"
//          " ANGLES    WIRE    CONNECTION DATA   TAG\n"
//          "   No:       X         Y         Z      LENGTH     ALPHA     "
//          " BETA    RADIUS    I-     I    I+   No:");
//
//        for(i = 0; i < data.n; i++) {
//          xw1 = data.x2[i] - data.x1[i];
//          yw1 = data.y2[i] - data.y1[i];
//          zw1 = data.z2[i] - data.z1[i];
//          data.x[i] = (data.x1[i] + data.x2[i]) / 2.;
//          data.y[i] = (data.y1[i] + data.y2[i]) / 2.;
//          data.z[i] = (data.z1[i] + data.z2[i]) / 2.;
//          xw2 = xw1* xw1 + yw1* yw1 + zw1* zw1;
//          yw2 = sqrt(xw2);
//          yw2 = (xw2 / yw2 + yw2)*.5;
//          data.si[i] = yw2;
//          data.cab[i] = xw1 / yw2;
//          data.sab[i] = yw1 / yw2;
//          xw2 = zw1 / yw2;
//
//          if(xw2 > 1.)
//            xw2 = 1.;
//          if(xw2 < -1.)
//            xw2 = -1.;
//
//          data.salp[i] = xw2;
//          xw2 = asin(xw2)* TD;
//          yw2 = atan2(yw1, xw1)* TD;
//
//          fprintf(output_fp, "\n"
//            " %5d %9.4f %9.4f %9.4f %9.4f"
//            " %9.4f %9.4f %9.4f %5d %5d %5d %5d",
//            i + 1, data.x[i], data.y[i], data.z[i], data.si[i], xw2, yw2,
//            data.bi[i], data.icon1[i], i + 1, data.icon2[i], data.itag[i]);
//
//          if(plot.iplp1 == 1)
//            fprintf(plot_fp, "%12.4E %12.4E %12.4E "
//              "%12.4E %12.4E %12.4E %12.4E %5d %5d %5d\n",
//              data.x[i], data.y[i], data.z[i], data.si[i], xw2, yw2,
//              data.bi[i], data.icon1[i], i + 1, data.icon2[i]);
//
//          if((data.si[i] <= 1.e-20) || (data.bi[i] <= 0.)) {
//            fprintf(output_fp, "\n SEGMENT DATA ERROR");
//            stop(-1);
//          }
//
//        } /* for( i = 0; i < data.n; i++ ) */
//      } /* if( data.n != 0) */
//
//      // print out the patch data if there is any
//      if (data.m != 0) {
//        fprintf(output_fp, "\n\n\n"
//          "                                   "
//          " --------- SURFACE PATCH DATA ---------\n"
//          "                                            "
//          " COORDINATES IN METERS\n\n"
//          " PATCH      COORD. OF PATCH CENTER           UNIT NORMAL VECTOR      "
//          " PATCH           COMPONENTS OF UNIT TANGENT VECTORS\n"
//          "  No:       X          Y          Z          X        Y        Z      "
//          " AREA         X1       Y1       Z1        X2       Y2      Z2");
//
//        for(i = 0; i < data.m; i++) {
//          xw1 = (data.t1y[i] * data.t2z[i] - data.t1z[i] * data.t2y[i])* data.psalp[i];
//          yw1 = (data.t1z[i] * data.t2x[i] - data.t1x[i] * data.t2z[i])* data.psalp[i];
//          zw1 = (data.t1x[i] * data.t2y[i] - data.t1y[i] * data.t2x[i])* data.psalp[i];
//
//          fprintf(output_fp, "\n"
//            " %4d %10.5f %10.5f %10.5f  %8.4f %8.4f %8.4f"
//            " %10.5f  %8.4f %8.4f %8.4f  %8.4f %8.4f %8.4f",
//            i + 1, data.px[i], data.py[i], data.pz[i], xw1, yw1, zw1, data.pbi[i],
//            data.t1x[i], data.t1y[i], data.t1z[i], data.t2x[i], data.t2y[i], data.t2z[i]);
//
//        } /* for( i = 0; i < data.m; i++ ) */
//
//      } /* if( data.m == 0) */
//
//        // update the counters that track the total number of objects
//      data.npm = data.n + data.m;
//      data.np2m = data.n + 2 * data.m;
//      data.np3m = data.n + 3 * data.m;
//
//      return;
//
//    case 5: /* "gm" card, move structure or reproduce/duplicate original structure in new positions */
//
//      fprintf(output_fp,
//        "\n     THE STRUCTURE HAS BEEN MOVED, MOVE DATA CARD IS:\n"
//        "   %3d %5d %10.5f %10.5f %10.5f %10.5f %10.5f %10.5f %10.5f",
//        itg, ns, xw1, yw1, zw1, xw2, yw2, zw2, rad);
//
//      xw1 = xw1* TA;
//      yw1 = yw1* TA;
//      zw1 = zw1* TA;
//
//      move(xw1, yw1, zw1, xw2, yw2, zw2, (int)(rad + .5), ns, itg);
//      continue;
//
//    case 6: /* "sp" card, generate single new patch */
//
//      i1 = data.m + 1;
//      ns++;
//
//      if (itg != 0) {
//        fprintf(output_fp, "\n  PATCH DATA ERROR");
//        stop(-1);
//      }
//
//      fprintf(output_fp, "\n"
//        " %5d%c %10.5f %10.5f %10.5f %10.5f %10.5f %10.5f",
//        i1, ipt[ns - 1], xw1, yw1, zw1, xw2, yw2, zw2);
//
//      if ((ns == 2) || (ns == 4))
//        isct = 1;
//
//      if (ns > 1) {
//        //read_geometry_card(gm, &ix, &iy, &x3, &y3, &z3, &x4, &y4, &z4, &dummy);
//
//        if ((ns == 2) || (itg > 0)) {
//          x4 = xw1 + x3 - xw2;
//          y4 = yw1 + y3 - yw2;
//          z4 = zw1 + z3 - zw2;
//        }
//
//        fprintf(output_fp, "\n"
//          "      %11.5f %11.5f %11.5f %11.5f %11.5f %11.5f",
//          x3, y3, z3, x4, y4, z4);
//
//        if (strcmp(gm, "SC") != 0) {
//          fprintf(output_fp, "\n  PATCH DATA ERROR");
//          stop(-1);
//        }
//
//      } /* if( ns > 1) */
//      else {
//        xw2 = xw2* TA;
//        yw2 = yw2* TA;
//      }
//
//      patch(itg, ns, xw1, yw1, zw1, xw2, yw2, zw2, x3, y3, z3, x4, y4, z4);
//
//      continue;
//
//    case 7: /* "sm" card, generate multiple-patch surface */
//
//      i1 = data.m + 1;
//      fprintf(output_fp, "\n"
//        " %5d%c %10.5f %11.5f %11.5f %11.5f %11.5f %11.5f"
//        "     SURFACE - %d BY %d PATCHES",
//        i1, ipt[1], xw1, yw1, zw1, xw2, yw2, zw2, itg, ns);
//
//      if ((itg < 1) || (ns < 1)) {
//        fprintf(output_fp, "\n  PATCH DATA ERROR");
//        stop(-1);
//      }
//
//      //read_geometry_card(gm, &ix, &iy, &x3, &y3, &z3, &x4, &y4, &z4, &dummy);
//
//      if ((ns == 2) || (itg > 0)) {
//        x4 = xw1 + x3 - xw2;
//        y4 = yw1 + y3 - yw2;
//        z4 = zw1 + z3 - zw2;
//      }
//
//      fprintf(output_fp, "\n"
//        "      %11.5f %11.5f %11.5f %11.5f %11.5f %11.5f",
//        x3, y3, z3, x4, y4, z4);
//
//      if (strcmp(gm, "SC") != 0) {
//        fprintf(output_fp, "\n  PATCH DATA ERROR");
//        stop(-1);
//      }
//
//      patch(itg, ns, xw1, yw1, zw1, xw2, yw2, zw2, x3, y3, z3, x4, y4, z4);
//
//      continue;
//
//    case 8: /* "ga" card, generate segment data for wire arc */
//
//      nwire++;
//      i1 = data.n + 1;
//      i2 = data.n + ns;
//
//      fprintf(output_fp, "\n"
//        " %5d ARC RADIUS: %9.5f  FROM: %8.3f TO: %8.3f DEGREES"
//        "       %11.5f %5d %5d %5d %4d",
//        nwire, xw1, yw1, zw1, xw2, ns, i1, i2, itg);
//
//      arc(itg, ns, xw1, yw1, zw1, xw2);
//
//      continue;
//
//    case 9: /* "sc" card, patch continuation */
//
//      if (isct == 0) {
//        fprintf(output_fp, "\n  PATCH DATA ERROR");
//        stop(-1);
//      }
//
//      i1 = data.m + 1;
//      ns++;
//
//      if ((itg != 0) || ((ns != 2) && (ns != 4))) {
//        fprintf(output_fp, "\n  PATCH DATA ERROR");
//        stop(-1);
//      }
//
//      xs1 = x4;
//      ys1 = y4;
//      zs1 = z4;
//      xs2 = x3;
//      ys2 = y3;
//      zs2 = z3;
//      x3 = xw1;
//      y3 = yw1;
//      z3 = zw1;
//
//      if (ns == 4) {
//        x4 = xw2;
//        y4 = yw2;
//        z4 = zw2;
//      }
//
//      xw1 = xs1;
//      yw1 = ys1;
//      zw1 = zs1;
//      xw2 = xs2;
//      yw2 = ys2;
//      zw2 = zs2;
//
//      if (ns != 4) {
//        x4 = xw1 + x3 - xw2;
//        y4 = yw1 + y3 - yw2;
//        z4 = zw1 + z3 - zw2;
//      }
//
//      fprintf(output_fp, "\n"
//        " %5d%c %10.5f %11.5f %11.5f %11.5f %11.5f %11.5f",
//        i1, ipt[ns - 1], xw1, yw1, zw1, xw2, yw2, zw2);
//
//      fprintf(output_fp, "\n"
//        "      %11.5f %11.5f %11.5f  %11.5f %11.5f %11.5f",
//        x3, y3, z3, x4, y4, z4);
//
//      patch(itg, ns, xw1, yw1, zw1, xw2, yw2, zw2, x3, y3, z3, x4, y4, z4);
//
//      continue;
//
//    case 10: /* "gh" card, generate helix */
//
//      nwire++;
//      i1 = data.n + 1;
//      i2 = data.n + ns;
//
//      fprintf(output_fp, "\n"
//        " %5d HELIX STRUCTURE - SPACING OF TURNS: %8.3f AXIAL"
//        " LENGTH: %8.3f  %8.3f %5d %5d %5d %4d\n      "
//        " RADIUS X1:%8.3f Y1:%8.3f X2:%8.3f Y2:%8.3f ",
//        nwire, xw1, yw1, rad, ns, i1, i2, itg, zw1, xw2, yw2, zw2);
//
//      helix(xw1, yw1, zw1, xw2, yw2, zw2, rad, ns, itg);
//
//      continue;
//
//    case 11: /* "gf" card, not supported */
//      abort_on_error(-5);
//
//    default: /* error message */
//
//      fprintf(output_fp, "\n  GEOMETRY DATA CARD ERROR");
//      fprintf(output_fp, "\n"
//        " %2s %3d %5d %10.5f %10.5f %10.5f %10.5f %10.5f %10.5f %10.5f",
//        gm, itg, ns, xw1, yw1, zw1, xw2, yw2, zw2, rad);
//
//      stop(-1);
//
//    } /* switch( gm_num ) */
//
//  } /* do */
//  while (TRUE);
//
//    // all done with this!
//  free(msg);
//
//} /* calculate_geometry */

/*-----------------------------------------------------------------------*/

/* isegno returns the segment number of the mth segment having the */
/* tag number itagi. if itagi=0 segment number m is returned. */
int isegno(int itagi, int m)
{
	int icnt, iseg;

	if (m <= 0) {
		fprintf(output_fp,
			"\n  CHECK DATA, PARAMETER SPECIFYING SEGMENT"
			" POSITION IN A GROUP OF EQUAL TAGS MUST NOT BE ZERO");
		stop(-1);
	}

	// if the tag number is zero, then simply return that as the segment
	// FIXME: is there any point assigning iseg here?
	if (itagi == 0) {
		iseg = m;
		return(iseg);
	}

	// if the tag isn't zero, look for it in the segment collection
	icnt = 0;
	if (data.n > 0) {
		int i;	// shouldn't this be defined in the for?
		for (i = 0; i < data.n; i++) {
			if (data.itag[i] != itagi)
				continue;

			icnt++;
			if (icnt == m) {
				iseg = i + 1;
				return(iseg);
			}
		} /* for( i = 0; i < data.n; i++ ) */
	} /* if( data.n > 0) */

	fprintf(output_fp, "\n\n"
		"  NO SEGMENT HAS AN ITAG OF %d", itagi);
	stop(-1);

	return(0);
} /* end of isegno */

/*-------------------------------------------------------------------*/

/* arc generates segment geometry data for an arc of ns segments */
void arc( int itg, int ns, double rada, double ang1, double ang2, double rad )
{
  int ist;

  ist= data.n;
  data.n += ns;
  data.np= data.n;
  data.mp= data.m;
  data.ipsym=0;

  if( ns < 1)
	return;

  if( fabs( ang2- ang1) < 360.00001)
  {
	int i;
	double ang, dang, xs1, xs2, zs1, zs2;

	/* Reallocate tags buffer */
	size_t mreq = (size_t)data.n;
	mreq *= sizeof(int);
	mem_realloc( (void *)&data.itag, mreq );

	/* Reallocate wire buffers */
	mreq = (size_t)data.n;
	mreq *= sizeof(double);
	mem_realloc( (void *)&data.x1, mreq );
	mem_realloc( (void *)&data.y1, mreq );
	mem_realloc( (void *)&data.z1, mreq );
	mem_realloc( (void *)&data.x2, mreq );
	mem_realloc( (void *)&data.y2, mreq );
	mem_realloc( (void *)&data.z2, mreq );
	mem_realloc( (void *)&data.bi, mreq );

	ang= ang1* TA;
	dang=( ang2- ang1)* TA/ ns;
	xs1= rada* cos( ang);
	zs1= rada* sin( ang);

	for( i = ist; i < data.n; i++ )
	{
	  ang += dang;
	  xs2= rada* cos( ang);
	  zs2= rada* sin( ang);
	  data.x1[i]= xs1;

	  data.y1[i]=0.;
	  data.z1[i]= zs1;
	  data.x2[i]= xs2;
	  data.y2[i]=0.;
	  data.z2[i]= zs2;
	  xs1= xs2;
	  zs1= zs2;
	  data.bi[i]= rad;
	  data.itag[i]= itg;

	} /* for( i = ist; i < data.n; i++ ) */

  } /* if( fabs( ang2- ang1) < 360.00001) */
  else
  {
	fprintf( output_fp, "\n  ERROR -- ARC ANGLE EXCEEDS 360 DEGREES");
	stop(-1);
  }
} /* end of arc */

/*-----------------------------------------------------------------------*/

/* connect sets up segment connection data in arrays icon1 and */
/* icon2 by searching for segment ends that are in contact. */
void connect_segments( int ignd )
{
  int i, iz, ic, j, jx, ix, ixx, iseg, iend, jend, jump, ipf;
  double sep=0., xi1, yi1, zi1, xi2, yi2, zi2;
  double slen, xa, ya, za, xs, ys, zs;
  size_t mreq;

  segj.maxcon = 1;

  if( ignd != 0) {
	fprintf( output_fp, "\n\n     GROUND PLANE SPECIFIED." );

	if( ignd > 0)
	  fprintf( output_fp,
		  "\n     WHERE WIRE ENDS TOUCH GROUND, CURRENT WILL"
		  " BE INTERPOLATED TO IMAGE IN GROUND PLANE.\n" );

	if( data.ipsym == 2) {
	  data.np=2* data.np;
	  data.mp=2* data.mp;
	}

	if( abs( data.ipsym) > 2 ) {
	  data.np= data.n;
	  data.mp= data.m;
	}

	/** possibly should be error condition?? **/
	if(data.np > data.n) {
	  fprintf( output_fp,
		  "\n ERROR: NP > N IN CONECT()" );
	  stop(-1);
	}

	if((data.np == data.n) && (data.mp == data.m))
	  data.ipsym=0;

  } /* if( ignd != 0) */

  if(data.n != 0) {
	/* Allocate memory to connections */
	mreq = (size_t)(data.n + data.m);
	mreq *= sizeof(int);
	mem_realloc( (void *)&data.icon1, mreq );
	mem_realloc( (void *)&data.icon2, mreq );

	for(i = 0; i < data.n; i++) {
	  data.icon1[i] = data.icon2[i] = 0;
	  iz = i+1;
	  xi1= data.x1[i];
	  yi1= data.y1[i];
	  zi1= data.z1[i];
	  xi2= data.x2[i];
	  yi2= data.y2[i];
	  zi2= data.z2[i];
	  slen= sqrt( (xi2- xi1)*(xi2- xi1) + (yi2- yi1) *
		  (yi2- yi1) + (zi2- zi1)*(zi2- zi1) ) * SMIN;

	  /* determine connection data for end 1 of segment. */
	  jump = FALSE;
	  if(ignd > 0) {
		if(zi1 <= -slen) {
		  fprintf( output_fp,
			  "\n  GEOMETRY DATA ERROR -- SEGMENT"
			  " %d EXTENDS BELOW GROUND", iz );
		  stop(-1);
		}

		if( zi1 <= slen) {
		  data.icon1[i]= iz;
		  data.z1[i]=0.;
		  jump = TRUE;
		} /* if( zi1 <= slen) */
	  } /* if( ignd > 0) */

	  if( !jump ) {
		ic= i;
		for( j = 1; j < data.n; j++) {
		  ic++;
		  if( ic >= data.n)
			ic=0;

		  sep= fabs( xi1- data.x1[ic])+ fabs(yi1- data.y1[ic])+ fabs(zi1- data.z1[ic]);
		  if( sep <= slen) {
			data.icon1[i]= -(ic+1);
			break;
		  }

		  sep= fabs( xi1- data.x2[ic])+ fabs(yi1- data.y2[ic])+ fabs(zi1- data.z2[ic]);
		  if( sep <= slen) {
			data.icon1[i]= (ic+1);
			break;
		  }
		} /* for( j = 1; j < data.n; j++) */
	  } /* if( ! jump ) */

	  /* determine connection data for end 2 of segment. */
	  if( (ignd > 0) || jump ) {
		if( zi2 <= -slen) {
		  fprintf( output_fp,
			  "\n  GEOMETRY DATA ERROR -- SEGMENT"
			  " %d EXTENDS BELOW GROUND", iz );
		  stop(-1);
		}

		if( zi2 <= slen) {
		  if( data.icon1[i] == iz ) {
			fprintf( output_fp,
				"\n  GEOMETRY DATA ERROR -- SEGMENT"
				" %d LIES IN GROUND PLANE", iz );
			stop(-1);
		  }

		  data.icon2[i]= iz;
		  data.z2[i]=0.;
		  continue;

		} /* if( zi2 <= slen) */
	  } /* if( ignd > 0) */

	  ic= i;
	  for( j = 1; j < data.n; j++ ) {
		ic++;
		if( ic >= data.n)
		  ic=0;

		sep= fabs(xi2- data.x1[ic])+ fabs(yi2- data.y1[ic])+ fabs(zi2- data.z1[ic]);
		if(sep <= slen) {
		  data.icon2[i]= (ic+1);
		  break;
		}

		sep= fabs(xi2- data.x2[ic])+ fabs(yi2- data.y2[ic])+ fabs(zi2- data.z2[ic]);
		if(sep <= slen) {
		  data.icon2[i]= -(ic+1);
		  break;
		}

	  } /* for( j = 1; j < data.n; j++ ) */
	} /* for( i = 0; i < data.n; i++ ) */

	/* find wire-surface connections for new patches */
	if( data.m != 0) {
	  ix = -1;
	  i = 0;
	  while( ++i <= data.m ) {
		ix++;
		xs= data.px[ix];
		ys= data.py[ix];
		zs= data.pz[ix];

		for( iseg = 0; iseg < data.n; iseg++ ) {
		  xi1= data.x1[iseg];
		  yi1= data.y1[iseg];
		  zi1= data.z1[iseg];
		  xi2= data.x2[iseg];
		  yi2= data.y2[iseg];
		  zi2= data.z2[iseg];

		  /* for first end of segment */
		  slen=( fabs(xi2- xi1)+ fabs(yi2- yi1)+ fabs(zi2- zi1))* SMIN;
		  sep= fabs(xi1- xs)+ fabs(yi1- ys)+ fabs(zi1- zs);

		  /* connection - divide patch into 4 patches at present array loc. */
		  if( sep <= slen) {
			data.icon1[iseg]=PCHCON+ i;
			ic=0;
			subph( i, ic );
			break;
		  }

		  sep= fabs(xi2- xs)+ fabs(yi2- ys)+ fabs(zi2- zs);
		  if( sep <= slen) {
			data.icon2[iseg]=PCHCON+ i;
			ic=0;
			subph( i, ic );
			break;
		  }

		} /* for( iseg = 0; iseg < data.n; iseg++ ) */
	  } /* while( ++i <= data.m ) */
	} /* if( data.m != 0) */
  } /* if( data.n != 0) */

  fprintf( output_fp, "\n\n"
	  "     TOTAL SEGMENTS USED: %d   SEGMENTS IN A"
	  " SYMMETRIC CELL: %d   SYMMETRY FLAG: %d",
	  data.n, data.np, data.ipsym );

  if( data.m > 0)
	fprintf( output_fp,	"\n"
		"       TOTAL PATCHES USED: %d   PATCHES"
		" IN A SYMMETRIC CELL: %d",  data.m, data.mp );

  iseg=( data.n+ data.m)/( data.np+ data.mp);
  if( iseg != 1)  {
	/*** may be error condition?? ***/
	if( data.ipsym == 0 ) {
	  fprintf( output_fp,
		  "\n  ERROR: IPSYM=0 IN CONECT()" );
	  stop(-1);
	}

	if( data.ipsym < 0 )
	  fprintf( output_fp,
		  "\n  STRUCTURE HAS %d FOLD ROTATIONAL SYMMETRY\n", iseg );
	else {
	  ic= iseg/2;
	  if( iseg == 8)
		ic=3;
	  fprintf( output_fp,
		  "\n  STRUCTURE HAS %d PLANES OF SYMMETRY\n", ic );
	} /* if( data.ipsym < 0 ) */
  } /* if( iseg == 1) */

  if( data.n == 0)
	return;

  /* Allocate to connection buffers */
  mreq = (size_t)segj.maxcon;
  mreq *= sizeof(int);
  mem_realloc( (void *)&segj.jco, mreq );

  /* adjust connected segment ends to exactly coincide.  print junctions */
  /* of 3 or more seg.  also find old seg. connecting to new seg. */
  iseg = 0;
  ipf = FALSE;
  for( j = 0; j < data.n; j++ ) {
	jx = j+1;
	iend=-1;
	jend=-1;
	ix= data.icon1[j];
	ic=1;
	segj.jco[0]= -jx;
	xa= data.x1[j];
	ya= data.y1[j];
	za= data.z1[j];

	/* if( ix == 0 ) Not needed??
	{
	  fprintf( output_fp,
		  "\n  CONNECT - SEGMENT CONNECTION ERROR FOR SEGMENT: %d", ix );
	  stop(-1);
	} */

	while( TRUE ) {
	  if( (ix != 0) && (ix != (j+1)) && (ix <= PCHCON) ) {
		do {
		  if( ix < 0 )
			ix= -ix;
		  else
			jend= -jend;

		  jump = FALSE;

		  if( ix == jx )
			break;

		  if( ix < jx ) {
			jump = TRUE;
			break;
		  }

		  /* Record max. no. of connections */
		  ic++;
		  if( ic >= segj.maxcon ) {
			segj.maxcon = ic+1;
			mreq = (size_t)segj.maxcon;
			mreq *= sizeof(int);
			mem_realloc( (void *)&segj.jco, mreq );
		  }
		  segj.jco[ic-1]= ix* jend;

		  ixx = ix-1;
		  if( jend != 1) {
			xa= xa+ data.x1[ixx];
			ya= ya+ data.y1[ixx];
			za= za+ data.z1[ixx];
			ix= data.icon1[ixx];
			continue;
		  }

		  xa= xa+ data.x2[ixx];
		  ya= ya+ data.y2[ixx];
		  za= za+ data.z2[ixx];
		  ix= data.icon2[ixx];

		} /* do */
		while( ix != 0 );

		if( jump && (iend == 1) )
		  break;
		else
		  if( jump ) {
			iend=1;
			jend=1;
			ix= data.icon2[j];
			ic=1;
			segj.jco[0]= jx;
			xa= data.x2[j];
			ya= data.y2[j];
			za= data.z2[j];
			continue;
		  }

		sep= (double)ic;
		xa= xa/ sep;
		ya= ya/ sep;
		za= za/ sep;

		for( i = 0; i < ic; i++ ) {
		  ix= segj.jco[i];
		  if( ix <= 0) {
			ix= -ix;
			ixx = ix-1;
			data.x1[ixx]= xa;
			data.y1[ixx]= ya;
			data.z1[ixx]= za;
			continue;
		  }

		  ixx = ix-1;
		  data.x2[ixx]= xa;
		  data.y2[ixx]= ya;
		  data.z2[ixx]= za;
		} /* for( i = 0; i < ic; i++ ) */

		if( ic >= 3) {
		  if( ! ipf ) {
			fprintf( output_fp, "\n\n"
				"    ---------- MULTIPLE WIRE JUNCTIONS ----------\n"
				"    JUNCTION  SEGMENTS (- FOR END 1, + FOR END 2)" );
			ipf = TRUE;
		  }

		  iseg++;
		  fprintf( output_fp, "\n   %5d      ", iseg );

		  for( i = 1; i <= ic; i++ )  {
			fprintf( output_fp, "%5d", segj.jco[i-1] );
			if( !(i % 20) )
			  fprintf( output_fp, "\n              " );
		  }

		} /* if( ic >= 3) */
	  } /*if( (ix != 0) && (ix != j) && (ix <= PCHCON) ) */

	  if( iend == 1)
		break;

	  iend=1;
	  jend=1;
	  ix= data.icon2[j];
	  ic=1;
	  segj.jco[0]= jx;
	  xa= data.x2[j];
	  ya= data.y2[j];
	  za= data.z2[j];

	} /* while( TRUE ) */
  } /* for( j = 0; j < data.n; j++ ) */

  mreq = (size_t)segj.maxcon;
  mreq *= sizeof(double);
  mem_realloc( (void *)&segj.ax, mreq );
  mem_realloc( (void *)&segj.bx, mreq );
  mem_realloc( (void *)&segj.cx, mreq );
} /* end of connect_segments */

/*-----------------------------------------------------------------------*/

/* subroutine helix generates segment geometry */
/* data for a helix of ns segments */
void helix( double s, double hl, double a1, double b1,
	double a2, double b2, double rad, int ns, int itg )
{
  int ist, i;
  size_t mreq;
  double zinc, copy, sangle, hdia, turn, pitch, hmaj, hmin;

  ist= data.n;
  data.n += ns;
  data.np= data.n;
  data.mp= data.m;
  data.ipsym=0;

  if( ns < 1)
	return;

  zinc= fabs( hl/ ns);

  /* Reallocate tags buffer */
  mreq = (size_t)(data.n + data.m);
  mreq *= sizeof(int);
  mem_realloc( (void *)&data.itag, mreq );

  /* Reallocate wire buffers */
  mreq = (size_t)data.n;
  mreq *= sizeof(double);
  mem_realloc( (void *)&data.x1, mreq );
  mem_realloc( (void *)&data.y1, mreq );
  mem_realloc( (void *)&data.z1, mreq );
  mem_realloc( (void *)&data.x2, mreq );
  mem_realloc( (void *)&data.y2, mreq );
  mem_realloc( (void *)&data.z2, mreq );
  mem_realloc( (void *)&data.bi, mreq );

  data.z1[ist]=0.;
  for( i = ist; i < data.n; i++ )
  {
	data.bi[i]= rad;
	data.itag[i]= itg;

	if( i != ist )
	  data.z1[i]= data.z1[i-1]+ zinc;

	data.z2[i]= data.z1[i]+ zinc;

	if( a2 == a1 )
	{
	  if( b1 == 0.)
		b1= a1;

	  data.x1[i]= a1* cos(2.* PI* data.z1[i]/ s);
	  data.y1[i]= b1* sin(2.* PI* data.z1[i]/ s);
	  data.x2[i]= a1* cos(2.* PI* data.z2[i]/ s);
	  data.y2[i]= b1* sin(2.* PI* data.z2[i]/ s);
	}
	else
	{
	  if( b2 == 0.)
		b2= a2;

	  data.x1[i]=( a1+( a2- a1)* data.z1[i]/ fabs( hl))* cos(2.* PI* data.z1[i]/ s);
	  data.y1[i]=( b1+( b2- b1)* data.z1[i]/ fabs( hl))* sin(2.* PI* data.z1[i]/ s);
	  data.x2[i]=( a1+( a2- a1)* data.z2[i]/ fabs( hl))* cos(2.* PI* data.z2[i]/ s);
	  data.y2[i]=( b1+( b2- b1)* data.z2[i]/ fabs( hl))* sin(2.* PI* data.z2[i]/ s);

	} /* if( a2 == a1) */

	if( hl > 0.)
	  continue;

	copy= data.x1[i];
	data.x1[i]= data.y1[i];
	data.y1[i]= copy;
	copy= data.x2[i];
	data.x2[i]= data.y2[i];
	data.y2[i]= copy;

  } /* for( i = ist; i < data.n; i++ ) */

  if( a2 != a1)
  {
	sangle= atan( a2/( fabs( hl)+( fabs( hl)* a1)/( a2- a1)));
	fprintf( output_fp,
		"\n       THE CONE ANGLE OF THE SPIRAL IS %10.4f", sangle );
	return;
  }

  if( a1 == b1)
  {
	hdia=2.* a1;
	turn= hdia* PI;
	pitch= atan( s/( PI* hdia));
	turn= turn/ cos( pitch);
	pitch=180.* pitch/ PI;
  }
  else
  {
	if( a1 >= b1)
	{
	  hmaj=2.* a1;
	  hmin=2.* b1;
	}
	else
	{
	  hmaj=2.* b1;
	  hmin=2.* a1;
	}

	hdia= sqrt(( hmaj*hmaj+ hmin*hmin)/2* hmaj);
	turn=2.* PI* hdia;
	pitch=(180./ PI)* atan( s/( PI* hdia));

  } /* if( a1 == b1) */

  fprintf( output_fp, "\n"
	  "       THE PITCH ANGLE IS: %.4f    THE LENGTH OF WIRE/TURN IS: %.4f",
	  pitch, turn );
} /* end of helix */

/*-----------------------------------------------------------------------*/

/* scale() scales all existing geometry by the given factor. */
void scale(double xw1)
{
  int yw1;
  
  // scale the wires
  if (data.n > 0) {
    for (int i = 0; i < data.n; i++) {
      data.x1[i] = data.x1[i] * xw1;
      data.y1[i] = data.y1[i] * xw1;
      data.z1[i] = data.z1[i] * xw1;
      data.x2[i] = data.x2[i] * xw1;
      data.y2[i] = data.y2[i] * xw1;
      data.z2[i] = data.z2[i] * xw1;
      data.bi[i] = data.bi[i] * xw1;
    }
  } /* if( data.n >= n2) */
  
  // and then the patches
  if (data.m > 0) {
    yw1 = xw1* xw1;
    for (int i = 0; i < data.m; i++) {
      data.px[i] = data.px[i] * xw1;
      data.py[i] = data.py[i] * xw1;
      data.pz[i] = data.pz[i] * xw1;
      data.pbi[i] = data.pbi[i] * yw1;
    }
  } /* if( data.m >= m2) */
} /* end of scale */

/*-----------------------------------------------------------------------*/
/* copy() moves the structure with respect to its coordinate system
 * or reproduces/duplicates the structure in new positions.
 * structure is rotated about x,y,z axes by rox,roy,roz
 * respectively, then shifted by xs,ys,zs
 * formerly known as move(), but that conflicts with stdio
 */
void duplicate( double rox, double roy, double roz, double xs,
	double ys, double zs, int its, int nrpt, int itgi )
{
  int nrp, ix, i1, k, i;
  size_t mreq;
  double sps, cps, sth, cth, sph, cph, xx, xy;
  double xz, yx, yy, yz, zx, zy, zz, xi, yi, zi;

  if( fabs( rox)+ fabs( roy) > 1.0e-10)
	data.ipsym= data.ipsym*3;

  sps= sin( rox);
  cps= cos( rox);
  sth= sin( roy);
  cth= cos( roy);
  sph= sin( roz);
  cph= cos( roz);
  xx= cph* cth;
  xy= cph* sth* sps- sph* cps;
  xz= cph* sth* cps+ sph* sps;
  yx= sph* cth;
  yy= sph* sth* sps+ cph* cps;
  yz= sph* sth* cps- cph* sps;
  zx= -sth;
  zy= cth* sps;
  zz= cth* cps;

  if( nrpt == 0)
	nrp=1;
  else
	nrp= nrpt;

  // move the wires
  ix=1;
  if( data.n > 0) {
	int ir;
	i1= isegno( its, 1);
	if( i1 < 1)
	  i1= 1;

	ix= i1;
	if( nrpt == 0)
	  k= i1-1;
	else {
	  k= data.n;
	  /* Reallocate tags buffer */
	  mreq = (size_t)(data.n + data.m + (data.n + 1 - i1) * nrpt);
	  mreq *= sizeof(int);
	  mem_realloc( (void *)&data.itag, mreq );

	  /* Reallocate wire buffers */
	  mreq = (size_t)(data.n + (data.n + 1 - i1) * nrpt);
	  mreq *= sizeof(double);
	  mem_realloc( (void *)&data.x1, mreq );
	  mem_realloc( (void *)&data.y1, mreq );
	  mem_realloc( (void *)&data.z1, mreq );
	  mem_realloc( (void *)&data.x2, mreq );
	  mem_realloc( (void *)&data.y2, mreq );
	  mem_realloc( (void *)&data.z2, mreq );
	  mem_realloc( (void *)&data.bi, mreq );
	}

	for( ir = 0; ir < nrp; ir++ ) {
	  for( i = i1-1; i < data.n; i++ )  {
		xi= data.x1[i];
		yi= data.y1[i];
		zi= data.z1[i];
		data.x1[k]= xi* xx+ yi* xy+ zi* xz+ xs;
		data.y1[k]= xi* yx+ yi* yy+ zi* yz+ ys;
		data.z1[k]= xi* zx+ yi* zy+ zi* zz+ zs;
		xi= data.x2[i];
		yi= data.y2[i];
		zi= data.z2[i];
		data.x2[k]= xi* xx+ yi* xy+ zi* xz+ xs;
		data.y2[k]= xi* yx+ yi* yy+ zi* yz+ ys;
		data.z2[k]= xi* zx+ yi* zy+ zi* zz+ zs;
		data.bi[k]= data.bi[i];
		data.itag[k]= data.itag[i];
		if( data.itag[i] != 0)
		  data.itag[k]= data.itag[i]+ itgi;

		k++;

	  } /* for( i = i1; i < data.n; i++ ) */

	  i1= data.n+1;
	  data.n= k;

	} /* for( ir = 0; ir < nrp; ir++ ) */

  } /* if( data.n >= n2) */

  // repeat the move for any patches
  if( data.m > 0) {
	int ii;
	i1 = 0;
	if( nrpt == 0)
	  k= 0;
	else
	  k = data.m;

	/* Reallocate patch buffers */
	mreq = (size_t)(data.m * (nrpt + 1));
	mreq *= sizeof(double);
	mem_realloc( (void *)&data.px, mreq );
	mem_realloc( (void *)&data.py, mreq );
	mem_realloc( (void *)&data.pz, mreq );
	mem_realloc( (void *)&data.t1x, mreq );
	mem_realloc( (void *)&data.t1y, mreq );
	mem_realloc( (void *)&data.t1z, mreq );
	mem_realloc( (void *)&data.t2x, mreq );
	mem_realloc( (void *)&data.t2y, mreq );
	mem_realloc( (void *)&data.t2z, mreq );
	mem_realloc( (void *)&data.pbi, mreq );
	mem_realloc( (void *)&data.psalp, mreq );

	for( ii = 0; ii < nrp; ii++ ) {
	  for( i = i1; i < data.m; i++ ) {
		xi= data.px[i];
		yi= data.py[i];
		zi= data.pz[i];
		data.px[k]= xi* xx+ yi* xy+ zi* xz+ xs;
		data.py[k]= xi* yx+ yi* yy+ zi* yz+ ys;
		data.pz[k]= xi* zx+ yi* zy+ zi* zz+ zs;
		xi= data.t1x[i];
		yi= data.t1y[i];
		zi= data.t1z[i];
		data.t1x[k]= xi* xx+ yi* xy+ zi* xz;
		data.t1y[k]= xi* yx+ yi* yy+ zi* yz;
		data.t1z[k]= xi* zx+ yi* zy+ zi* zz;
		xi= data.t2x[i];
		yi= data.t2y[i];
		zi= data.t2z[i];
		data.t2x[k]= xi* xx+ yi* xy+ zi* xz;
		data.t2y[k]= xi* yx+ yi* yy+ zi* yz;
		data.t2z[k]= xi* zx+ yi* zy+ zi* zz;
		data.psalp[k]= data.psalp[i];
		data.pbi[k]= data.pbi[i];
		k++;
	  } /* for( i = i1; i < data.m; i++ ) */

	  i1= data.m;
	  data.m = k;
	} /* for( ii = 0; ii < nrp; ii++ ) */

  } /* if( data.m >= m2) */

  if( (nrpt == 0) && (ix == 1) )
    return;

  data.np= data.n;
  data.mp= data.m;
  data.ipsym=0;
} /* end of duplicate */

/*-----------------------------------------------------------------------*/

/* reflect() reflects partial structure along x,y, or z axes or rotates */
/* structure to complete a symmetric structure. */
void reflect( int ix, int iy, int iz, int itx, int nop )
{
  int iti, i, nx, itagi, k;
  size_t mreq;
  double e1, e2, fnop, sam, cs, ss, xk, yk;
  
  data.np= data.n;
  data.mp= data.m;
  data.ipsym=0;
  iti= itx;
  
  if(ix >= 0) {
    if(nop == 0)
      return;
    
    data.ipsym=1;
    
    /* reflect along z axis */
    if( iz != 0 ) {
      data.ipsym=2;
      
      if( data.n > 0 ) {
        /* Reallocate tags buffer */
        mreq = (size_t)(2 * data.n + data.m);
        mreq *= sizeof(int);
        mem_realloc( (void *)&data.itag, mreq );
        
        /* Reallocate wire buffers */
        mreq = (size_t)(2 * data.n);
        mreq *= sizeof(double);
        mem_realloc( (void *)&data.x1, mreq );
        mem_realloc( (void *)&data.y1, mreq );
        mem_realloc( (void *)&data.z1, mreq );
        mem_realloc( (void *)&data.x2, mreq );
        mem_realloc( (void *)&data.y2, mreq );
        mem_realloc( (void *)&data.z2, mreq );
        mem_realloc( (void *)&data.bi, mreq );
        
        for(i = 0; i < data.n; i++) {
          nx= i+ data.n;
          e1= data.z1[i];
          e2= data.z2[i];
          
          if( (fabs(e1)+fabs(e2) <= 1.0e-5) || (e1*e2 < -1.0e-6) ) {
            fprintf( output_fp,
                    "\n  GEOMETRY DATA ERROR--SEGMENT %d"
                    " LIES IN PLANE OF SYMMETRY", i+1 );
            stop(-1);
          }
          
          data.x1[nx]= data.x1[i];
          data.y1[nx]= data.y1[i];
          data.z1[nx]= -e1;
          data.x2[nx]= data.x2[i];
          data.y2[nx]= data.y2[i];
          data.z2[nx]= -e2;
          itagi= data.itag[i];
          
          if( itagi == 0)
            data.itag[nx]=0;
          if( itagi != 0)
            data.itag[nx]= itagi+ iti;
          
          data.bi[nx]= data.bi[i];
          
        } /* for( i = 0; i < data.n; i++ ) */
        
        data.n= data.n*2;
        iti= iti*2;
        
      } /* if( data.n > 0) */
      
      if(data.m > 0) {
        /* Reallocate patch buffers */
        mreq = (size_t)(2 * data.m);
        mreq *= sizeof(double);
        mem_realloc( (void *)&data.px, mreq );
        mem_realloc( (void *)&data.py, mreq );
        mem_realloc( (void *)&data.pz, mreq );
        mem_realloc( (void *)&data.t1x, mreq );
        mem_realloc( (void *)&data.t1y, mreq );
        mem_realloc( (void *)&data.t1z, mreq );
        mem_realloc( (void *)&data.t2x, mreq );
        mem_realloc( (void *)&data.t2y, mreq );
        mem_realloc( (void *)&data.t2z, mreq );
        mem_realloc( (void *)&data.pbi, mreq );
        mem_realloc( (void *)&data.psalp, mreq );
        
        for(i = 0; i < data.m; i++) {
          nx = i+data.m;
          if(fabs(data.pz[i]) <= 1.0e-10) {
            fprintf( output_fp,
                    "\n  GEOMETRY DATA ERROR--PATCH %d"
                    " LIES IN PLANE OF SYMMETRY", i+1 );
            stop(-1);
          }
          
          data.px[nx]= data.px[i];
          data.py[nx]= data.py[i];
          data.pz[nx]= -data.pz[i];
          data.t1x[nx]= data.t1x[i];
          data.t1y[nx]= data.t1y[i];
          data.t1z[nx]= -data.t1z[i];
          data.t2x[nx]= data.t2x[i];
          data.t2y[nx]= data.t2y[i];
          data.t2z[nx]= -data.t2z[i];
          data.psalp[nx]= -data.psalp[i];
          data.pbi[nx]= data.pbi[i];
        }
        
        data.m= data.m*2;
      } /* if( data.m >= m2) */
    } /* if( iz != 0) */
    
    /* reflect along y axis */
    if( iy != 0) {
      if( data.n > 0) {
        /* Reallocate tags buffer */
        mreq = (size_t)(2 * data.n + data.m);
        mreq *= sizeof(int);
        mem_realloc( (void *)&data.itag, mreq );
        
        /* Reallocate wire buffers */
        mreq = (size_t)(2 * data.n);
        mreq *= sizeof(double);
        mem_realloc( (void *)&data.x1, mreq );
        mem_realloc( (void *)&data.y1, mreq );
        mem_realloc( (void *)&data.z1, mreq );
        mem_realloc( (void *)&data.x2, mreq );
        mem_realloc( (void *)&data.y2, mreq );
        mem_realloc( (void *)&data.z2, mreq );
        mem_realloc( (void *)&data.bi, mreq );
        
        for( i = 0; i < data.n; i++ ) {
          nx= i+ data.n;
          e1= data.y1[i];
          e2= data.y2[i];
          
          if( (fabs(e1)+fabs(e2) <= 1.0e-5) || (e1*e2 < -1.0e-6) ) {
            fprintf( output_fp,
                    "\n  GEOMETRY DATA ERROR--SEGMENT %d"
                    " LIES IN PLANE OF SYMMETRY", i+1 );
            stop(-1);
          }
          
          data.x1[nx]= data.x1[i];
          data.y1[nx]= -e1;
          data.z1[nx]= data.z1[i];
          data.x2[nx]= data.x2[i];
          data.y2[nx]= -e2;
          data.z2[nx]= data.z2[i];
          itagi= data.itag[i];
          
          if( itagi == 0)
            data.itag[nx]=0;
          if( itagi != 0)
            data.itag[nx]= itagi+ iti;
          
          data.bi[nx]= data.bi[i];
          
        } /* for( i = n2-1; i < data.n; i++ ) */
        
        data.n= data.n*2;
        iti= iti*2;
        
      } /* if( data.n >= n2) */
      
      // reflect any patches
      if(data.m > 0)  {
        // reflection doubles the number of patches, so we start
        // by reallocating the patch list to hold the new ones
        mreq = (size_t)(2 * data.m);
        mreq *= sizeof(double);
        mem_realloc( (void *)&data.px, mreq );
        mem_realloc( (void *)&data.py, mreq );
        mem_realloc( (void *)&data.pz, mreq );
        mem_realloc( (void *)&data.t1x, mreq );
        mem_realloc( (void *)&data.t1y, mreq );
        mem_realloc( (void *)&data.t1z, mreq );
        mem_realloc( (void *)&data.t2x, mreq );
        mem_realloc( (void *)&data.t2y, mreq );
        mem_realloc( (void *)&data.t2z, mreq );
        mem_realloc( (void *)&data.pbi, mreq );
        mem_realloc( (void *)&data.psalp, mreq );
        
        for( i = 0; i < data.m; i++ ) {
          nx= i+data.m;
          if( fabs( data.py[i]) <= 1.0e-10) {
            fprintf( output_fp,
                    "\n  GEOMETRY DATA ERROR--PATCH %d"
                    " LIES IN PLANE OF SYMMETRY", i+1 );
            stop(-1);
          }
          
          data.px[nx]= data.px[i];
          data.py[nx]= -data.py[i];
          data.pz[nx]= data.pz[i];
          data.t1x[nx]= data.t1x[i];
          data.t1y[nx]= -data.t1y[i];
          data.t1z[nx]= data.t1z[i];
          data.t2x[nx]= data.t2x[i];
          data.t2y[nx]= -data.t2y[i];
          data.t2z[nx]= data.t2z[i];
          data.psalp[nx]= -data.psalp[i];
          data.pbi[nx]= data.pbi[i];
          
        } /* for( i = m2; i <= data.m; i++ ) */
        
        data.m= data.m*2;
        
      } /* if( data.m >= m2) */
      
    } /* if( iy != 0) */
    
    /* reflect along x axis */
    if( ix == 0 )
      return;
    
    if( data.n > 0 ) {
      /* Reallocate tags buffer */
      mreq = (size_t)(2 * data.n + data.m);
      mreq *= sizeof(int);
      mem_realloc( (void *)&data.itag, mreq );
      
      /* Reallocate wire buffers */
      mreq = (size_t)(2 * data.n);
      mreq *= sizeof(double);
      mem_realloc( (void *)&data.x1, mreq );
      mem_realloc( (void *)&data.y1, mreq );
      mem_realloc( (void *)&data.z1, mreq );
      mem_realloc( (void *)&data.x2, mreq );
      mem_realloc( (void *)&data.y2, mreq );
      mem_realloc( (void *)&data.z2, mreq );
      mem_realloc( (void *)&data.bi, mreq );
      
      for( i = 0; i < data.n; i++ ) {
        nx= i+ data.n;
        e1= data.x1[i];
        e2= data.x2[i];
        
        if( (fabs(e1)+fabs(e2) <= 1.0e-5) || (e1*e2 < -1.0e-6) ) {
          fprintf( output_fp,
                  "\n  GEOMETRY DATA ERROR--SEGMENT %d"
                  " LIES IN PLANE OF SYMMETRY", i+1 );
          stop(-1);
        }
        
        data.x1[nx]= -e1;
        data.y1[nx]= data.y1[i];
        data.z1[nx]= data.z1[i];
        data.x2[nx]= -e2;
        data.y2[nx]= data.y2[i];
        data.z2[nx]= data.z2[i];
        itagi= data.itag[i];
        
        if( itagi == 0)
          data.itag[nx]=0;
        if( itagi != 0)
          data.itag[nx]= itagi+ iti;
        
        data.bi[nx]= data.bi[i];
      }
      
      data.n= data.n*2;
      
    } /* if( data.n > 0) */
    
    if( data.m == 0 )
      return;
    
    /* Reallocate patch buffers */
    mreq = (size_t)(2 * data.m);
    mreq *= sizeof(double);
    mem_realloc( (void *)&data.px, mreq );
    mem_realloc( (void *)&data.py, mreq );
    mem_realloc( (void *)&data.pz, mreq );
    mem_realloc( (void *)&data.t1x, mreq );
    mem_realloc( (void *)&data.t1y, mreq );
    mem_realloc( (void *)&data.t1z, mreq );
    mem_realloc( (void *)&data.t2x, mreq );
    mem_realloc( (void *)&data.t2y, mreq );
    mem_realloc( (void *)&data.t2z, mreq );
    mem_realloc( (void *)&data.pbi, mreq );
    mem_realloc( (void *)&data.psalp, mreq );
    
    for( i = 0; i < data.m; i++ ) {
      nx= i+data.m;
      if( fabs( data.px[i]) <= 1.0e-10) {
        fprintf( output_fp,
                "\n  GEOMETRY DATA ERROR--PATCH %d"
                " LIES IN PLANE OF SYMMETRY", i+1 );
        stop(-1);
      }
      
      data.px[nx]= -data.px[i];
      data.py[nx]= data.py[i];
      data.pz[nx]= data.pz[i];
      data.t1x[nx]= -data.t1x[i];
      data.t1y[nx]= data.t1y[i];
      data.t1z[nx]= data.t1z[i];
      data.t2x[nx]= -data.t2x[i];
      data.t2y[nx]= data.t2y[i];
      data.t2z[nx]= data.t2z[i];
      data.psalp[nx]= -data.psalp[i];
      data.pbi[nx]= data.pbi[i];
    }
    
    data.m= data.m*2;
    return;
    
  } /* if( ix >= 0) */
  
  /* reproduce structure with rotation to form cylindrical structure */
  fnop= (double)nop;
  data.ipsym=-1;
  sam=TP/ fnop;
  cs= cos( sam);
  ss= sin( sam);
  
  if( data.n > 0) {
    data.n *= nop;
    nx= data.np;
    
    /* Reallocate tags buffer */
    mreq = (size_t)(data.n + data.m);
    mreq *= sizeof(int);
    mem_realloc( (void *)&data.itag, mreq );
    
    /* Reallocate wire buffers */
    mreq = (size_t)data.n;
    mreq *= sizeof(double);
    mem_realloc( (void *)&data.x1, mreq );
    mem_realloc( (void *)&data.y1, mreq );
    mem_realloc( (void *)&data.z1, mreq );
    mem_realloc( (void *)&data.x2, mreq );
    mem_realloc( (void *)&data.y2, mreq );
    mem_realloc( (void *)&data.z2, mreq );
    mem_realloc( (void *)&data.bi, mreq );
    
    for( i = nx; i < data.n; i++ ) {
      k= i- data.np;
      xk= data.x1[k];
      yk= data.y1[k];
      data.x1[i]= xk* cs- yk* ss;
      data.y1[i]= xk* ss+ yk* cs;
      data.z1[i]= data.z1[k];
      xk= data.x2[k];
      yk= data.y2[k];
      data.x2[i]= xk* cs- yk* ss;
      data.y2[i]= xk* ss+ yk* cs;
      data.z2[i]= data.z2[k];
      data.bi[i]= data.bi[k];
      itagi= data.itag[k];
      
      if( itagi == 0)
        data.itag[i]=0;
      if( itagi != 0)
        data.itag[i]= itagi+ iti;
    }
    
  } /* if( data.n >= n2) */
  
  if( data.m == 0 )
    return;
  
  data.m *= nop;
  nx= data.mp;
  
  /* Reallocate patch buffers */
  mreq = (size_t)data.m;
  mreq *= sizeof(double);
  mem_realloc( (void *)&data.px, mreq  );
  mem_realloc( (void *)&data.py, mreq  );
  mem_realloc( (void *)&data.pz, mreq );
  mem_realloc( (void *)&data.t1x, mreq );
  mem_realloc( (void *)&data.t1y, mreq );
  mem_realloc( (void *)&data.t1z, mreq );
  mem_realloc( (void *)&data.t2x, mreq );
  mem_realloc( (void *)&data.t2y, mreq );
  mem_realloc( (void *)&data.t2z, mreq );
  mem_realloc( (void *)&data.pbi, mreq );
  mem_realloc( (void *)&data.psalp, mreq );
  
  for(i = nx; i < data.m; i++) {
    k = i-data.mp;
    xk= data.px[k];
    yk= data.py[k];
    data.px[i]= xk* cs- yk* ss;
    data.py[i]= xk* ss+ yk* cs;
    data.pz[i]= data.pz[k];
    xk= data.t1x[k];
    yk= data.t1y[k];
    data.t1x[i]= xk* cs- yk* ss;
    data.t1y[i]= xk* ss+ yk* cs;
    data.t1z[i]= data.t1z[k];
    xk= data.t2x[k];
    yk= data.t2y[k];
    data.t2x[i]= xk* cs- yk* ss;
    data.t2y[i]= xk* ss+ yk* cs;
    data.t2z[i]= data.t2z[k];
    data.psalp[i]= data.psalp[k];
    data.pbi[i]= data.pbi[k];
  } /* for( i = nx; i < data.m; i++ ) */
} /* end of reflect */

/*-----------------------------------------------------------------------*/

/* patch generates and modifies patch geometry data */
void patch( int nx, int ny,
	double ax1, double ay1, double az1,
	double ax2, double ay2, double az2,
	double ax3, double ay3, double az3,
	double ax4, double ay4, double az4 )
{
  int mi, ntp;
  size_t mreq;
  double s1x=0., s1y=0., s1z=0., s2x=0., s2y=0., s2z=0., xst=0.;
  double znv, xnv, ynv, xa, xn2, yn2, zn2;

  /* new patches.  for nx=0, ny=1,2,3,4 patch is (respectively) */
  /* arbitrary, rectagular, triangular, or quadrilateral. */
  /* for nx and ny  > 0 a rectangular surface is produced with */
  /* nx by ny rectangular patches. */

  data.m++;
  mi= data.m-1;

  /* Reallocate patch buffers */
  mreq = (size_t)data.m;
  mreq *= sizeof(double);
  mem_realloc( (void *)&data.px, mreq );
  mem_realloc( (void *)&data.py, mreq );
  mem_realloc( (void *)&data.pz, mreq );
  mem_realloc( (void *)&data.t1x, mreq );
  mem_realloc( (void *)&data.t1y, mreq );
  mem_realloc( (void *)&data.t1z, mreq );
  mem_realloc( (void *)&data.t2x, mreq );
  mem_realloc( (void *)&data.t2y, mreq );
  mem_realloc( (void *)&data.t2z, mreq );
  mem_realloc( (void *)&data.pbi, mreq );
  mem_realloc( (void *)&data.psalp, mreq );

  if( nx > 0)
	ntp=2;
  else
	ntp= ny;

  if( ntp <= 1) {
    data.px[mi]= ax1;
    data.py[mi]= ay1;
    data.pz[mi]= az1;
    data.pbi[mi]= az2;
    znv= cos( ax2);
    xnv= znv* cos( ay2);
    ynv= znv* sin( ay2);
    znv= sin( ax2);
    xa= sqrt( xnv* xnv+ ynv* ynv);

    if( xa >= 1.0e-6) {
      data.t1x[mi]= -ynv/ xa;
      data.t1y[mi]= xnv/ xa;
      data.t1z[mi]=0.;
    } else {
      data.t1x[mi]=1.;
      data.t1y[mi]=0.;
      data.t1z[mi]=0.;
    }

  } /* if( ntp <= 1) */
  else {
    s1x= ax2- ax1;
    s1y= ay2- ay1;
    s1z= az2- az1;
    s2x= ax3- ax2;
    s2y= ay3- ay2;
    s2z= az3- az2;

    if( nx != 0) {
      s1x= s1x/ nx;
      s1y= s1y/ nx;
      s1z= s1z/ nx;
      s2x= s2x/ ny;
      s2y= s2y/ ny;
      s2z= s2z/ ny;
    }

    xnv= s1y* s2z- s1z* s2y;
    ynv= s1z* s2x- s1x* s2z;
    znv= s1x* s2y- s1y* s2x;
    xa= sqrt( xnv* xnv+ ynv* ynv+ znv* znv);
    xnv= xnv/ xa;
    ynv= ynv/ xa;
    znv= znv/ xa;
    xst= sqrt( s1x* s1x+ s1y* s1y+ s1z* s1z);
    data.t1x[mi]= s1x/ xst;
    data.t1y[mi]= s1y/ xst;
    data.t1z[mi]= s1z/ xst;

    if( ntp <= 2) {
      data.px[mi]= ax1+.5*( s1x+ s2x);
      data.py[mi]= ay1+.5*( s1y+ s2y);
      data.pz[mi]= az1+.5*( s1z+ s2z);
      data.pbi[mi]= xa;
    }
    else {
      if( ntp != 4) {
      data.px[mi]=( ax1+ ax2+ ax3)/3.;
      data.py[mi]=( ay1+ ay2+ ay3)/3.;
      data.pz[mi]=( az1+ az2+ az3)/3.;
      data.pbi[mi]=.5* xa;
      }
      else  {
        double salpn;
        s1x= ax3- ax1;
        s1y= ay3- ay1;
        s1z= az3- az1;
        s2x= ax4- ax1;
        s2y= ay4- ay1;
        s2z= az4- az1;
        xn2= s1y* s2z- s1z* s2y;
        yn2= s1z* s2x- s1x* s2z;
        zn2= s1x* s2y- s1y* s2x;
        xst= sqrt( xn2* xn2+ yn2* yn2+ zn2* zn2);
        salpn=1./(3.*( xa+ xst));
        data.px[mi]=( xa*( ax1+ ax2+ ax3)+ xst*( ax1+ ax3+ ax4))* salpn;
        data.py[mi]=( xa*( ay1+ ay2+ ay3)+ xst*( ay1+ ay3+ ay4))* salpn;
        data.pz[mi]=( xa*( az1+ az2+ az3)+ xst*( az1+ az3+ az4))* salpn;
        data.pbi[mi]=.5*( xa+ xst);
        s1x=( xnv* xn2+ ynv* yn2+ znv* zn2)/ xst;

        if( s1x <= 0.9998) {
          fprintf( output_fp,
            "\n  ERROR -- CORNERS OF QUADRILATERAL"
            " PATCH DO NOT LIE IN A PLANE" );
          stop(-1);
        }

      } /* if( ntp != 4) */

    } /* if( ntp <= 2) */

  } /* if( ntp <= 1) */

  data.t2x[mi]= ynv* data.t1z[mi]- znv* data.t1y[mi];
  data.t2y[mi]= znv* data.t1x[mi]- xnv* data.t1z[mi];
  data.t2z[mi]= xnv* data.t1y[mi]- ynv* data.t1x[mi];
  data.psalp[mi]=1.;

  if( nx != 0) {
	int iy, ix;
	double xs, ys, zs, xt, yt, zt;

	data.m += nx*ny-1;
	/* Reallocate patch buffers */
	mreq = (size_t)data.m;
	mreq *= sizeof(double);
	mem_realloc( (void *)&data.px, mreq );
	mem_realloc( (void *)&data.py, mreq );
	mem_realloc( (void *)&data.pz, mreq );
	mem_realloc( (void *)&data.t1x, mreq );
	mem_realloc( (void *)&data.t1y, mreq );
	mem_realloc( (void *)&data.t1z, mreq );
	mem_realloc( (void *)&data.t2x, mreq );
	mem_realloc( (void *)&data.t2y, mreq );
	mem_realloc( (void *)&data.t2z, mreq );
	mem_realloc( (void *)&data.pbi, mreq );
	mem_realloc( (void *)&data.psalp, mreq );

	xn2= data.px[mi]- s1x- s2x;
	yn2= data.py[mi]- s1y- s2y;
	zn2= data.pz[mi]- s1z- s2z;
	xs= data.t1x[mi];
	ys= data.t1y[mi];
	zs= data.t1z[mi];
	xt= data.t2x[mi];
	yt= data.t2y[mi];
	zt= data.t2z[mi];

	for(iy = 0; iy < ny; iy++) {
	  xn2 += s2x;
	  yn2 += s2y;
	  zn2 += s2z;

	  for(ix = 1; ix <= nx; ix++) {
      xst= (double)ix;
      data.px[mi]= xn2+ xst* s1x;
      data.py[mi]= yn2+ xst* s1y;
      data.pz[mi]= zn2+ xst* s1z;
      data.pbi[mi]= xa;
      data.psalp[mi]=1.;
      data.t1x[mi]= xs;
      data.t1y[mi]= ys;
      data.t1z[mi]= zs;
      data.t2x[mi]= xt;
      data.t2y[mi]= yt;
      data.t2z[mi]= zt;
      mi++;
	  } /* for( ix = 0; ix < nx; ix++ ) */
	} /* for( iy = 0; iy < ny; iy++ ) */
  } /* if( nx != 0) */

  data.ipsym = 0;
  data.np = data.n;
  data.mp = data.m;
} /* end of patch */

/*-----------------------------------------------------------------------*/

/*** this function was an 'entry point' (part of) 'patch()' ***/
void subph( int nx, int ny )
{
  int mia, ix, iy, mi;
  size_t mreq;
  double xs, ys, zs, xa, xst, s1x, s1y, s1z, s2x, s2y, s2z, saln, xt, yt;

  /* Reallocate patch buffers */
  if( ny == 0 ) {
    data.m += 3;
  } else {
    data.m += 4;
  }

  mreq = (size_t)data.m;
  mreq *= sizeof(double);
  mem_realloc( (void *)&data.px, mreq );
  mem_realloc( (void *)&data.py, mreq );
  mem_realloc( (void *)&data.pz, mreq );
  mem_realloc( (void *)&data.t1x, mreq );
  mem_realloc( (void *)&data.t1y, mreq );
  mem_realloc( (void *)&data.t1z, mreq );
  mem_realloc( (void *)&data.t2x, mreq );
  mem_realloc( (void *)&data.t2y, mreq );
  mem_realloc( (void *)&data.t2z, mreq );
  mem_realloc( (void *)&data.pbi, mreq );
  mem_realloc( (void *)&data.psalp, mreq );
  mreq = (size_t)(data.n + data.m);
  mreq *= sizeof(int);
  mem_realloc( (void *)&data.icon1, mreq );
  mem_realloc( (void *)&data.icon2, mreq );

  /* Shift patches to make room for new ones */
  if((ny == 0) && (nx != data.m))  {
    for(iy = data.m-1; iy > nx+2; iy--) {
      ix = iy-3;
      data.px[iy]= data.px[ix];
      data.py[iy]= data.py[ix];
      data.pz[iy]= data.pz[ix];
      data.pbi[iy]= data.pbi[ix];
      data.psalp[iy]= data.psalp[ix];
      data.t1x[iy]= data.t1x[ix];
      data.t1y[iy]= data.t1y[ix];
      data.t1z[iy]= data.t1z[ix];
      data.t2x[iy]= data.t2x[ix];
      data.t2y[iy]= data.t2y[ix];
      data.t2z[iy]= data.t2z[ix];
    }
  } /* if( (ny == 0) || (nx != m) ) */

  /* divide patch for connection */
  mi= nx-1;
  xs= data.px[mi];
  ys= data.py[mi];
  zs= data.pz[mi];
  xa= data.pbi[mi]/4.;
  xst= sqrt( xa)/2.;
  s1x= data.t1x[mi];
  s1y= data.t1y[mi];
  s1z= data.t1z[mi];
  s2x= data.t2x[mi];
  s2y= data.t2y[mi];
  s2z= data.t2z[mi];
  saln= data.psalp[mi];
  xt= xst;
  yt= xst;

  if(ny == 0)
    mia= mi;
  else {
    data.mp++;
    mia= data.m-1;
  }

  for(ix = 1; ix <= 4; ix++) {
    data.px[mia]= xs+ xt* s1x+ yt* s2x;
    data.py[mia]= ys+ xt* s1y+ yt* s2y;
    data.pz[mia]= zs+ xt* s1z+ yt* s2z;
    data.pbi[mia]= xa;
    data.t1x[mia]= s1x;
    data.t1y[mia]= s1y;
    data.t1z[mia]= s1z;
    data.t2x[mia]= s2x;
    data.t2y[mia]= s2y;
    data.t2z[mia]= s2z;
    data.psalp[mia]= saln;

    if( ix == 2)
      yt= -yt;

    if( (ix == 1) || (ix == 3) )
      xt= -xt;

    mia++;
  }

  if(nx <= data.mp)
    data.mp += 3;

  if(ny > 0)
    data.pz[mi]=10000.0;
} /* end of subph */

/*-----------------------------------------------------------------------*/

/* subroutine wire() generates segment geometry */
/* data for a straight wire of ns segments. */
void wire( double xw1, double yw1, double zw1,
	double xw2, double yw2, double zw2, double rad,
	double rdel, double rrad, int ns, int itg )
{
  int first_segment_num, i;
  size_t mreq;
  double xd, yd, zd, delz, rd, fns, radz;
  double xs1, ys1, zs1, xs2, ys2, zs2;

  // copy down the starting segment number, and then move up all the segment counters
  first_segment_num = data.n;
  data.n += ns;
  data.np = data.n;
  data.mp = data.m;	// do we need this? it shouldn't have changed
  data.ipsym = 0;	// this says that symmetry is not true?

  // only add this wire if it actually has segments
  if(ns < 1) return;

  /* Reallocate tags buffer */
  mreq = (size_t)(data.n + data.m);
  mreq *= sizeof(int);
  mem_realloc( (void *)&data.itag, mreq );

  /* Reallocate wire buffers */
  mreq = (size_t)data.n;	// this is the current number of wire segments
  mreq *= sizeof(double);
  mem_realloc( (void *)&data.x1, mreq );
  mem_realloc( (void *)&data.y1, mreq );
  mem_realloc( (void *)&data.z1, mreq );
  mem_realloc( (void *)&data.x2, mreq );
  mem_realloc( (void *)&data.y2, mreq );
  mem_realloc( (void *)&data.z2, mreq );
  mem_realloc( (void *)&data.bi, mreq );

  // calculate a segment length based either on the rdels parameter from a GC,
  // or the number of segments in a normal GW.
  xd = xw2- xw1;
  yd = yw2- yw1;
  zd = zw2- zw1;

  if(fabs(rdel - 1) >= 1.0e-6) {
    delz = sqrt( xd* xd+ yd* yd+ zd* zd);
    xd /= delz;
    yd /= delz;
    zd /= delz;
    delz = delz*(1.- rdel)/(1.- pow(rdel, ns) );
    rd = rdel;
  } else {
    fns= (double)ns;
    xd /= fns;
    yd /= fns;
    zd /= fns;
    delz = 1.;
    rd=1.;
  }

  // now start at one end...
  radz= rad;
  xs1= xw1;
  ys1= yw1;
  zs1= zw1;

  // and for the rest of the segments, generate a segment end after moving
  // xd/yd/zd along the line, filling out the interior points
  for(i = first_segment_num; i < data.n; i++) {
    data.itag[i] = itg;
    xs2 = xs1+ xd* delz;
    ys2 = ys1+ yd* delz;
    zs2 = zs1+ zd* delz;
    data.x1[i] = xs1;
    data.y1[i] = ys1;
    data.z1[i] = zs1;
    data.x2[i] = xs2;
    data.y2[i] = ys2;
    data.z2[i] = zs2;
    data.bi[i] = radz;
    delz = delz* rd;
    radz = radz* rrad;
    xs1 = xs2;
    ys1 = ys2;
    zs1 = zs2;
  }

  // fill in the end of the line with the end point
  data.x2[data.n-1] = xw2;
  data.y2[data.n-1] = yw2;
  data.z2[data.n-1] = zw2;
} /* end of wire() */
