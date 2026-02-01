/******************************************************************************
 * geometry.c
 *
 * geometry.c contains the code that parses the geometry section of
 * the deck and then generates a list of segments, patches, and
 * connections. These are collected into a geometry_t structure for
 * the deck.
 *
 ******************************************************************************/

#include "opennec.h"

/******************************************************************************
 * calculate_geometry
 *
 * calculate_geometry (formerly datagn) is the main routine for creation
 * of geometry data. It reads the geometry cards, builds segments and patches,
 * and returns various errors. The resulting data, in ctx->geometry, can then
 * be used to draw a diagram of the antenna as well as being used in the
 * calculations.
 *
 * The list of errors is local to geometry, as this allows the various work
 * methods to add new entries without having to pass around an errors object.
 * It's likely useful to create a new errors object for every geometry, but
 * it's equally usable by passing in a global errors.
 *
 * @param ctx nec_context_t structure that will be modified
 * @param deck deck_t structure that has the geometry cards
 * @param errors a list of errors to add to
 *
 */
void calculate_geometry(nec_context_t *ctx, deck_t *deck, errors_list_t *errors, outputs_list_t *outputs)
{
  //(void)outputs;// currently unused
  card_t *card;
  char *msg = calloc(1, MAX_ERROR_LEN);
  
  int code_num;   // geometry card code as a number
  int tag, segs;  // tag number (or zero) and number of segments to be added
  //int isct, iphd; // no longer used
  double rad, xs1, ys1, zs1, x4 = 0.0, y4 = 0.0, z4 = 0.0;
  double x3 = 0, y3 = 0, z3 = 0, xw1, xw2, yw1, yw2, zw1, zw2;
  int ix, iy, iz; // only used for reflection
  
  // set up the counters and flags
  ctx->geometry.ipsym = 0;
  ctx->geometry.n = 0;
  ctx->geometry.np = 0;
  ctx->geometry.m = 0;
  ctx->geometry.mp = 0;
  //isct = 0;     // this is "I am looking for an SC card", which we no longer need
  //iphd = FALSE;	// this is "I printed the header", also not used
  
  // make sure there's cards to process
  // TODO: should this be an error/warning? or just in test?
  if(deck->num_cards == 0 || deck->geometry_start == 0 || deck->geometry_end == 0) {
    free(msg);
    return;
  }
  
  // make sure all the formula-based values are up to date
  update_deck_values(deck);
  
  // loop over the geometry section of the deck, which should be correct by this point
  for(int i = deck->geometry_start; i <= deck->geometry_end; i++) {
    card = &deck->cards[i];
    
    // one of the few ways that onec modifies the original NEC code is by adding
    // a flag saying whether this card should be ignored. That makes it easy to
    // have a GUI with a switch to turn off a card during testing (for example)
    // without having to physically remove it from the deck. this is not the same
    // as commenting it out, because the card is still read and parsed, and the
    // segments are in the geometry and can still be used in a GUI
    //
    // TODO: implement this!
    if(card->ignore) continue;
    
    // convert the code into its numeric value so we can switch on it
    for(code_num = 0; code_num < NUM_GEOMETRY_CODES; code_num++) {
      if(strncmp(card->card_code, geometry_codes[code_num], 2) == 0) break;
    }
    // now read in the values that are the same for all the cards
    // NOTE: remember to read the VALUES, not the original inputs!
    tag = card->iv[1];
    segs = card->iv[2];
    xw1 = card->fv[1];
    yw1 = card->fv[2];
    zw1 = card->fv[3];
    xw2 = card->fv[4];
    yw2 = card->fv[5];
    zw2 = card->fv[6];
    rad = card->fv[7];
    
    // set the card's tag number and number of segments
    card->tag = tag;
    card->num_segments = segs;
    
    // and now the switch. basically all this does is call the appropriate
    // function to insert the segments for that card type, or complete
    // processing when it sees the GE
    switch(code_num) {
      case 0: // GW, make a wire
        // the radius can be in the f7 field, or it can be on the next card if its tapered
        if(rad != 0.0) {
          xs1 = 1.0;
          ys1 = 1.0;
        } else {
          // make sure the next card is a GC, although we should have already done that
          if(strcmp(deck->cards[i + 1].card_code, "GC") != 0) {
            sprintf(msg, "The card on line %d is a GW with a zero radius, but the next card is not a GC with the tapering info.", i + 1);
            add_error(ctx, errors, msg, WARNING);
            continue;
          }
          // and also that the values in it are valid
          if((deck->cards[i + 1].fv[2] == 0.0) || (deck->cards[i + 1].fv[3] == 0.0)) {
            sprintf(msg, "The card on line %d is a GC with tapering info for GW in card %d, but there is a zero in Y1 or Z1.", i + 2, i + 1);
            add_error(ctx,errors, msg, WARNING);
            continue;
          }
          // override the original inputs with the ones from the GC
          xs1 = deck->cards[i + 1].fv[1];  // check this!
          ys1 = deck->cards[i + 1].fv[2];
          zs1 = deck->cards[i + 1].fv[3];
          rad = ys1;
          ys1 = pow((zs1 / ys1), (1.0 / (segs - 1.0)));
          
          // move up a card so we don't process the GC
          i++;
        }
        
        // update the number of wires and the segment counts
        card->start_segment = ctx->geometry.n + 1;
        // now we have all the data, so turn it into segments
        wire(ctx, i, tag, segs, xw1, yw1, zw1, xw2, yw2, zw2, rad, xs1, ys1);
        // and cache the final number
        card->end_segment = ctx->geometry.n;
        continue;
        
      case 1: // GX, reflect structure along x, y, or z axes, or rotate to form cylinder
        // the gx puts a three-digit integer value in the I2 slot, and then uses its digits
        // as bit flags for the x, y and z axes. here were pull them out...
        iy = segs / 10;
        iz = segs - iy * 10;
        ix = iy / 10;
        iy = iy - ix * 10;
        
        if(ix != 0) ix = 1;
        if(iy != 0) iy = 1;
        if(iz != 0) iz = 1;
        
        card->start_segment = ctx->geometry.n + 1;
        reflect(ctx, i, tag, ix, iy, iz);
        card->end_segment = ctx->geometry.n;
        continue;
        
      case 2: // GR, rotate the structure
        // I2 is the number of times to duplicate the structure as it rotates
        
        // ix is set to -1 to indicate this is a rotation, not reflection
        rotate(ctx, i, tag, segs);
        continue;
        
      case 3: // GS, scale structure dimensions by factor xw1
        scale(ctx, xw1);
        continue;
        
      case 4: // GE, finish off the segments and patches, and calculate everything
        // FIXME: it's not clear what this is testing, on a GE card there shouldn't be an ns input
        //  perhaps it is  clearing out the ns from the previous line? but why bother when it's
        //  about to return anyway?
        if(segs != 0) {
          ctx->plot.iplp1 = 1;
          ctx->plot.iplp2 = 1;
        }
        
        // if we're at the end of the geometry section, we have all the segments
        // so now is an opportune time to connect them together
        connect_segments(ctx, tag, outputs);
        
        // ... and calculate the midpoints and other bits
        finish_geometry(ctx);
        
        // and in this case, we're done
        return;
        
      case 5: // GM, move structure or reproduce/duplicate original structure in new positions
        xw1 = xw1 * TA;
        yw1 = yw1 * TA;
        zw1 = zw1 * TA;
        
        // convert the original float value in F7 to int
        int tag_increment = (int)(card->fv[7] + .5);
        
        reproduce(ctx, xw1, yw1, zw1, xw2, yw2, zw2, tag_increment, segs, tag);
        continue;
        
      case 6: // SP, generate single new patch or a series of patches with SC
        //ns++;
        
        // SP cards have to have a blank in I1, but is this really an error?
        if (tag != 0) {
          sprintf(msg, "card_t %d is a SP, but it has data in I1.", i);
          add_error(ctx,errors, msg, WARNING);
        }
        
        // start with the simple case of a simple, single patch, no set shape
        if(segs == 0) {
          xw2 = xw2 * TA;
          yw2 = yw2 * TA;
          patch(ctx, i, tag, segs + 1, xw1, yw1, zw1, xw2, yw2, zw2, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
        }
        // other shapes, segs=1,2,3, require more inputs and there will be additional SC cards
        else {
          // make sure the next card is an SC
          // TODO: we should test the sanity of the inputs based on the ns
          if(strcmp(deck->cards[i + 1].card_code, "SC") != 0) {
            sprintf(msg, "The card on line %d is a SP with type %d, but the next card is not an SC, which it needs.", i + 1, segs);
            add_error(ctx, errors, msg, WARNING);
            continue;
          }
          // if it's a triangle we just read one more point from the new card and go...
          if(segs == 2) {
            x3 = deck->cards[i + 1].fv[1];
            y3 = deck->cards[i + 1].fv[2];
            z3 = deck->cards[i + 1].fv[3];
            i++; // skip the SC card next time through the main loop
            patch(ctx, i, tag, segs, xw1, yw1, zw1, xw2, yw2, zw2, x3, y3, z3, 0.0, 0.0, 0.0);
          } /* ns == 2 */
          // if it's not a triangle, we have to loop over the following cards
          else {
            // there has to be at least one following...
            x3 = deck->cards[i + 1].fv[1];
            y3 = deck->cards[i + 1].fv[2];
            z3 = deck->cards[i + 1].fv[3];
            x4 = deck->cards[i + 1].fv[4];
            y4 = deck->cards[i + 1].fv[5];
            z4 = deck->cards[i + 1].fv[6];
            i++;
            patch(ctx, i, tag, segs, xw1, yw1, zw1, xw2, yw2, zw2, x3, y3, z3, x4, y4, z4);
            
            // if it was segs=1 we are done at this point, for segs=3 there's more,
            // so loop until we run out of following SC's
            while(strcmp(deck->cards[i + 1].card_code, "SC") == 0) {
              // copy the last set of end coords into this set's start coords
              xw1 = x3;
              yw1 = y3;
              zw1 = z3;
              xw2 = x4;
              yw2 = y4;
              zw2 = z4;
              // and then get the next set of end coords
              x3 = deck->cards[i + 1].fv[1];
              y3 = deck->cards[i + 1].fv[2];
              z3 = deck->cards[i + 1].fv[3];
              x4 = deck->cards[i + 1].fv[4];
              y4 = deck->cards[i + 1].fv[5];
              z4 = deck->cards[i + 1].fv[6];
              i++;
              patch(ctx, i, tag, segs, xw1, yw1, zw1, xw2, yw2, zw2, x3, y3, z3, x4, y4, z4);
            } /* while cards are SC's */
          }/* ns = 2 */
        } /* ns > 0 */
        
        continue;
        
      case 7: // SM, generate multiple-patch rectangular surface
        if(tag < 1 || segs < 1) {
          sprintf(msg, "The card on line %d is a SM, but the number of patches in I1 or I2 is too small.", i + 1);
          add_error(ctx, errors, msg, 1);
          continue;
        }
        if(strcmp(deck->cards[i + 1].card_code, "SC") != 0) {
          sprintf(msg, "The card on line %d is a SM, but the next card is not an SC, which it needs.", i + 1);
          add_error(ctx, errors, msg, 1);
          continue;
        }
        
        // read the sc and skip it
        x3 = deck->cards[i + 1].fv[1];
        y3 = deck->cards[i + 1].fv[2];
        z3 = deck->cards[i + 1].fv[3];
        i++;
        
        // calculate corner 4
        if(segs == 2 || tag > 0) {
          x4 = xw1 + x3 - xw2;
          y4 = yw1 + y3 - yw2;
          z4 = zw1 + z3 - zw2;
        }
        
        patch(ctx, i, tag, segs, xw1, yw1, zw1, xw2, yw2, zw2, x3, y3, z3, x4, y4, z4);
        continue;
        
      case 8: // GA, generate segment data for wire arc
        arc(ctx, i, tag, segs, xw1, yw1, zw1, xw2);
        continue;
        
      case 9: // SC card, skip it - but it should never happen because SP/SM should have read it
        
      case 10: // GH, generate helix
        helix(ctx, i, tag, segs, xw1, yw1, zw1, xw2, yw2, zw2, rad, outputs);
        continue;
        
      case 11: // GF, not supported
        // TODO: support this!
        add_error(ctx, errors, "GF card not supported", FATAL);
        return;
        
      default: // error message if this isn't a comment
        if(!is_comment(card)) {
          sprintf(msg, "Geometry card on line %d has an unknown mnemonic, '%s'.", i + 1, card->card_code);
          add_error(ctx, errors, msg, 1);
        }
    } /* switch on card type */
  } /* for loop over cards */
  
  // free any message we might have made
  free(msg);
} /* calculate_geometry */

/******************************************************************************
 * segment_number
 *
 * segment_number (formerly isegno) returns the segment number for the @p m th
 * segment within the structure generated by the card with tag number @p tag.
 * For instance, the 5th segment within tag 7 might be segment_number 25.
 *
 * @param tag The tag number of the structure/card
 * @param m The segment number within that structure
 *
 */
int segment_number(nec_context_t *ctx, int tag, int m)
{
  int icnt, iseg;
  char *msg;
  
  if (m <= 0) {
    msg = calloc(MAX_ERROR_LEN, sizeof(char));
    sprintf(msg, "segment_number was called with a segment number less or equal to zero.");
    add_error(ctx, &ctx->geometry.errors, msg, 1);
    free(msg);
  }
  
  // if the tag number is zero, then simply return the mth segment as the answer
  // FIXME: is there any point assigning iseg here?
  if (tag == 0) {
    iseg = m;
    return(iseg);
  }
  
  // if the tag isn't zero, look for it in the segment collection
  icnt = 0;
  if (ctx->geometry.n > 0) {
    for (int i = 0; i < ctx->geometry.n; i++) {
      if (ctx->geometry.tag_nums[i] != tag)
        continue;
      
      icnt++;
      if (icnt == m) {
        iseg = i + 1;
        return(iseg);
      }
    } /* for( i = 0; i < ctx->geometry.n; i++ ) */
  } /* if( ctx->geometry.n > 0) */
  
  // if we didn't find it, report the error and return 0
  {
    msg = calloc(MAX_ERROR_LEN, sizeof(char));
    sprintf(msg, "segment_number was called with an unknown tag %d", tag);
    add_error(ctx, &ctx->geometry.errors, msg, 1);
    free(msg);
  }
  
  return(0);
} /* end of segment_number */

/******************************************************************************
 * connect_segments
 *
 * connect_segments (formerly CONECT) sets up segment connection data in
 * arrays icon1 and icon2 by searching for segment ends that are in contact.
 *
 * @param ignd If a ground plane is in use, checks if wires touch ground
 *
 */
int connect_segments(nec_context_t *ctx, int ignd, outputs_list_t *outputs)
{
  int i, iz, ic, j, jx, ix, ixx, iseg, iend, jend, jump, ipf;
  double sep=0., xi1, yi1, zi1, xi2, yi2, zi2;
  double slen, xa, ya, za, xs, ys, zs;
  size_t mreq;
  char *msg = calloc(MAX_ERROR_LEN * 10, sizeof(char));
  
  ctx->segj.maxcon = 1;
  
  if(ignd != 0) {
    add_message(ctx, outputs, "\n\n     GROUND PLANE SPECIFIED.");

    if( ignd > 0)
      add_message(ctx, outputs,
              "\n     WHERE WIRE ENDS TOUCH GROUND, CURRENT WILL"
              " BE INTERPOLATED TO IMAGE IN GROUND PLANE.\n" );

    if(ctx->geometry.ipsym == 2) {
      ctx->geometry.np = 2 * ctx->geometry.np;
      ctx->geometry.mp = 2 * ctx->geometry.mp;
    }

    if(abs(ctx->geometry.ipsym) > 2) {
      ctx->geometry.np = ctx->geometry.n;
      ctx->geometry.mp = ctx->geometry.m;
    }
    
    /** possibly should be error condition?? **/
    if(ctx->geometry.np > ctx->geometry.n) {
      char err_msg[256];
      snprintf(err_msg, sizeof(err_msg), "connect_segments was called np > n, %d > %d", ctx->geometry.np, ctx->geometry.n);
      add_error(ctx, &ctx->errors, err_msg, FATAL);
      return -1;
    }
    
    if((ctx->geometry.np == ctx->geometry.n) && (ctx->geometry.mp == ctx->geometry.m))
      ctx->geometry.ipsym = 0;
    
  } /* if( ignd != 0) */
  
  if(ctx->geometry.n != 0) {
    /* Allocate memory to connections */
    mreq = (size_t)(ctx->geometry.n + ctx->geometry.m);
    mreq *= sizeof(int);
    mem_realloc(ctx, (void *)&ctx->geometry.icon1, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.icon2, mreq);
    
    for(i = 0; i < ctx->geometry.n; i++) {
      ctx->geometry.icon1[i] = ctx->geometry.icon2[i] = 0;
      iz = i+1;
      xi1 = ctx->geometry.x1[i];
      yi1 = ctx->geometry.y1[i];
      zi1 = ctx->geometry.z1[i];
      xi2 = ctx->geometry.x2[i];
      yi2 = ctx->geometry.y2[i];
      zi2 = ctx->geometry.z2[i];
      slen = sqrt( (xi2- xi1)*(xi2- xi1) + (yi2- yi1) *
                  (yi2- yi1) + (zi2- zi1)*(zi2- zi1) ) * SMIN;
      
      // determine connection data for end 1 of segment
      jump = FALSE;
      if(ignd > 0) {
        if(zi1 <= -slen) {
          char *msg = calloc(MAX_ERROR_LEN, sizeof(char));
          sprintf(msg, "GEOMETRY DATA ERROR -- SEGMENT %d EXTENDS BELOW GROUND", iz);
          add_error(ctx, &ctx->geometry.errors, msg, 1);
          free(msg);
          return -1;
        }
        
        if( zi1 <= slen) {
          ctx->geometry.icon1[i]= iz;
          ctx->geometry.z1[i]=0.;
          jump = TRUE;
        } /* if( zi1 <= slen) */
      } /* if( ignd > 0) */
      
      if( !jump ) {
        ic= i;
        for( j = 1; j < ctx->geometry.n; j++) {
          ic++;
          if( ic >= ctx->geometry.n)
            ic=0;
          
          sep= fabs( xi1- ctx->geometry.x1[ic])+ fabs(yi1- ctx->geometry.y1[ic])+ fabs(zi1- ctx->geometry.z1[ic]);
          if( sep <= slen) {
            ctx->geometry.icon1[i]= -(ic+1);
            break;
          }
          
          sep= fabs( xi1- ctx->geometry.x2[ic])+ fabs(yi1- ctx->geometry.y2[ic])+ fabs(zi1- ctx->geometry.z2[ic]);
          if( sep <= slen) {
            ctx->geometry.icon1[i]= (ic+1);
            break;
          }
        } /* for( j = 1; j < data.n; j++) */
      } /* if( ! jump ) */
      
      /* determine connection data for end 2 of segment. */
      if( (ignd > 0) || jump ) {
        if( zi2 <= -slen) {
          char err_msg[256];
          snprintf(err_msg, sizeof(err_msg), "GEOMETRY DATA ERROR -- SEGMENT %d EXTENDS BELOW GROUND", iz);
          add_error(ctx, &ctx->errors, err_msg, FATAL);
          return -1;
        }
        
        if( zi2 <= slen) {
          if( ctx->geometry.icon1[i] == iz ) {
            char err_msg[256];
            snprintf(err_msg, sizeof(err_msg), "GEOMETRY DATA ERROR -- SEGMENT %d LIES IN GROUND PLANE", iz);
            add_error(ctx, &ctx->errors, err_msg, FATAL);
            return -1;
          }
          
          ctx->geometry.icon2[i] = iz;
          ctx->geometry.z2[i] = 0.;
          continue;
          
        } /* if( zi2 <= slen) */
      } /* if( ignd > 0) */
      
      ic= i;
      for(j = 1; j < ctx->geometry.n; j++) {
        ic++;
        if( ic >= ctx->geometry.n)
          ic=0;
        
        sep= fabs(xi2- ctx->geometry.x1[ic])+ fabs(yi2- ctx->geometry.y1[ic])+ fabs(zi2- ctx->geometry.z1[ic]);
        if(sep <= slen) {
          ctx->geometry.icon2[i]= (ic+1);
          break;
        }
        
        sep= fabs(xi2- ctx->geometry.x2[ic])+ fabs(yi2- ctx->geometry.y2[ic])+ fabs(zi2- ctx->geometry.z2[ic]);
        if(sep <= slen) {
          ctx->geometry.icon2[i]= -(ic+1);
          break;
        }
        
      } /* for( j = 1; j < data.n; j++ ) */
    } /* for( i = 0; i < data.n; i++ ) */
    
    /* find wire-surface connections for new patches */
    if(ctx->geometry.m != 0) {
      ix = -1;
      i = 0;
      while(++i <= ctx->geometry.m) {
        ix++;
        xs = ctx->geometry.px[ix];
        ys = ctx->geometry.py[ix];
        zs = ctx->geometry.pz[ix];
        
        for(iseg = 0; iseg < ctx->geometry.n; iseg++) {
          xi1 = ctx->geometry.x1[iseg];
          yi1 = ctx->geometry.y1[iseg];
          zi1 = ctx->geometry.z1[iseg];
          xi2 = ctx->geometry.x2[iseg];
          yi2 = ctx->geometry.y2[iseg];
          zi2 = ctx->geometry.z2[iseg];
          
          /* for first end of segment */
          slen = (fabs(xi2 - xi1) + fabs(yi2 - yi1) + fabs(zi2 - zi1))* SMIN;
          sep = fabs(xi1 - xs) + fabs(yi1 - ys) + fabs(zi1 - zs);
          
          /* connection - divide patch into 4 patches at present array loc. */
          if(sep <= slen) {
            ctx->geometry.icon1[iseg] = PCHCON + i;
            ic=0;
            calculate_patch(ctx, i, ic);
            break;
          }
          
          sep = fabs(xi2- xs)+ fabs(yi2- ys)+ fabs(zi2- zs);
          if(sep <= slen) {
            ctx->geometry.icon2[iseg] = PCHCON + i;
            ic = 0;
            calculate_patch(ctx, i, ic);
            break;
          }
          
        } /* for( iseg = 0; iseg < data.n; iseg++ ) */
      } /* while( ++i <= data.m ) */
    } /* if( data.m != 0) */
  } /* if( data.n != 0) */
  
  // if we have no geometry, we're done
  if(ctx->geometry.n == 0) {
    free(msg);
    return 0;
  }
  
  // allocate to connection buffers
  mreq = (size_t)ctx->segj.maxcon;
  mreq *= sizeof(int);
  mem_realloc(ctx, (void *)&ctx->segj.jco, mreq);
  
  /* adjust connected segment ends to exactly coincide.  print junctions */
  /* of 3 or more seg.  also find old seg. connecting to new seg. */
  iseg = 0;
  ipf = FALSE;
  for(j = 0; j < ctx->geometry.n; j++) {
    jx = j + 1;
    iend = -1;
    jend = -1;
    ix = ctx->geometry.icon1[j];
    ic = 1;
    ctx->segj.jco[0] = -jx;
    xa = ctx->geometry.x1[j];
    ya = ctx->geometry.y1[j];
    za = ctx->geometry.z1[j];
    
    /* if( ix == 0 ) Not needed??
     {
     fprintf( output_fp,
     "\n  CONNECT - SEGMENT CONNECTION ERROR FOR SEGMENT: %d", ix );
     stop(ctx, -1);
     } */
    
    while(TRUE) {
      if((ix != 0) && (ix != (j+1)) && (ix <= PCHCON)) {
        do {
          if(ix < 0)
            ix = -ix;
          else
            jend = -jend;
          
          jump = FALSE;
          
          if(ix == jx)
            break;
          
          if(ix < jx) {
            jump = TRUE;
            break;
          }
          
          /* Record max. no. of connections */
          ic++;
          if(ic >= ctx->segj.maxcon) {
            ctx->segj.maxcon = ic + 1;
            mreq = (size_t)ctx->segj.maxcon;
            mreq *= sizeof(int);
            mem_realloc(ctx, (void *)&ctx->segj.jco, mreq);
          }
          ctx->segj.jco[ic-1]= ix* jend;
          
          ixx = ix-1;
          if(jend != 1) {
            xa = xa + ctx->geometry.x1[ixx];
            ya = ya + ctx->geometry.y1[ixx];
            za = za + ctx->geometry.z1[ixx];
            ix = ctx->geometry.icon1[ixx];
            continue;
          }
          
          xa = xa + ctx->geometry.x2[ixx];
          ya = ya + ctx->geometry.y2[ixx];
          za = za + ctx->geometry.z2[ixx];
          ix = ctx->geometry.icon2[ixx];
          
        } /* do */
        while(ix != 0);
        
        if(jump && (iend == 1))
          break;
        else
          if(jump) {
            iend = 1;
            jend = 1;
            ix = ctx->geometry.icon2[j];
            ic = 1;
            ctx->segj.jco[0] = jx;
            xa = ctx->geometry.x2[j];
            ya = ctx->geometry.y2[j];
            za = ctx->geometry.z2[j];
            continue;
          }
        
        sep= (double)ic;
        xa= xa / sep;
        ya= ya / sep;
        za= za / sep;
        
        for(i = 0; i < ic; i++) {
          ix = ctx->segj.jco[i];
          if(ix <= 0) {
            ix = -ix;
            ixx = ix - 1;
            ctx->geometry.x1[ixx] = xa;
            ctx->geometry.y1[ixx] = ya;
            ctx->geometry.z1[ixx] = za;
            continue;
          }
          
          ixx = ix - 1;
          ctx->geometry.x2[ixx] = xa;
          ctx->geometry.y2[ixx] = ya;
          ctx->geometry.z2[ixx] = za;
        } /* for( i = 0; i < ic; i++ ) */
        
        if(ic >= 3) {
          if(!ipf) {
            sprintf(msg, "\n\n    ---------- MULTIPLE WIRE JUNCTIONS ----------\n    JUNCTION  SEGMENTS (- FOR END 1, + FOR END 2)");
            add_message(ctx, outputs, msg);
            ipf = TRUE;
          }

          iseg++;
          sprintf(msg, "\n   %5d      ", iseg);

          for(i = 1; i <= ic; i++)  {
            sprintf(msg + strlen(msg), "%5d", ctx->segj.jco[i-1]);
            if(!(i % 20)) // why 20?
              sprintf(msg + strlen(msg), "\n              ");
          }
          add_message(ctx, outputs, msg);
          
        } /* if( ic >= 3) */
      } /*if( (ix != 0) && (ix != j) && (ix <= PCHCON) ) */
      
      if(iend == 1)
        break;
      
      iend = 1;
      jend = 1;
      ix = ctx->geometry.icon2[j];
      ic = 1;
      ctx->segj.jco[0] = jx;
      xa = ctx->geometry.x2[j];
      ya = ctx->geometry.y2[j];
      za = ctx->geometry.z2[j];
      
    } /* while( TRUE ) */
  } /* for( j = 0; j < data.n; j++ ) */
  
  mreq = (size_t)ctx->segj.maxcon;
  mreq *= sizeof(double);
  mem_realloc(ctx, (void *)&ctx->segj.ax, mreq);
  mem_realloc(ctx, (void *)&ctx->segj.bx, mreq);
  mem_realloc(ctx, (void *)&ctx->segj.cx, mreq);
  free(msg);
  return 0;
} /* end of connect_segments */

/******************************************************************************
 * finish_geometry
 *
 * finish_geometry (formerly part of calculate_geometry) calculates midpoints
 * of wires and patches and similar values that run when the GE is seen.
 *
 * Some of the calculations it performed were used only for display in the
 * output files, including the angles of segments and the midpoints of patches.
 * These have been moved to output.c. As a result, this code no longer does
 * anything with the patches and it's possible that more of the values being
 * cached here may be removed entirely.
 *
 */
void finish_geometry(nec_context_t *ctx)
{
  size_t mreq;
  double xw1, yw1, zw1;
  double xw2, yw2;
  char *msg = calloc(1, MAX_ERROR_LEN);
  
  // and now we calculate various geometry-related data for wires,
  // like the centerpoints and orientation
  if(ctx->geometry.n != 0) {
    // reallocate the buffers
    mreq = (size_t)ctx->geometry.n * sizeof(double);
    mem_realloc(ctx, (void *)&ctx->geometry.si, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.sab, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.cab, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.salp, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.x, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.y, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.z, mreq);
    
    for(int i = 0; i < ctx->geometry.n; i++) {
      // calculate the segment midpoints
      xw1 = ctx->geometry.x2[i] - ctx->geometry.x1[i];
      yw1 = ctx->geometry.y2[i] - ctx->geometry.y1[i];
      zw1 = ctx->geometry.z2[i] - ctx->geometry.z1[i];
      ctx->geometry.x[i] = (ctx->geometry.x1[i] + ctx->geometry.x2[i]) / 2.0;
      ctx->geometry.y[i] = (ctx->geometry.y1[i] + ctx->geometry.y2[i]) / 2.0;
      ctx->geometry.z[i] = (ctx->geometry.z1[i] + ctx->geometry.z2[i]) / 2.0;
      
      // and lengths
      xw2 = xw1 * xw1 + yw1 * yw1 + zw1 * zw1;
      yw2 = sqrt(xw2);
      yw2 = (xw2 / yw2 + yw2) * 0.5;
      ctx->geometry.si[i] = yw2;
      
      // and angles
      ctx->geometry.cab[i] = xw1 / yw2;
      ctx->geometry.sab[i] = yw1 / yw2;
      xw2 = zw1 / yw2;
      
      if(xw2 > 1.0)
        xw2 = 1.0;
      if(xw2 < -1.0)
        xw2 = -1.0;
      ctx->geometry.salp[i] = xw2;
      
      if(ctx->geometry.si[i] <= 1.e-20) {
        sprintf(msg, "The length of segment %d is too small to process.", i + 1);
        add_error(ctx, &ctx->geometry.errors, msg, 1);
      }
      if(ctx->geometry.bi[i] <= 0.0) {
        sprintf(msg, "The radius of segment %d is too small to process.", i + 1);
        add_error(ctx, &ctx->geometry.errors, msg, 1);
      }
    } /* for( i = 0; i < ctx->geometry.n; i++ ) */
  } /* if( ctx->geometry.n != 0) */
  
  // update the counters that track the total number of segments and patches
  ctx->geometry.npm = ctx->geometry.n + ctx->geometry.m;
  ctx->geometry.np2m = ctx->geometry.n + 2 * ctx->geometry.m;
  ctx->geometry.np3m = ctx->geometry.n + 3 * ctx->geometry.m;
  
  free(msg);
}

/******************************************************************************
 * wire
 *
 * wire generates segment geometry data for a straight wire of @p segs segments.
 *
 * @param card_num card_t number for this set of segments
 * @param tag_num Tag number for this set of segments, maybe 0segs
 * @param segs Number of segments in the arc
 * @param xw1 Starting X point of one end of the wire
 * @param yw1 Starting Y point of one end of the wire
 * @param zw1 Starting Z point of one end of the wire
 * @param xw2 Ending X point of other end of the wire
 * @param yw2 Ending Y point of other end of the wire
 * @param zw2 Ending Z point of other end of the wire
 * @param wire_radius Radius of the wire
 * @param rdel Taper parameter length
 * @param rrad Taper parameter radius
 *
 */
void wire(nec_context_t *ctx, int card_num, int tag_num, int segs,
          double xw1, double yw1, double zw1,
          double xw2, double yw2, double zw2,
          double rad, double rdel, double rrad)
{
  int first_segment_num;
  size_t mreq;
  double xd, yd, zd, delz, rd, fns, radz;
  double xs1, ys1, zs1, xs2, ys2, zs2;
  
  // only add this wire if it actually has segments
  // NOTE: in the original code  this was done below setting the n and np
  //       below, which would mean adding a wire with zero segments would
  //       reset geometry, which seems to make no sense
  if(segs < 1) return;
  
  // FIXME: should this also check if the length is zero?
  
  // copy down the starting segment number, and then move up all the segment counters
  first_segment_num = ctx->geometry.n;
  ctx->geometry.n += segs;
  
  // reset the symmetry
  ctx->geometry.np = ctx->geometry.n;
  ctx->geometry.mp = ctx->geometry.m;
  ctx->geometry.ipsym = 0;
  
  // reallocate the cards and tags buffers
  mreq = (size_t)(ctx->geometry.n + ctx->geometry.m);
  mreq *= sizeof(int);
  mem_realloc(ctx, (void *)&ctx->geometry.card_nums, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.tag_nums, mreq);
  
  // reallocate wire buffers
  mreq = (size_t)ctx->geometry.n;  // this is the current number of wire segments, after adding the new segments
  mreq *= sizeof(double);
  mem_realloc(ctx, (void *)&ctx->geometry.x1, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.y1, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.z1, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.x2, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.y2, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.z2, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.bi, mreq);
  
  // calculate a segment length based either on the rdels parameter from a GC,
  // or the number of segments in a normal GW
  xd = xw2 - xw1;
  yd = yw2 - yw1;
  zd = zw2 - zw1;
  
  if(fabs(rdel - 1) >= 1.0e-6) {
    delz = sqrt(xd * xd + yd * yd + zd * zd);
    xd /= delz;
    yd /= delz;
    zd /= delz;
    delz = delz * (1.0- rdel)/(1.0- pow(rdel, segs) );
    rd = rdel;
  } else {
    fns= (double)segs;
    xd /= fns;
    yd /= fns;
    zd /= fns;
    delz = 1.0;
    rd= 1.0;
  }
  
  // now start at one end...
  radz = rad;
  xs1 = xw1;
  ys1 = yw1;
  zs1 = zw1;
  
  // and for the rest of the segments, generate a segment end after moving
  // xd/yd/zd along the line, filling out the interior points
  for(int i = first_segment_num; i < ctx->geometry.n; i++) {
    // save these out
    ctx->geometry.card_nums[i] = card_num;
    ctx->geometry.tag_nums[i] = tag_num;
    
    // calculate the new locations
    xs2 = xs1 + xd * delz;
    ys2 = ys1 + yd * delz;
    zs2 = zs1 + zd * delz;
    
    // set the geometry
    ctx->geometry.x1[i] = xs1;
    ctx->geometry.y1[i] = ys1;
    ctx->geometry.z1[i] = zs1;
    ctx->geometry.x2[i] = xs2;
    ctx->geometry.y2[i] = ys2;
    ctx->geometry.z2[i] = zs2;
    ctx->geometry.bi[i] = radz;
    
    // move to the other end and and re-taper
    delz = delz * rd;
    radz = radz * rrad;
    xs1 = xs2;
    ys1 = ys2;
    zs1 = zs2;
  } /* loop over remaining segments */
  
  // fill in the end of the line with the last point
  ctx->geometry.x2[ctx->geometry.n-1] = xw2;
  ctx->geometry.y2[ctx->geometry.n-1] = yw2;
  ctx->geometry.z2[ctx->geometry.n-1] = zw2;
} /* end of wire() */

/******************************************************************************
 * arc
 *
 * arc generates segment geometry data for an arc of @p segs segments.
 *
 * @param card_num card_t number for this set of segments
 * @param tag_num Tag number for this set of segments, maybe 0
 * @param segs Number of segments in the arc
 * @param arc_radius Radius of the arc
 * @param ang1 Starting angle
 * @param ang2 Ending angle - ang2-ang1 <= 360
 * @param wire_radius Radius of the wire
 *
 */
void arc(nec_context_t *ctx, int card_num, int tag_num, int segs, double rada, double ang1, double ang2, double rad)
{
  double ang, dang, xs1, xs2, zs1, zs2;
  int first_segment_num = ctx->geometry.n;
  
  // no point continuing if there are no segments
  if(segs < 1) return;
  
  // this test was previously performed at the end, which meant that
  // symmetry was removed even if it didn't actually build the arc.
  // as is the case in wire and helix, we will do the test now
  if(fabs(ang2- ang1) > 360.0000) {
    char *msg = calloc(1, MAX_ERROR_LEN);
    sprintf(msg, "The card on line %d is a GA with an angle >360 degrees.", card_num + 1);
    add_error(ctx, &ctx->geometry.errors, msg, 1);
    free(msg);
    return;
  }
  
  // update the segment count
  ctx->geometry.n += segs;
  
  // reset symmetry
  ctx->geometry.np = ctx->geometry.n;
  ctx->geometry.mp = ctx->geometry.m;
  ctx->geometry.ipsym = 0;
  
  // Reallocate card nums and tags buffer
  size_t mreq = (size_t)ctx->geometry.n;
  mreq *= sizeof(int);
  mem_realloc(ctx, (void *)&ctx->geometry.card_nums, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.tag_nums, mreq);
  
  // reallocate wire buffers
  mreq = (size_t)ctx->geometry.n;
  mreq *= sizeof(double);
  mem_realloc(ctx, (void *)&ctx->geometry.x1, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.y1, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.z1, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.x2, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.y2, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.z2, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.bi, mreq);
  
  ang = ang1 * TA;
  dang = (ang2- ang1) * TA/ segs;
  xs1 = rada * cos(ang);
  zs1 = rada * sin(ang);
  
  for(int i = first_segment_num; i < ctx->geometry.n; i++) {
    // save these bits out
    ctx->geometry.card_nums[i] = card_num;
    ctx->geometry.tag_nums[i] = tag_num;
    ctx->geometry.bi[i] = rad;
    
    // move around the arc by the delta angle
    ang += dang;
    xs2 = rada * cos(ang);
    zs2 = rada * sin(ang);
    
    // save that out
    ctx->geometry.x1[i] = xs1;
    ctx->geometry.y1[i] = 0.0;
    ctx->geometry.z1[i] = zs1;
    ctx->geometry.x2[i] = xs2;
    ctx->geometry.y2[i] = 0.0;
    ctx->geometry.z2[i] = zs2;
    
    // move up one stop
    xs1 = xs2;
    zs1 = zs2;
  } /* for( i = ist; i < data.n; i++ ) */
} /* end of arc */

/******************************************************************************
 * helix
 *
 * helix generates segment geometry data for an a helix of @p segs segments.
 *
 * @param tag_num Tag number for this set of segments
 * @param segs Number of segments in the arc
 * @param rad Radius of the wire
 *
 */
void helix(nec_context_t *ctx, int card_num, int tag_num, int segs, double s, double hl,
           double a1, double b1, double a2, double b2, double rad, outputs_list_t *outputs)
{
  int first_seg_num;
  size_t mreq;
  double zinc, copy, sangle, hdia, turn, pitch, hmaj, hmin;
  
  // no point continuing if the number of segments is zero
  if(segs < 1) return;
  
  // update the counters
  first_seg_num = ctx->geometry.n;
  ctx->geometry.n += segs;
  
  // reset symmetry
  ctx->geometry.np = ctx->geometry.n;
  ctx->geometry.mp = ctx->geometry.m;
  ctx->geometry.ipsym = 0;
  
  zinc = fabs(hl / segs);
  
  // reallocate card num and tags buffer
  mreq = (size_t)(ctx->geometry.n + ctx->geometry.m);
  mreq *= sizeof(int);
  mem_realloc(ctx, (void *)&ctx->geometry.card_nums, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.tag_nums, mreq);
  
  // reallocate wire buffers
  mreq = (size_t)ctx->geometry.n;
  mreq *= sizeof(double);
  mem_realloc(ctx, (void *)&ctx->geometry.x1, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.y1, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.z1, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.x2, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.y2, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.z2, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.bi, mreq);
  
  ctx->geometry.z1[first_seg_num] = 0.0;
  for(int i = first_seg_num; i < ctx->geometry.n; i++ ) {
    // save these out
    ctx->geometry.card_nums[i] = card_num;
    ctx->geometry.tag_nums[i] = tag_num;
    ctx->geometry.bi[i] = rad;
    
    if(i != first_seg_num)
      ctx->geometry.z1[i] = ctx->geometry.z1[i-1] + zinc;
    
    ctx->geometry.z2[i] = ctx->geometry.z1[i] + zinc;
    
    if(a2 == a1) {
      if(b1 == 0.0)
        b1 = a1;
      
      ctx->geometry.x1[i]= a1* cos(2.* PI* ctx->geometry.z1[i]/ s);
      ctx->geometry.y1[i]= b1* sin(2.* PI* ctx->geometry.z1[i]/ s);
      ctx->geometry.x2[i]= a1* cos(2.* PI* ctx->geometry.z2[i]/ s);
      ctx->geometry.y2[i]= b1* sin(2.* PI* ctx->geometry.z2[i]/ s);
    }
    else
    {
      if(b2 == 0.0)
        b2= a2;
      
      ctx->geometry.x1[i]=( a1+( a2- a1)* ctx->geometry.z1[i]/ fabs( hl))* cos(2.* PI* ctx->geometry.z1[i]/ s);
      ctx->geometry.y1[i]=( b1+( b2- b1)* ctx->geometry.z1[i]/ fabs( hl))* sin(2.* PI* ctx->geometry.z1[i]/ s);
      ctx->geometry.x2[i]=( a1+( a2- a1)* ctx->geometry.z2[i]/ fabs( hl))* cos(2.* PI* ctx->geometry.z2[i]/ s);
      ctx->geometry.y2[i]=( b1+( b2- b1)* ctx->geometry.z2[i]/ fabs( hl))* sin(2.* PI* ctx->geometry.z2[i]/ s);
    } /* if( a2 == a1) */
    
    if(hl > 0.0)
      continue;
    
    copy= ctx->geometry.x1[i];
    ctx->geometry.x1[i]= ctx->geometry.y1[i];
    ctx->geometry.y1[i]= copy;
    copy= ctx->geometry.x2[i];
    ctx->geometry.x2[i]= ctx->geometry.y2[i];
    ctx->geometry.y2[i]= copy;
    
  } /* for( i = ist; i < data.n; i++ ) */
  
  if(a2 != a1) {
    sangle = atan( a2/( fabs( hl)+( fabs( hl)* a1)/( a2- a1)));
    char *msg = calloc(MAX_ERROR_LEN, sizeof(char));
    sprintf(msg, "\n       THE CONE ANGLE OF THE SPIRAL IS %10.4f", sangle);
    add_message(ctx, outputs, msg);
    free(msg);
    return;
  }
  
  if(a1 == b1) {
    hdia=2.* a1;
    turn= hdia* PI;
    pitch= atan( s/( PI* hdia));
    turn= turn/ cos( pitch);
    pitch=180.* pitch/ PI;
  }
  else
  {
    if(a1 >= b1) {
      hmaj=2.* a1;
      hmin=2.* b1;
    } else {
      hmaj=2.* b1;
      hmin=2.* a1;
    }
    
    hdia = sqrt(( hmaj*hmaj+ hmin*hmin)/2* hmaj);
    turn = 2.0 * PI * hdia;
    pitch = (180.0/ PI)* atan( s/( PI* hdia));
  } /* if( a1 == b1) */
  
  {
    char *msg = calloc(MAX_ERROR_LEN, sizeof(char));
    sprintf(msg, "\n       THE PITCH ANGLE IS: %.4f    THE LENGTH OF WIRE/TURN IS: %.4f", pitch, turn);
    add_message(ctx, outputs, msg);
    free(msg);
  }
} /* end of helix */

/******************************************************************************
 * scale
 *
 * scales all existing geometry by the given factor. As the overall geometry
 * is the same before and after, differing only in values, there are no changes
 * to the tag or card numbers.
 *
 * @param scale_factor the amount to scale by
 *
 */
void scale(nec_context_t *ctx, double xw1)
{
  // scale the wires
  if(ctx->geometry.n > 0) {
    for(int i = 0; i < ctx->geometry.n; i++) {
      ctx->geometry.x1[i] = ctx->geometry.x1[i] * xw1;
      ctx->geometry.y1[i] = ctx->geometry.y1[i] * xw1;
      ctx->geometry.z1[i] = ctx->geometry.z1[i] * xw1;
      ctx->geometry.x2[i] = ctx->geometry.x2[i] * xw1;
      ctx->geometry.y2[i] = ctx->geometry.y2[i] * xw1;
      ctx->geometry.z2[i] = ctx->geometry.z2[i] * xw1;
      ctx->geometry.bi[i] = ctx->geometry.bi[i] * xw1;
    }
  } /* if( data.n >= n2) */

  // and then the patches
  if(ctx->geometry.m > 0) {
    double area_factor = xw1 * xw1;
    for (int i = 0; i < ctx->geometry.m; i++) {
      ctx->geometry.px[i] = ctx->geometry.px[i] * xw1;
      ctx->geometry.py[i] = ctx->geometry.py[i] * xw1;
      ctx->geometry.pz[i] = ctx->geometry.pz[i] * xw1;
      ctx->geometry.pbi[i] = ctx->geometry.pbi[i] * area_factor;
    }
  } /* if( data.m >= m2) */
} /* end of scale */

/******************************************************************************
 * reproduce
 *
 * reproduce moves the structure with respect to its coordinate system or
 * reproduces/duplicates the structure in new positions. The structure is
 * rotated about x,y,z axes by rox,roy,roz respectively, and then shifted by
 * xs,ys,zs. Any new elements are given new tag numbers offset from their
 * original value by the number in tag_increment. Geometry with a tag of
 * zero will also be zero after duplication.
 *
 * formerly known as move(), but that conflicts with stdio
 *
 */
void reproduce(nec_context_t *ctx, double rox, double roy, double roz, double xs,
               double ys, double zs, int its, int nrpt, int tag_increment)
{
  int nrp, ix, i1, k, i;
  size_t mreq;
  double sps, cps, sth, cth, sph, cph, xx, xy;
  double xz, yx, yy, yz, zx, zy, zz, xi, yi, zi;
  
  // if we are rotating around X or Y the update the symmetry
  if(fabs(rox) + fabs(roy) > 1.0e-10)
    ctx->geometry.ipsym = ctx->geometry.ipsym * 3;
  
  sps = sin(rox);
  cps = cos(rox);
  sth = sin(roy);
  cth = cos(roy);
  sph = sin(roz);
  cph = cos(roz);
  xx = cph * cth;
  xy = cph * sth * sps - sph * cps;
  xz = cph * sth * cps + sph * sps;
  yx = sph * cth;
  yy = sph * sth * sps + cph * cps;
  yz = sph * sth * cps - cph * sps;
  zx = -sth;
  zy = cth * sps;
  zz = cth * cps;
  
  if(nrpt == 0)
    nrp = 1;
  else
    nrp = nrpt;
  
  // move the wires, if there are any
  ix = 1;
  if(ctx->geometry.n > 0) {
    int ir;
    int original_n = ctx->geometry.n;

    // get the first segment of this object
    i1 = segment_number(ctx, its, 1);
    if(i1 < 1)
      i1 = 1;

    ix = i1;
    if(nrpt == 0)
      k= i1-1;
    else {
      k = ctx->geometry.n;
      /* Reallocate tags buffer */
      mreq = (size_t)(ctx->geometry.n + ctx->geometry.m + (ctx->geometry.n + 1 - i1) * nrpt);
      mreq *= sizeof(int);
      mem_realloc(ctx, (void *)&ctx->geometry.tag_nums, mreq);

      /* Reallocate wire buffers */
      mreq = (size_t)(ctx->geometry.n + (ctx->geometry.n + 1 - i1) * nrpt);
      mreq *= sizeof(double);
      mem_realloc(ctx, (void *)&ctx->geometry.x1, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.y1, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.z1, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.x2, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.y2, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.z2, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.bi, mreq);
    }

    for(ir = 0; ir < nrp; ir++) {
      for(i = i1-1; i < original_n; i++)  {
        xi= ctx->geometry.x1[i];
        yi= ctx->geometry.y1[i];
        zi= ctx->geometry.z1[i];
        ctx->geometry.x1[k]= xi* xx+ yi* xy+ zi* xz+ xs;
        ctx->geometry.y1[k]= xi* yx+ yi* yy+ zi* yz+ ys;
        ctx->geometry.z1[k]= xi* zx+ yi* zy+ zi* zz+ zs;
        xi= ctx->geometry.x2[i];
        yi= ctx->geometry.y2[i];
        zi= ctx->geometry.z2[i];
        ctx->geometry.x2[k]= xi* xx+ yi* xy+ zi* xz+ xs;
        ctx->geometry.y2[k]= xi* yx+ yi* yy+ zi* yz+ ys;
        ctx->geometry.z2[k]= xi* zx+ yi* zy+ zi* zz+ zs;
        ctx->geometry.bi[k]= ctx->geometry.bi[i];
        ctx->geometry.tag_nums[k]= ctx->geometry.tag_nums[i];
        if(ctx->geometry.tag_nums[i] != 0)
          ctx->geometry.tag_nums[k]= ctx->geometry.tag_nums[i]+ tag_increment;

        k++;
      } /* for( i = i1; i < data.n; i++ ) */

      ctx->geometry.n = k;
    } /* for( ir = 0; ir < nrp; ir++ ) */
  } /* if( data.n >= n2) */
  
  // repeat the move for any patches
  if(ctx->geometry.m > 0) {
    int ii;
    int original_m = ctx->geometry.m;
    i1 = 0;
    if( nrpt == 0)
      k= 0;
    else
      k = ctx->geometry.m;

    /* Reallocate patch buffers */
    mreq = (size_t)(ctx->geometry.m * (nrpt + 1));
    mreq *= sizeof(double);
    mem_realloc(ctx, (void *)&ctx->geometry.px, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.py, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.pz, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.t1x, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.t1y, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.t1z, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.t2x, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.t2y, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.t2z, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.pbi, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.psalp, mreq);

    for( ii = 0; ii < nrp; ii++ ) {
      for( i = i1; i < original_m; i++ ) {
        xi= ctx->geometry.px[i];
        yi= ctx->geometry.py[i];
        zi= ctx->geometry.pz[i];
        ctx->geometry.px[k]= xi* xx+ yi* xy+ zi* xz+ xs;
        ctx->geometry.py[k]= xi* yx+ yi* yy+ zi* yz+ ys;
        ctx->geometry.pz[k]= xi* zx+ yi* zy+ zi* zz+ zs;
        xi= ctx->geometry.t1x[i];
        yi= ctx->geometry.t1y[i];
        zi= ctx->geometry.t1z[i];
        ctx->geometry.t1x[k]= xi* xx+ yi* xy+ zi* xz;
        ctx->geometry.t1y[k]= xi* yx+ yi* yy+ zi* yz;
        ctx->geometry.t1z[k]= xi* zx+ yi* zy+ zi* zz;
        xi= ctx->geometry.t2x[i];
        yi= ctx->geometry.t2y[i];
        zi= ctx->geometry.t2z[i];
        ctx->geometry.t2x[k]= xi* xx+ yi* xy+ zi* xz;
        ctx->geometry.t2y[k]= xi* yx+ yi* yy+ zi* yz;
        ctx->geometry.t2z[k]= xi* zx+ yi* zy+ zi* zz;
        ctx->geometry.psalp[k]= ctx->geometry.psalp[i];
        ctx->geometry.pbi[k]= ctx->geometry.pbi[i];
        k++;
      } /* for( i = i1; i < data.m; i++ ) */

      ctx->geometry.m = k;
    } /* for( ii = 0; ii < nrp; ii++ ) */

  } /* if( data.m >= m2) */
  
  // test whether we did a complete rotation/copy
  if((nrpt == 0) && (ix == 1))
    return;
  
  // otherwise, reset the symmetry flags to "none"
  ctx->geometry.np = ctx->geometry.n;
  ctx->geometry.mp = ctx->geometry.m;
  ctx->geometry.ipsym = 0;
} /* end of reproduce */

/******************************************************************************
 * reflect
 *
 * reflect (formerly reflc) creates new geometry entries for all existing
 * entries to create reflections across the selected axes. reflect can
 * duplicate across the X, Y and/or Z axes in a single operation. If the
 * original entries had a tag number, it will be updated by the tag_increment,
 * while those with a zero tag will remain zero.
 *
 * reflect formerly performed two separate functions, reflecting for GX cards
 * or rotating for GR cards. The code was entirely separate for these two
 * functions, controlled by a long if statement. It made no sense to leave
 * them combined, so the handler for the GR case has been split out into its
 * own function, rotate.
 *
 * @param card_num card_t number that contains this instruction
 * @param tag_increment the number to increment the tag by, see notes below
 * @param ix see iz
 * @param iy see iz
 * @param iz flags indicating whether to relect on this axis
 *
 */
void reflect(nec_context_t *ctx, int card_num, int tag_increment, int ix, int iy, int iz)
{
  int iti, i, nx, itagi;
  size_t mreq;
  double e1, e2;
  
  // sanity check, formerly used nop>0 but we no longer pass that in
  if(ix == 0 && iy == 0 && iz == 0) {
    char *msg = calloc(1, MAX_ERROR_LEN);
    sprintf(msg, "GX on card %d has no reflection axes.", card_num + 1);
    add_error(ctx, &ctx->geometry.errors, msg, 1);
    free(msg);
    return;
  }
  
  // we are going to create symmetry one way or the other,
  // so we copy down how much geometry is in the symmetry "cell"
  ctx->geometry.np = ctx->geometry.n;
  ctx->geometry.mp = ctx->geometry.m;
  iti = tag_increment;
  
  // both GR and GX cards use only the I1 and I2 inputs in the card. I1 is
  // passed in the tag_increment, and I2 in num_copies. However, the I2 value
  // means different things in the two cards, in the GR card is is the number
  // of times to make a copy of the wires, for the GX is is a flag saying which
  // axes to reflect along. Since the flag value is a value number of copies
  // value, the code that calls reflect copies the I2 value into the ix, iy and iz
  // so to indicate if we are performing
  
  // we are now symmetric
  // FIXME: the original code for this is confusing, this should be reviewed
  ctx->geometry.ipsym = 1;
  
  // reflect along z axis
  if(iz != 0) {
    ctx->geometry.ipsym = 2;
    
    // copy existing wires if there are any
    if(ctx->geometry.n > 0) {
      // reallocate cards and tags buffers
      mreq = (size_t)(2 * ctx->geometry.n + ctx->geometry.m);
      mreq *= sizeof(int);
      mem_realloc(ctx, (void *)&ctx->geometry.tag_nums, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.card_nums, mreq);
      
      // Reallocate wire buffers
      mreq = (size_t)(2 * ctx->geometry.n);
      mreq *= sizeof(double);
      mem_realloc(ctx, (void *)&ctx->geometry.x1, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.y1, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.z1, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.x2, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.y2, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.z2, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.bi, mreq);
      
      for(i = 0; i < ctx->geometry.n; i++) {
        // get the existing segment number and
        nx = i + ctx->geometry.n;
        
        // get the existing z end points and test them
        e1 = ctx->geometry.z1[i];
        e2 = ctx->geometry.z2[i];
        
        if((fabs(e1) + fabs(e2) <= 1.0e-5) || (e1 * e2 < -1.0e-6)) {
          char *msg = calloc(1, MAX_ERROR_LEN);
          sprintf(msg,
                  "\n  GEOMETRY DATA ERROR--SEGMENT %d"
                  " LIES IN PLANE OF SYMMETRY",
                  i + 1);
          add_error(ctx, &ctx->geometry.errors, msg, 1);
          free(msg);
          return;
        }
        
        ctx->geometry.x1[nx] = ctx->geometry.x1[i];
        ctx->geometry.y1[nx] = ctx->geometry.y1[i];
        ctx->geometry.z1[nx] = -e1;
        ctx->geometry.x2[nx] = ctx->geometry.x2[i];
        ctx->geometry.y2[nx] = ctx->geometry.y2[i];
        ctx->geometry.z2[nx] = -e2;
        
        // get the last used tag num
        itagi = ctx->geometry.tag_nums[i];
        
        // now set the tag of the new entries to zero or that offset
        if(itagi == 0)
          ctx->geometry.tag_nums[nx] = 0;
        if(itagi != 0)
          ctx->geometry.tag_nums[nx]= itagi + iti;
        
        ctx->geometry.bi[nx]= ctx->geometry.bi[i];
      } /* for( i = 0; i < data.n; i++ ) */
      
      // and that means the amount of geometry has doubled
      ctx->geometry.n = ctx->geometry.n * 2;
      
      // and that if we make more entries they need to be
      // offset by a greater number
      iti = iti * 2;
    } /* if( geomtry.n > 0) */
    
    // and now the patches, if there are any
    if(ctx->geometry.m > 0) {
      /* Reallocate patch buffers */
      mreq = (size_t)(2 * ctx->geometry.m);
      mreq *= sizeof(double);
      mem_realloc(ctx, (void *)&ctx->geometry.px, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.py, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.pz, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.t1x, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.t1y, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.t1z, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.t2x, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.t2y, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.t2z, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.pbi, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.psalp, mreq);
      
      for(i = 0; i < ctx->geometry.m; i++) {
        nx = i+ctx->geometry.m;
        if(fabs(ctx->geometry.pz[i]) <= 1.0e-10) {
          char *msg = calloc(1, MAX_ERROR_LEN);
          sprintf(msg,
                  "\n  GEOMETRY DATA ERROR--PATCH %d"
                  " LIES IN PLANE OF SYMMETRY",
                  i + 1);
          add_error(ctx, &ctx->geometry.errors, msg, 1);
          free(msg);
          return;
        }
        
        ctx->geometry.px[nx]= ctx->geometry.px[i];
        ctx->geometry.py[nx]= ctx->geometry.py[i];
        ctx->geometry.pz[nx]= -ctx->geometry.pz[i];
        ctx->geometry.t1x[nx]= ctx->geometry.t1x[i];
        ctx->geometry.t1y[nx]= ctx->geometry.t1y[i];
        ctx->geometry.t1z[nx]= -ctx->geometry.t1z[i];
        ctx->geometry.t2x[nx]= ctx->geometry.t2x[i];
        ctx->geometry.t2y[nx]= ctx->geometry.t2y[i];
        ctx->geometry.t2z[nx]= -ctx->geometry.t2z[i];
        ctx->geometry.psalp[nx]= -ctx->geometry.psalp[i];
        ctx->geometry.pbi[nx]= ctx->geometry.pbi[i];
      }
      
      ctx->geometry.m= ctx->geometry.m*2;
    } /* if( data.m >= m2) */
  } /* if( iz != 0) */
  
  // now repeat all of that for the y-axis
  if(iy != 0) {
    if(ctx->geometry.n > 0) {
      /* Reallocate tags buffer */
      mreq = (size_t)(2 * ctx->geometry.n + ctx->geometry.m);
      mreq *= sizeof(int);
      mem_realloc(ctx, (void *)&ctx->geometry.tag_nums, mreq);
      
      /* Reallocate wire buffers */
      mreq = (size_t)(2 * ctx->geometry.n);
      mreq *= sizeof(double);
      mem_realloc(ctx, (void *)&ctx->geometry.x1, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.y1, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.z1, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.x2, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.y2, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.z2, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.bi, mreq);
      
      for(i = 0; i < ctx->geometry.n; i++) {
        nx= i+ ctx->geometry.n;
        e1= ctx->geometry.y1[i];
        e2= ctx->geometry.y2[i];
        
        if((fabs(e1)+fabs(e2) <= 1.0e-5) || (e1*e2 < -1.0e-6)) {
          char *msg = calloc(1, MAX_ERROR_LEN);
          sprintf(msg,
                  "\n  GEOMETRY DATA ERROR--SEGMENT %d"
                  " LIES IN PLANE OF SYMMETRY",
                  i + 1);
          add_error(ctx, &ctx->geometry.errors, msg, 1);
          free(msg);
          return;
        }
        
        ctx->geometry.x1[nx] = ctx->geometry.x1[i];
        ctx->geometry.y1[nx] = -e1;
        ctx->geometry.z1[nx] = ctx->geometry.z1[i];
        ctx->geometry.x2[nx] = ctx->geometry.x2[i];
        ctx->geometry.y2[nx] = -e2;
        ctx->geometry.z2[nx] = ctx->geometry.z2[i];
        itagi = ctx->geometry.tag_nums[i];
        
        if( itagi == 0)
          ctx->geometry.tag_nums[nx]=0;
        if( itagi != 0)
          ctx->geometry.tag_nums[nx]= itagi+ iti;
        
        ctx->geometry.bi[nx]= ctx->geometry.bi[i];
        
      } /* for( i = n2-1; i < data.n; i++ ) */
      
      ctx->geometry.n= ctx->geometry.n*2;
      iti= iti*2;
      
    } /* if( data.n >= n2) */
    
    // reflect any patches
    if(ctx->geometry.m > 0)  {
      // reflection doubles the number of patches, so we start
      // by reallocating the patch list to hold the new ones
      mreq = (size_t)(2 * ctx->geometry.m);
      mreq *= sizeof(double);
      mem_realloc(ctx, (void *)&ctx->geometry.px, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.py, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.pz, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.t1x, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.t1y, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.t1z, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.t2x, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.t2y, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.t2z, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.pbi, mreq);
      mem_realloc(ctx, (void *)&ctx->geometry.psalp, mreq);
      
      for( i = 0; i < ctx->geometry.m; i++ ) {
        nx= i+ctx->geometry.m;
        if( fabs( ctx->geometry.py[i]) <= 1.0e-10) {
          char *msg = calloc(1, MAX_ERROR_LEN);
          sprintf(msg,
                  "\n  GEOMETRY DATA ERROR--PATCH %d"
                  " LIES IN PLANE OF SYMMETRY",
                  i + 1);
          add_error(ctx, &ctx->geometry.errors, msg, 1);
          free(msg);
          return;
        }
        
        ctx->geometry.px[nx]= ctx->geometry.px[i];
        ctx->geometry.py[nx]= -ctx->geometry.py[i];
        ctx->geometry.pz[nx]= ctx->geometry.pz[i];
        ctx->geometry.t1x[nx]= -ctx->geometry.t1x[i];
        ctx->geometry.t1y[nx]= ctx->geometry.t1y[i];
        ctx->geometry.t1z[nx]= ctx->geometry.t1z[i];
        ctx->geometry.t2x[nx]= -ctx->geometry.t2x[i];
        ctx->geometry.t2y[nx]= ctx->geometry.t2y[i];
        ctx->geometry.t2z[nx]= ctx->geometry.t2z[i];
        ctx->geometry.psalp[nx]= -ctx->geometry.psalp[i];
        ctx->geometry.pbi[nx]= ctx->geometry.pbi[i];
        
      } /* for( i = m2; i <= ctx->geometry.m; i++ ) */
      
      ctx->geometry.m= ctx->geometry.m * 2;
    } /* if( ctx->geometry.m >= m2) */
  } /* if( iy != 0) */
  
  // and finally the x axis
  if(ix == 0)
    return;
  
  if( ctx->geometry.n > 0 ) {
    /* Reallocate tags buffer */
    mreq = (size_t)(2 * ctx->geometry.n + ctx->geometry.m);
    mreq *= sizeof(int);
    mem_realloc(ctx, (void *)&ctx->geometry.tag_nums, mreq);
    
    /* Reallocate wire buffers */
    mreq = (size_t)(2 * ctx->geometry.n);
    mreq *= sizeof(double);
    mem_realloc(ctx, (void *)&ctx->geometry.x1, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.y1, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.z1, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.x2, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.y2, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.z2, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.bi, mreq);
    
    for(i = 0; i < ctx->geometry.n; i++) {
      nx= i+ ctx->geometry.n;
      e1= ctx->geometry.x1[i];
      e2= ctx->geometry.x2[i];
      
      if( (fabs(e1)+fabs(e2) <= 1.0e-5) || (e1*e2 < -1.0e-6) ) {
        char *msg = calloc(1, MAX_ERROR_LEN);
        sprintf(msg,
                "\n  GEOMETRY DATA ERROR--SEGMENT %d"
                " LIES IN PLANE OF SYMMETRY",
                i + 1);
        add_error(ctx, &ctx->geometry.errors, msg, 1);
        free(msg);
        return;
      }
      
      ctx->geometry.x1[nx]= -e1;
      ctx->geometry.y1[nx]= ctx->geometry.y1[i];
      ctx->geometry.z1[nx]= ctx->geometry.z1[i];
      ctx->geometry.x2[nx]= -e2;
      ctx->geometry.y2[nx]= ctx->geometry.y2[i];
      ctx->geometry.z2[nx]= ctx->geometry.z2[i];
      itagi= ctx->geometry.tag_nums[i];
      
      if(itagi == 0)
        ctx->geometry.tag_nums[nx]=0;
      if(itagi != 0)
        ctx->geometry.tag_nums[nx]= itagi + iti;
      
      ctx->geometry.bi[nx]= ctx->geometry.bi[i];
    }
    
    ctx->geometry.n= ctx->geometry.n*2;
    
  } /* if( data.n > 0) */
  
  if(ctx->geometry.m == 0)
    return;
  
  /* Reallocate patch buffers */
  mreq = (size_t)(2 * ctx->geometry.m);
  mreq *= sizeof(double);
  mem_realloc(ctx, (void *)&ctx->geometry.px, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.py, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.pz, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.t1x, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.t1y, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.t1z, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.t2x, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.t2y, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.t2z, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.pbi, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.psalp, mreq);
  
  for( i = 0; i < ctx->geometry.m; i++ ) {
    nx = i+ctx->geometry.m;
    if(fabs(ctx->geometry.px[i]) <= 1.0e-10) {
      char *msg = calloc(1, MAX_ERROR_LEN);
      sprintf(msg,
              "\n  GEOMETRY DATA ERROR--PATCH %d"
              " LIES IN PLANE OF SYMMETRY",
              i + 1);
      add_error(ctx, &ctx->geometry.errors, msg, 1);
      free(msg);
      return;
    }
    
    ctx->geometry.px[nx]= -ctx->geometry.px[i];
    ctx->geometry.py[nx]= ctx->geometry.py[i];
    ctx->geometry.pz[nx]= ctx->geometry.pz[i];
    ctx->geometry.t1x[nx]= -ctx->geometry.t1x[i];
    ctx->geometry.t1y[nx]= ctx->geometry.t1y[i];
    ctx->geometry.t1z[nx]= ctx->geometry.t1z[i];
    ctx->geometry.t2x[nx]= -ctx->geometry.t2x[i];
    ctx->geometry.t2y[nx]= ctx->geometry.t2y[i];
    ctx->geometry.t2z[nx]= ctx->geometry.t2z[i];
    ctx->geometry.psalp[nx]= -ctx->geometry.psalp[i];
    ctx->geometry.pbi[nx]= ctx->geometry.pbi[i];
  }
  
  ctx->geometry.m= ctx->geometry.m * 2;
} /* end of reflect */

/******************************************************************************
 * rotate
 *
 * rotate creates new geometry entries for all existing entries to create
 * a rotation around the Z axis. If the original entries had a tag number,
 * it will be updated by the tag_increment, while those with a zero tag will
 * remain zero.
 *
 * rotate was formerly part of reflect, although the code was entirely
 * separate, so it has been moved it its own function for clarity.
 *
 * @param card_num card_t number that contains this instruction
 * @param tag_increment the number to increment the tag by, see notes below
 * @param num_copies number of new copies to produce
 *
 */
void rotate(nec_context_t *ctx, int card_num, int tag_increment, int num_copies)
{
  int nx, itagi, k;
  size_t mreq;
  double fnop, sam, cs, ss, xk, yk;
  
  // we are going to create symmetry around the Z axis
  ctx->geometry.np = ctx->geometry.n;
  ctx->geometry.mp = ctx->geometry.m;
  ctx->geometry.ipsym = -1;      // rotational symmetry
  
  // reproduce structure with rotation to form cylindrical structure
  fnop = (double)num_copies;
  sam = TP / fnop;
  cs = cos(sam);
  ss = sin(sam);
  
  if(ctx->geometry.n > 0) {
    ctx->geometry.n *= num_copies;
    nx = ctx->geometry.np;
    
    //r eallocate cards and tags buffers
    mreq = (size_t)(ctx->geometry.n + ctx->geometry.m);
    mreq *= sizeof(int);
    mem_realloc(ctx, (void *)&ctx->geometry.tag_nums, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.card_nums, mreq);
    
    // reallocate wire buffers
    mreq = (size_t)ctx->geometry.n;
    mreq *= sizeof(double);
    mem_realloc(ctx, (void *)&ctx->geometry.x1, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.y1, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.z1, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.x2, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.y2, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.z2, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.bi, mreq);
    
    for(int i = nx; i < ctx->geometry.n; i++ ) {
      k= i - ctx->geometry.np;
      xk = ctx->geometry.x1[k];
      yk = ctx->geometry.y1[k];
      ctx->geometry.x1[i]= xk* cs- yk* ss;
      ctx->geometry.y1[i]= xk* ss+ yk* cs;
      ctx->geometry.z1[i]= ctx->geometry.z1[k];
      xk= ctx->geometry.x2[k];
      yk= ctx->geometry.y2[k];
      ctx->geometry.x2[i]= xk* cs- yk* ss;
      ctx->geometry.y2[i]= xk* ss+ yk* cs;
      ctx->geometry.z2[i]= ctx->geometry.z2[k];
      ctx->geometry.bi[i]= ctx->geometry.bi[k];
      itagi= ctx->geometry.tag_nums[k];
      
      if(itagi == 0)
        ctx->geometry.tag_nums[i] = 0;
      if( itagi != 0)
        ctx->geometry.tag_nums[i] = itagi + tag_increment;
      
      ctx->geometry.card_nums[i] = card_num;
    }
  } /* if( data.n >= n2) */
  
  // now do it all again for the patches if there are any
  // FIXME: this doesn't see to record tag or card numbers, did that happen above?
  if(ctx->geometry.m == 0)
    return;
  
  ctx->geometry.m *= num_copies;
  nx = ctx->geometry.mp;
  
  /* Reallocate patch buffers */
  mreq = (size_t)ctx->geometry.m;
  mreq *= sizeof(double);
  mem_realloc(ctx, (void *)&ctx->geometry.px, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.py, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.pz, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.t1x, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.t1y, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.t1z, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.t2x, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.t2y, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.t2z, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.pbi, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.psalp, mreq);
  
  for(int i = nx; i < ctx->geometry.m; i++) {
    k = i-ctx->geometry.mp;
    xk= ctx->geometry.px[k];
    yk= ctx->geometry.py[k];
    ctx->geometry.px[i]= xk* cs- yk* ss;
    ctx->geometry.py[i]= xk* ss+ yk* cs;
    ctx->geometry.pz[i]= ctx->geometry.pz[k];
    xk= ctx->geometry.t1x[k];
    yk= ctx->geometry.t1y[k];
    ctx->geometry.t1x[i]= xk* cs- yk* ss;
    ctx->geometry.t1y[i]= xk* ss+ yk* cs;
    ctx->geometry.t1z[i]= ctx->geometry.t1z[k];
    xk= ctx->geometry.t2x[k];
    yk= ctx->geometry.t2y[k];
    ctx->geometry.t2x[i]= xk* cs- yk* ss;
    ctx->geometry.t2y[i]= xk* ss+ yk* cs;
    ctx->geometry.t2z[i]= ctx->geometry.t2z[k];
    ctx->geometry.psalp[i]= ctx->geometry.psalp[k];
    ctx->geometry.pbi[i]= ctx->geometry.pbi[k];
  } /* for( i = nx; i < data.m; i++ ) */
} /* end of rotate */

/******************************************************************************
 * patch
 *
 * patch creates a surface patch of one of three types, including a free-form
 * surface that is defined on multiple cards.
 *
 * FIXME: this should be broken into two methods, single_patch and multi_
 *
 * @param card_num card_t number that contains this instruction
 * @param nx the number of patches to generate in x...
 * @param ny ... and y.
 *
 */
void patch(nec_context_t *ctx, int card_num, int nx, int ny,
           double ax1, double ay1, double az1,
           double ax2, double ay2, double az2,
           double ax3, double ay3, double az3,
           double ax4, double ay4, double az4)
{
  int mi, ntp;
  size_t mreq;
  double s1x=0.0, s1y=0.0, s1z=0.0, s2x=0.0, s2y=0.0, s2z=0.0, xst=0.0;
  double znv, xnv, ynv, xa, xn2, yn2, zn2;
  
  // new patches. for nx=0, ny=1,2,3,4 patch is (respectively)
  // arbitrary, rectangular, triangular, or quadrilateral.
  // for nx and ny > 0 a rectangular surface is produced with
  // nx by ny rectangular patches.
  
  ctx->geometry.m++;
  mi = ctx->geometry.m - 1;
  
  // reallocate patch buffers
  mreq = (size_t)ctx->geometry.m;
  mreq *= sizeof(double);
  mem_realloc(ctx, (void *)&ctx->geometry.px, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.py, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.pz, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.t1x, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.t1y, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.t1z, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.t2x, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.t2y, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.t2z, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.pbi, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.psalp, mreq);
  
  if(nx > 0)
    ntp = 2;
  else
    ntp = ny;
  
  if(ntp <= 1) {
    ctx->geometry.px[mi] = ax1;
    ctx->geometry.py[mi] = ay1;
    ctx->geometry.pz[mi] = az1;
    ctx->geometry.pbi[mi] = az2;
    znv = cos(ax2);
    xnv = znv * cos(ay2);
    ynv = znv * sin(ay2);
    znv = sin(ax2);
    xa = sqrt(xnv * xnv+ ynv * ynv);
    
    if(xa >= 1.0e-6) {
      ctx->geometry.t1x[mi] = -ynv/ xa;
      ctx->geometry.t1y[mi] = xnv/ xa;
      ctx->geometry.t1z[mi] = 0.0;
    } else {
      ctx->geometry.t1x[mi]=1.;
      ctx->geometry.t1y[mi]=0.;
      ctx->geometry.t1z[mi]=0.;
    }
    
  } /* if( ntp <= 1) */
  else {
    s1x = ax2 - ax1;
    s1y = ay2 - ay1;
    s1z = az2 - az1;
    s2x = ax3 - ax2;
    s2y = ay3 - ay2;
    s2z = az3 - az2;
    
    if(nx != 0) {
      s1x = s1x / nx;
      s1y = s1y / nx;
      s1z = s1z / nx;
      s2x = s2x / ny;
      s2y = s2y / ny;
      s2z = s2z / ny;
    }
    
    xnv = s1y * s2z - s1z * s2y;
    ynv = s1z * s2x - s1x * s2z;
    znv = s1x * s2y - s1y * s2x;
    xa = sqrt(xnv * xnv + ynv * ynv + znv * znv);
    xnv = xnv/ xa;
    ynv = ynv/ xa;
    znv = znv/ xa;
    xst = sqrt( s1x* s1x+ s1y* s1y+ s1z* s1z);
    ctx->geometry.t1x[mi] = s1x / xst;
    ctx->geometry.t1y[mi] = s1y / xst;
    ctx->geometry.t1z[mi] = s1z / xst;
    
    if(ntp <= 2) {
      ctx->geometry.px[mi] = ax1 + 0.5 * (s1x + s2x);
      ctx->geometry.py[mi] = ay1 + 0.5 * (s1y + s2y);
      ctx->geometry.pz[mi] = az1 + 0.5 * (s1z + s2z);
      ctx->geometry.pbi[mi] = xa;
    }
    else {
      if( ntp != 4) {
        ctx->geometry.px[mi] = (ax1 + ax2 + ax3) / 3.0;
        ctx->geometry.py[mi] = (ay1 + ay2 + ay3) / 3.0;
        ctx->geometry.pz[mi] = (az1 + az2 + az3) / 3.0;
        ctx->geometry.pbi[mi] = 0.5 * xa;
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
        ctx->geometry.px[mi]=( xa*( ax1+ ax2+ ax3)+ xst*( ax1+ ax3+ ax4))* salpn;
        ctx->geometry.py[mi]=( xa*( ay1+ ay2+ ay3)+ xst*( ay1+ ay3+ ay4))* salpn;
        ctx->geometry.pz[mi]=( xa*( az1+ az2+ az3)+ xst*( az1+ az3+ az4))* salpn;
        ctx->geometry.pbi[mi]=.5*( xa+ xst);
        s1x=( xnv* xn2+ ynv* yn2+ znv* zn2)/ xst;
        
        if(s1x <= 0.9998) {
          char *msg = calloc(1, MAX_ERROR_LEN);
          sprintf(msg,
                  "\n  ERROR -- CORNERS OF QUADRILATERAL"
                  " PATCH DO NOT LIE IN A PLANE" );
          add_error(ctx, &ctx->geometry.errors, msg, 1);
          free(msg);
          return;
        }
      } /* if( ntp != 4) */
    } /* if( ntp <= 2) */
  } /* if( ntp <= 1) */
  
  ctx->geometry.t2x[mi] = ynv * ctx->geometry.t1z[mi] - znv * ctx->geometry.t1y[mi];
  ctx->geometry.t2y[mi] = znv * ctx->geometry.t1x[mi] - xnv * ctx->geometry.t1z[mi];
  ctx->geometry.t2z[mi] = xnv * ctx->geometry.t1y[mi] - ynv * ctx->geometry.t1x[mi];
  ctx->geometry.psalp[mi] = 1.0;
  
  if(nx != 0) {
    int iy, ix;
    double xs, ys, zs, xt, yt, zt;
    
    ctx->geometry.m += nx * ny - 1;
    // reallocate patch buffers
    mreq = (size_t)ctx->geometry.m;
    mreq *= sizeof(double);
    mem_realloc(ctx, (void *)&ctx->geometry.px, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.py, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.pz, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.t1x, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.t1y, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.t1z, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.t2x, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.t2y, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.t2z, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.pbi, mreq);
    mem_realloc(ctx, (void *)&ctx->geometry.psalp, mreq);
    
    xn2 = ctx->geometry.px[mi] - s1x - s2x;
    yn2 = ctx->geometry.py[mi] - s1y - s2y;
    zn2 = ctx->geometry.pz[mi] - s1z - s2z;
    xs = ctx->geometry.t1x[mi];
    ys = ctx->geometry.t1y[mi];
    zs = ctx->geometry.t1z[mi];
    xt = ctx->geometry.t2x[mi];
    yt = ctx->geometry.t2y[mi];
    zt = ctx->geometry.t2z[mi];
    
    for(iy = 0; iy < ny; iy++) {
      xn2 += s2x;
      yn2 += s2y;
      zn2 += s2z;
      
      for(ix = 1; ix <= nx; ix++) {
        xst= (double)ix;
        ctx->geometry.px[mi] = xn2+ xst* s1x;
        ctx->geometry.py[mi] = yn2+ xst* s1y;
        ctx->geometry.pz[mi] = zn2+ xst* s1z;
        ctx->geometry.pbi[mi] = xa;
        ctx->geometry.psalp[mi] =1.;
        ctx->geometry.t1x[mi] = xs;
        ctx->geometry.t1y[mi] = ys;
        ctx->geometry.t1z[mi] = zs;
        ctx->geometry.t2x[mi] = xt;
        ctx->geometry.t2y[mi] = yt;
        ctx->geometry.t2z[mi] = zt;
        mi++;
      } /* for( ix = 0; ix < nx; ix++ ) */
    } /* for( iy = 0; iy < ny; iy++ ) */
  } /* if( nx != 0) */
  
  // reset symmetry
  // TODO: why is this at the end? other methods have it at the top
  ctx->geometry.ipsym = 0;
  ctx->geometry.np = ctx->geometry.n;
  ctx->geometry.mp = ctx->geometry.m;
} /* end of patch */

/******************************************************************************
 * calculate_patch (formerly subph) was an entry point (part of)
 * patch()
 *
 */
void calculate_patch(nec_context_t *ctx, int nx, int ny )
{
  int mia, ix, iy, mi;
  size_t mreq;
  double xs, ys, zs, xa, xst, s1x, s1y, s1z, s2x, s2y, s2z, saln, xt, yt;
  
  // reallocate patch buffers
  if(ny == 0) {
    ctx->geometry.m += 3;
  } else {
    ctx->geometry.m += 4;
  }
  
  mreq = (size_t)ctx->geometry.m;
  mreq *= sizeof(double);
  mem_realloc(ctx, (void *)&ctx->geometry.px, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.py, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.pz, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.t1x, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.t1y, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.t1z, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.t2x, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.t2y, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.t2z, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.pbi, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.psalp, mreq);
  mreq = (size_t)(ctx->geometry.n + ctx->geometry.m);
  mreq *= sizeof(int);
  mem_realloc(ctx, (void *)&ctx->geometry.icon1, mreq);
  mem_realloc(ctx, (void *)&ctx->geometry.icon2, mreq);
  
  // shift patches to make room for new ones
  if((ny == 0) && (nx != ctx->geometry.m))  {
    for(iy = ctx->geometry.m - 1; iy > nx+2; iy--) {
      ix = iy-3;
      ctx->geometry.px[iy]= ctx->geometry.px[ix];
      ctx->geometry.py[iy]= ctx->geometry.py[ix];
      ctx->geometry.pz[iy]= ctx->geometry.pz[ix];
      ctx->geometry.pbi[iy]= ctx->geometry.pbi[ix];
      ctx->geometry.psalp[iy]= ctx->geometry.psalp[ix];
      ctx->geometry.t1x[iy]= ctx->geometry.t1x[ix];
      ctx->geometry.t1y[iy]= ctx->geometry.t1y[ix];
      ctx->geometry.t1z[iy]= ctx->geometry.t1z[ix];
      ctx->geometry.t2x[iy]= ctx->geometry.t2x[ix];
      ctx->geometry.t2y[iy]= ctx->geometry.t2y[ix];
      ctx->geometry.t2z[iy]= ctx->geometry.t2z[ix];
    }
  } /* if( (ny == 0) || (nx != m) ) */
  
  /* divide patch for connection */
  mi = nx-1;
  xs = ctx->geometry.px[mi];
  ys = ctx->geometry.py[mi];
  zs = ctx->geometry.pz[mi];
  xa = ctx->geometry.pbi[mi] / 4.0;
  xst = sqrt(xa) / 2.0;
  s1x = ctx->geometry.t1x[mi];
  s1y = ctx->geometry.t1y[mi];
  s1z = ctx->geometry.t1z[mi];
  s2x = ctx->geometry.t2x[mi];
  s2y = ctx->geometry.t2y[mi];
  s2z = ctx->geometry.t2z[mi];
  saln = ctx->geometry.psalp[mi];
  xt = xst;
  yt = xst;
  
  if(ny == 0)
    mia= mi;
  else {
    ctx->geometry.mp++;
    mia = ctx->geometry.m - 1;
  }
  
  for(ix = 1; ix <= 4; ix++) {
    ctx->geometry.px[mia]= xs+ xt* s1x+ yt* s2x;
    ctx->geometry.py[mia]= ys+ xt* s1y+ yt* s2y;
    ctx->geometry.pz[mia]= zs+ xt* s1z+ yt* s2z;
    ctx->geometry.pbi[mia]= xa;
    ctx->geometry.t1x[mia]= s1x;
    ctx->geometry.t1y[mia]= s1y;
    ctx->geometry.t1z[mia]= s1z;
    ctx->geometry.t2x[mia]= s2x;
    ctx->geometry.t2y[mia]= s2y;
    ctx->geometry.t2z[mia]= s2z;
    ctx->geometry.psalp[mia]= saln;
    
    if(ix == 2)
      yt= -yt;
    
    if((ix == 1) || (ix == 3))
      xt= -xt;
    
    mia++;
  }
  
  if(nx <= ctx->geometry.mp)
    ctx->geometry.mp += 3;
  
  if(ny > 0)
    ctx->geometry.pz[mi] = 10000.0;
} /* end of calculate_patch */

/* end of ctx->geometry.c */
