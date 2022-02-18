/*******************************************************************
 * geometry.c
 *
 * geometry.c contains the code that parses the geometry section of
 * the deck and then generates a list of segments, patches, and
 * connections. These are collected into a geometry_t structure for
 * the deck.
 *
*******************************************************************/

#include "opennec.h"
#include "shared.h"

/******************************************************************************
 * calculate_geometry
 *
 * calculate_geometry (formerly datagn) is the main routine for creation
 * of geometry data. It reads the geometry cards, builds segments and patches,
 * returns various errors, and when it reaches the end of the geometry it
 * calls the various worker functions to calculate the segment and patch data.
 * The resulting data, in geometry, can then be used to draw a diagram of the
 *  antenna.
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
	int ix, ns;
	double rad, xs1, xs2, ys1, ys2, zs1, zs2, x4 = 0, y4 = 0, z4 = 0;
	double x3 = 0, y3 = 0, z3 = 0, xw1, xw2, yw1, yw2, zw1, zw2;
	//double dummy;

  // set up the counters
	geometry.ipsym = 0;
	num_wires = 0;
	geometry.n = 0;
	geometry.np = 0;
	geometry.m = 0;
	geometry.mp = 0;
	isct = 0;     // this is "I am looking for an SC card"
	iphd = FALSE;	// this is "I printed the header"
  
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
    for(gm_num = 0; gm_num < NUM_GEOMETRY_CODES; gm_num++) {
      if(strncmp(card.card_code, geometry_codes[gm_num], 2) == 0) break;
    }
    // now read in the values, which are the same for all the cards
    //parse_geometry_card(gm, &itg, &ns, &xw1, &yw1, &zw1, &xw2, &yw2, &zw2, &rad);
    itg = card.i[1];
    ns = card.i[2];
    xw1 = card.f[1];
    yw1 = card.f[2];
    zw1 = card.f[3];
    xw2 = card.f[4];
    yw2 = card.f[5];
    zw2 = card.f[6];

    // and now the switch. basically all this does is call the appropriate
    // function to insert the segments for that card type
    switch(gm_num) {
        
      case 0: // GW, make a wire
        // the radius could be in the f7 field, or it could be on the next card if its tapered
        if(card.f[7] != 0.0) {
          rad = card.f[7];
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
          if(deck->cards[i + 1].f[2] == 0.0 || deck->cards[i + 1].f[3] == 0.0) {
            sprintf(msg, "Card %d is a GC with tapering info for GW in card %d, but there is a zero in Y1 or Z1.", i + 1, i);
            add_error(errors, msg, 1);
            continue;
          }
          // override the original inputs with the ones from the GC
          xs1 = deck->cards[i + 1].f[1];  // check this!
          ys1 = deck->cards[i + 1].f[2];
          zs1 = deck->cards[i + 1].f[3];
          rad = ys1;
          ys1 = pow((zs1 / ys1), (1. / (ns - 1.)));
          
          // move up a card so we don't process the GC
          i++;
        }
        
        // update the number of wires and the segment counts
        num_wires++;
        i1 = geometry.n + 1;
        i2 = geometry.n + ns;
        
        // now we have all the data, so turn it into segments
        wire(i, itg, ns, xw1, yw1, zw1, xw2, yw2, zw2, rad, xs1, ys1);
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
        if(geometry.n != 0) {
          /* reallocate the buffers */
          mreq = (size_t)geometry.n * sizeof(double);
          mem_realloc((void *)&geometry.si, mreq);
          mem_realloc((void *)&geometry.sab, mreq);
          mem_realloc((void *)&geometry.cab, mreq);
          mem_realloc((void *)&geometry.salp, mreq);
          mem_realloc((void *)&geometry.x, mreq);
          mem_realloc((void *)&geometry.y, mreq);
          mem_realloc((void *)&geometry.z, mreq);
          
          // and run the calcs
          for(i = 0; i < geometry.n; i++) {
            xw1 = geometry.x2[i] - geometry.x1[i];
            yw1 = geometry.y2[i] - geometry.y1[i];
            zw1 = geometry.z2[i] - geometry.z1[i];
            geometry.x[i] = (geometry.x1[i] + geometry.x2[i]) / 2.;
            geometry.y[i] = (geometry.y1[i] + geometry.y2[i]) / 2.;
            geometry.z[i] = (geometry.z1[i] + geometry.z2[i]) / 2.;
            xw2 = xw1* xw1 + yw1* yw1 + zw1* zw1;
            yw2 = sqrt(xw2);
            yw2 = (xw2 / yw2 + yw2)*.5;
            geometry.si[i] = yw2;
            geometry.cab[i] = xw1 / yw2;
            geometry.sab[i] = yw1 / yw2;
            xw2 = zw1 / yw2;
            
            if(xw2 > 1.) xw2 = 1.;
            if(xw2 < -1.) xw2 = -1.;
            
            geometry.salp[i] = xw2;
            xw2 = asin(xw2)* TD;
            yw2 = atan2(yw1, xw1)* TD;
            
            if(geometry.si[i] <= 1.e-20) {
                sprintf(msg, "The length of segment %d is too small to process.", i);
                add_error(errors, msg, 1);
            }
            if(geometry.bi[i] <= 0.) {
              sprintf(msg, "The radius of segment %d is too small to process.", i);
              add_error(errors, msg, 1);
            }
          } /* for( i = 0; i < data.n; i++ ) */
        } /* if( data.n != 0) */
        
        // and finally, do the same basic calculations for the patches
        // print out the patch data if there is any
        if (geometry.m != 0) {
          // don't really need the if, because nothing seems to be realloced
          for(i = 0; i < geometry.m; i++) {
            xw1 = (geometry.t1y[i] * geometry.t2z[i] - geometry.t1z[i] * geometry.t2y[i])* geometry.psalp[i];
            yw1 = (geometry.t1z[i] * geometry.t2x[i] - geometry.t1x[i] * geometry.t2z[i])* geometry.psalp[i];
            zw1 = (geometry.t1x[i] * geometry.t2y[i] - geometry.t1y[i] * geometry.t2x[i])* geometry.psalp[i];
          } /* for( i = 0; i < data.m; i++ ) */
        } /* if( data.m == 0) */
        
        // update the counters that track the total number of segments and patches
        geometry.npm = geometry.n + geometry.m;
        geometry.np2m = geometry.n + 2 * geometry.m;
        geometry.np3m = geometry.n + 3 * geometry.m;
        // and in this case, we're done
        return;
        
      case 5: /* "gm" card, move structure or reproduce/duplicate original structure in new positions */
        xw1 = xw1 * TA;
        yw1 = yw1 * TA;
        zw1 = zw1 * TA;
        
        // convert the original float value in F7 to int
        int its = (int)(card.f[7] + .5);
        
        duplicate(xw1, yw1, zw1, xw2, yw2, zw2, its, ns, itg);
        continue;
        
      case 6: /* "sp" card, generate single new patch or a series of patches with SC */
        i1 = geometry.m + 1;
        
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
            x3 = deck->cards[i + 1].f[1];
            y3 = deck->cards[i + 1].f[2];
            z3 = deck->cards[i + 1].f[3];
            i++; // skip the SC card next time through the loop
            patch(itg, ns, xw1, yw1, zw1, xw2, yw2, zw2, x3, y3, z3, 0.0, 0.0, 0.0);
          } /* ns == 2 */
          // if it's not a triangle, we have to loop over the following cards
          else {
            x3 = deck->cards[i + 1].f[1];
            y3 = deck->cards[i + 1].f[2];
            z3 = deck->cards[i + 1].f[3];
            x3 = deck->cards[i + 1].f[4];
            y3 = deck->cards[i + 1].f[5];
            z3 = deck->cards[i + 1].f[6];
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
              x3 = deck->cards[i + 1].f[1];
              y3 = deck->cards[i + 1].f[2];
              z3 = deck->cards[i + 1].f[3];
              x3 = deck->cards[i + 1].f[4];
              y3 = deck->cards[i + 1].f[5];
              z3 = deck->cards[i + 1].f[6];
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
        i1 = geometry.m + 1;
  
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
        x3 = deck->cards[i + 1].f[1];
        y3 = deck->cards[i + 1].f[2];
        z3 = deck->cards[i + 1].f[3];
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
        i1 = geometry.n + 1;
        i2 = geometry.n + ns;
  
//        fprintf(output_fp, "\n"
//          " %5d ARC RADIUS: %9.5f  FROM: %8.3f TO: %8.3f DEGREES"
//          "       %11.5f %5d %5d %5d %4d",
//          nwire, xw1, yw1, zw1, xw2, ns, i1, i2, itg);
  
        arc(i, itg, ns, xw1, yw1, zw1, xw2);
        continue;
        
      case 9:
        

      case 10: /* "gh" card, generate helix */
        num_wires++;
        i1 = geometry.n + 1;
        i2 = geometry.n + ns;
  
//        fprintf(output_fp, "\n"
//          " %5d HELIX STRUCTURE - SPACING OF TURNS: %8.3f AXIAL"
//          " LENGTH: %8.3f  %8.3f %5d %5d %5d %4d\n      "
//          " RADIUS X1:%8.3f Y1:%8.3f X2:%8.3f Y2:%8.3f ",
//          nwire, xw1, yw1, rad, ns, i1, i2, itg, zw1, xw2, yw2, zw2);
  
        // convert the original float value in F7 to int
        its = (int)(card.f[7] + .5);
        helix(i, card.i[1], card.i[2], card.f[1], card.f[2], card.f[3], card.f[4], card.f[5], card.f[6], its);
        continue;
  
      case 11: /* "gf" card, not supported */
        abort_on_error(-5);

      default: // error message if this isn't a comment
        if(!isComment(&card)) {
          sprintf(msg, "Geometry card on line %d has an unknown mnemonic, '%s'.", i + 1, card.card_code);
          add_error(errors, msg, 1);
        }

    } /* switch on card type */
  } /* for over cards */
  
  // free any message we might have made
  free(msg);
} /* calculate_geometry */


/******************************************************************************
 * segment_number
 *
 * segment_number (formerly isegno) returns the segment number for the mth
 * segment within the structure generated by the card with tag number tag.
 *
 * @param tag The tag number of the card
 * @param m The segment number within that structure
 *
 */
/* isegno returns the segment number of the mth segment having the */
/* tag number itagi. if itagi=0 segment number m is returned. */
int segment_number(int tag, int m)
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
	if (tag == 0) {
		iseg = m;
		return(iseg);
	}

	// if the tag isn't zero, look for it in the segment collection
	icnt = 0;
	if (geometry.n > 0) {
		int i;	// shouldn't this be defined in the for?
		for (i = 0; i < geometry.n; i++) {
			if (geometry.tag_nums[i] != tag)
				continue;

			icnt++;
			if (icnt == m) {
				iseg = i + 1;
				return(iseg);
			}
		} /* for( i = 0; i < data.n; i++ ) */
	} /* if( data.n > 0) */

	fprintf(output_fp, "\n\n"
		"  NO SEGMENT HAS AN ITAG OF %d", tag);
	stop(-1);

	return(0);
} /* end of segment_number */

/******************************************************************************
 * connect_segments
 *
 * connect_segments sets up segment connection data in arrays icon1 and
 * icon2 by searching for segment ends that are in contact.
 *
 * @param ignd Flag if a ground plane is in use, checks if wires touch it
 *
 */
void connect_segments(int ignd)
{
  int i, iz, ic, j, jx, ix, ixx, iseg, iend, jend, jump, ipf;
  double sep=0., xi1, yi1, zi1, xi2, yi2, zi2;
  double slen, xa, ya, za, xs, ys, zs;
  size_t mreq;

  segj.maxcon = 1;
  
  if(ignd != 0) {
    fprintf( output_fp, "\n\n     GROUND PLANE SPECIFIED." );
    
    if( ignd > 0)
      fprintf( output_fp,
              "\n     WHERE WIRE ENDS TOUCH GROUND, CURRENT WILL"
              " BE INTERPOLATED TO IMAGE IN GROUND PLANE.\n" );
    
    if( geometry.ipsym == 2) {
      geometry.np=2* geometry.np;
      geometry.mp=2* geometry.mp;
    }
    
    if( abs( geometry.ipsym) > 2 ) {
      geometry.np= geometry.n;
      geometry.mp= geometry.m;
    }
    
    /** possibly should be error condition?? **/
    if(geometry.np > geometry.n) {
      fprintf( output_fp,
              "\n ERROR: NP > N IN CONECT()" );
      stop(-1);
    }
    
    if((geometry.np == geometry.n) && (geometry.mp == geometry.m))
      geometry.ipsym=0;
    
  } /* if( ignd != 0) */

  if(geometry.n != 0) {
  /* Allocate memory to connections */
  mreq = (size_t)(geometry.n + geometry.m);
  mreq *= sizeof(int);
  mem_realloc((void *)&geometry.icon1, mreq);
  mem_realloc((void *)&geometry.icon2, mreq);

    for(i = 0; i < geometry.n; i++) {
      geometry.icon1[i] = geometry.icon2[i] = 0;
      iz = i+1;
      xi1= geometry.x1[i];
      yi1= geometry.y1[i];
      zi1= geometry.z1[i];
      xi2= geometry.x2[i];
      yi2= geometry.y2[i];
      zi2= geometry.z2[i];
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
          geometry.icon1[i]= iz;
          geometry.z1[i]=0.;
          jump = TRUE;
        } /* if( zi1 <= slen) */
      } /* if( ignd > 0) */
      
      if( !jump ) {
        ic= i;
        for( j = 1; j < geometry.n; j++) {
          ic++;
          if( ic >= geometry.n)
            ic=0;
          
          sep= fabs( xi1- geometry.x1[ic])+ fabs(yi1- geometry.y1[ic])+ fabs(zi1- geometry.z1[ic]);
          if( sep <= slen) {
            geometry.icon1[i]= -(ic+1);
            break;
          }
          
          sep= fabs( xi1- geometry.x2[ic])+ fabs(yi1- geometry.y2[ic])+ fabs(zi1- geometry.z2[ic]);
          if( sep <= slen) {
            geometry.icon1[i]= (ic+1);
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
          if( geometry.icon1[i] == iz ) {
            fprintf( output_fp,
                    "\n  GEOMETRY DATA ERROR -- SEGMENT"
                    " %d LIES IN GROUND PLANE", iz );
            stop(-1);
          }
          
          geometry.icon2[i]= iz;
          geometry.z2[i]=0.;
          continue;
          
        } /* if( zi2 <= slen) */
      } /* if( ignd > 0) */
      
      ic= i;
      for( j = 1; j < geometry.n; j++ ) {
        ic++;
        if( ic >= geometry.n)
          ic=0;
        
        sep= fabs(xi2- geometry.x1[ic])+ fabs(yi2- geometry.y1[ic])+ fabs(zi2- geometry.z1[ic]);
        if(sep <= slen) {
          geometry.icon2[i]= (ic+1);
          break;
        }
        
        sep= fabs(xi2- geometry.x2[ic])+ fabs(yi2- geometry.y2[ic])+ fabs(zi2- geometry.z2[ic]);
        if(sep <= slen) {
          geometry.icon2[i]= -(ic+1);
          break;
        }
        
      } /* for( j = 1; j < data.n; j++ ) */
    } /* for( i = 0; i < data.n; i++ ) */

  /* find wire-surface connections for new patches */
  if( geometry.m != 0) {
    ix = -1;
    i = 0;
    while( ++i <= geometry.m ) {
    ix++;
    xs= geometry.px[ix];
    ys= geometry.py[ix];
    zs= geometry.pz[ix];

    for( iseg = 0; iseg < geometry.n; iseg++ ) {
      xi1= geometry.x1[iseg];
      yi1= geometry.y1[iseg];
      zi1= geometry.z1[iseg];
      xi2= geometry.x2[iseg];
      yi2= geometry.y2[iseg];
      zi2= geometry.z2[iseg];

      /* for first end of segment */
      slen=( fabs(xi2- xi1)+ fabs(yi2- yi1)+ fabs(zi2- zi1))* SMIN;
      sep= fabs(xi1- xs)+ fabs(yi1- ys)+ fabs(zi1- zs);

      /* connection - divide patch into 4 patches at present array loc. */
      if( sep <= slen) {
      geometry.icon1[iseg]=PCHCON+ i;
      ic=0;
      subph( i, ic );
      break;
      }

      sep= fabs(xi2- xs)+ fabs(yi2- ys)+ fabs(zi2- zs);
      if( sep <= slen) {
      geometry.icon2[iseg]=PCHCON+ i;
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
    geometry.n, geometry.np, geometry.ipsym );

  if( geometry.m > 0)
  fprintf( output_fp,  "\n"
    "       TOTAL PATCHES USED: %d   PATCHES"
    " IN A SYMMETRIC CELL: %d",  geometry.m, geometry.mp );

  iseg=( geometry.n+ geometry.m)/( geometry.np+ geometry.mp);
  if( iseg != 1)  {
  /*** may be error condition?? ***/
  if( geometry.ipsym == 0 ) {
    fprintf( output_fp,
      "\n  ERROR: IPSYM=0 IN CONECT()" );
    stop(-1);
  }

  if( geometry.ipsym < 0 )
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

  if( geometry.n == 0)
  return;

  /* Allocate to connection buffers */
  mreq = (size_t)segj.maxcon;
  mreq *= sizeof(int);
  mem_realloc((void *)&segj.jco, mreq);

  /* adjust connected segment ends to exactly coincide.  print junctions */
  /* of 3 or more seg.  also find old seg. connecting to new seg. */
  iseg = 0;
  ipf = FALSE;
  for( j = 0; j < geometry.n; j++ ) {
  jx = j+1;
  iend=-1;
  jend=-1;
  ix= geometry.icon1[j];
  ic=1;
  segj.jco[0]= -jx;
  xa= geometry.x1[j];
  ya= geometry.y1[j];
  za= geometry.z1[j];

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
      mem_realloc((void *)&segj.jco, mreq);
      }
      segj.jco[ic-1]= ix* jend;

      ixx = ix-1;
      if( jend != 1) {
      xa= xa+ geometry.x1[ixx];
      ya= ya+ geometry.y1[ixx];
      za= za+ geometry.z1[ixx];
      ix= geometry.icon1[ixx];
      continue;
      }

      xa= xa+ geometry.x2[ixx];
      ya= ya+ geometry.y2[ixx];
      za= za+ geometry.z2[ixx];
      ix= geometry.icon2[ixx];

    } /* do */
    while( ix != 0 );

    if( jump && (iend == 1) )
      break;
    else
      if( jump ) {
      iend=1;
      jend=1;
      ix= geometry.icon2[j];
      ic=1;
      segj.jco[0]= jx;
      xa= geometry.x2[j];
      ya= geometry.y2[j];
      za= geometry.z2[j];
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
      geometry.x1[ixx]= xa;
      geometry.y1[ixx]= ya;
      geometry.z1[ixx]= za;
      continue;
      }

      ixx = ix-1;
      geometry.x2[ixx]= xa;
      geometry.y2[ixx]= ya;
      geometry.z2[ixx]= za;
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
    ix= geometry.icon2[j];
    ic=1;
    segj.jco[0]= jx;
    xa= geometry.x2[j];
    ya= geometry.y2[j];
    za= geometry.z2[j];

  } /* while( TRUE ) */
  } /* for( j = 0; j < data.n; j++ ) */

  mreq = (size_t)segj.maxcon;
  mreq *= sizeof(double);
  mem_realloc((void *)&segj.ax, mreq);
  mem_realloc((void *)&segj.bx, mreq);
  mem_realloc((void *)&segj.cx, mreq);
} /* end of connect_segments */

/******************************************************************************
 * wire
 *
 * wire generates segment geometry data for a straight wire of N segments.
 *
 * @param card_num Card number for this set of segments
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
 *****************************************************************************/
void wire(int card_num, int tag_num, int segs, double xw1, double yw1, double zw1,
  double xw2, double yw2, double zw2, double wire_radius, double rdel, double rrad)
{
  int first_segment_num;
  size_t mreq;
  double xd, yd, zd, delz, rd, fns, radz;
  double xs1, ys1, zs1, xs2, ys2, zs2;
  
  // copy down the starting segment number, and then move up all the segment counters
  first_segment_num = geometry.n;
  geometry.n += segs;
  geometry.np = geometry.n;
  geometry.mp = geometry.m;  // do we need this? it shouldn't have changed
  geometry.ipsym = 0;  // this says that symmetry is not true?
  
  // only add this wire if it actually has segments
  // FIXME: why don't we do this above?
  if(segs < 1) return;
  
  /* Reallocate tags buffer */
  mreq = (size_t)(geometry.n + geometry.m);
  mreq *= sizeof(int);
  mem_realloc((void *)&geometry.card_nums, mreq);
  mem_realloc((void *)&geometry.tag_nums, mreq);

  /* Reallocate wire buffers */
  mreq = (size_t)geometry.n;  // this is the current number of wire segments
  mreq *= sizeof(double);
  mem_realloc((void *)&geometry.x1, mreq);
  mem_realloc((void *)&geometry.y1, mreq);
  mem_realloc((void *)&geometry.z1, mreq);
  mem_realloc((void *)&geometry.x2, mreq);
  mem_realloc((void *)&geometry.y2, mreq);
  mem_realloc((void *)&geometry.z2, mreq);
  mem_realloc((void *)&geometry.bi, mreq);
  
  // calculate a segment length based either on the rdels parameter from a GC,
  // or the number of segments in a normal GW
  xd = xw2- xw1;
  yd = yw2- yw1;
  zd = zw2- zw1;
  
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
  radz = wire_radius;
  xs1 = xw1;
  ys1 = yw1;
  zs1 = zw1;
  
  // and for the rest of the segments, generate a segment end after moving
  // xd/yd/zd along the line, filling out the interior points
  for(int i = first_segment_num; i < geometry.n; i++) {
    // save these out
    geometry.card_nums[i] = card_num;
    geometry.tag_nums[i] = tag_num;
    
    // calculate the new locations
    xs2 = xs1 + xd * delz;
    ys2 = ys1 + yd * delz;
    zs2 = zs1 + zd * delz;
    
    // set the geometry
    geometry.x1[i] = xs1;
    geometry.y1[i] = ys1;
    geometry.z1[i] = zs1;
    geometry.x2[i] = xs2;
    geometry.y2[i] = ys2;
    geometry.z2[i] = zs2;
    geometry.bi[i] = radz;
    
    // move to the other and and re-taper
    delz = delz * rd;
    radz = radz * rrad;
    xs1 = xs2;
    ys1 = ys2;
    zs1 = zs2;
  }
  
  // fill in the end of the line with the end point
  geometry.x2[geometry.n-1] = xw2;
  geometry.y2[geometry.n-1] = yw2;
  geometry.z2[geometry.n-1] = zw2;
} /* end of wire() */

/******************************************************************************
 * arc
 *
 * arc generates segment geometry data for an arc of N segments.
 *
 * @param card_num Card number for this set of segments
 * @param tag_num Tag number for this set of segments, maybe 0
 * @param segs Number of segments in the arc
 * @param arc_radius Radius of the arc
 * @param ang1 Starting angle
 * @param ang2 Ending angle - ang2-ang1 <= 360
 * @param wire_radius Radius of the wire
 *
 *****************************************************************************/
void arc(int card_num, int tag_num, int segs, double arc_radius, double ang1, double ang2, double wire_radius)
{
  int first_segment_num = geometry.n;
  
  // update the geometry counters
  geometry.n += segs;
  geometry.np = geometry.n;
  geometry.mp = geometry.m;
  geometry.ipsym = 0;
  
  // no point continuing
  if(segs < 1) return;
  
  if(fabs(ang2- ang1) < 360.00001) {
    double ang, dang, xs1, xs2, zs1, zs2;
    
    // Reallocate card nums and tags buffer
    size_t mreq = (size_t)geometry.n;
    mreq *= sizeof(int);
    mem_realloc((void *)&geometry.card_nums, mreq);
    mem_realloc((void *)&geometry.tag_nums, mreq);

    // Reallocate wire buffers
    mreq = (size_t)geometry.n;
    mreq *= sizeof(double);
    mem_realloc((void *)&geometry.x1, mreq);
    mem_realloc((void *)&geometry.y1, mreq);
    mem_realloc((void *)&geometry.z1, mreq);
    mem_realloc((void *)&geometry.x2, mreq);
    mem_realloc((void *)&geometry.y2, mreq);
    mem_realloc((void *)&geometry.z2, mreq);
    mem_realloc((void *)&geometry.bi, mreq);
    
    ang = ang1 * TA;
    dang = (ang2- ang1) * TA/ segs;
    xs1 = arc_radius * cos(ang);
    zs1 = arc_radius * sin(ang);
    
    for(int i = first_segment_num; i < geometry.n; i++) {
      // save these bits out
      geometry.card_nums[i] = card_num;
      geometry.tag_nums[i] = tag_num;
      geometry.bi[i] = wire_radius;

      // move around the arc by the delta angle
      ang += dang;
      xs2 = arc_radius * cos(ang);
      zs2 = arc_radius * sin(ang);
      
      // save that out
      geometry.x1[i] = xs1;
      geometry.y1[i] = 0.0;
      geometry.z1[i] = zs1;
      geometry.x2[i] = xs2;
      geometry.y2[i] = 0.0;
      geometry.z2[i] = zs2;
      
      // move up one stop
      xs1 = xs2;
      zs1 = zs2;
    } /* for( i = ist; i < data.n; i++ ) */
  } /* if( fabs( ang2- ang1) < 360.00001) */
  else
  {
    fprintf( output_fp, "\n  ERROR -- ARC ANGLE EXCEEDS 360 DEGREES");
    stop(-1);
  }
} /* end of arc */

/******************************************************************************
 * helix
 *
 * helix generates segment geometry data for an a helix of ns segments.
 *
 * @param tag_num Tag number for this set of segments
 * @param segs Number of segments in the arc
 * @param rad Radius of the wire
 *
 *****************************************************************************/
void helix(int card_num, int tag_num, int segs, double s, double hl,
           double a1, double b1, double a2, double b2, double rad)
{
  int first_segment_num;
  size_t mreq;
  double zinc, copy, sangle, hdia, turn, pitch, hmaj, hmin;

  first_segment_num = geometry.n;
  geometry.n += segs;
  geometry.np = geometry.n;
  geometry.mp = geometry.m;
  geometry.ipsym = 0;
  
  // no point continuing...
  if(segs < 1) return;

  zinc = fabs(hl / segs);

  // Reallocate card num and tags buffer
  mreq = (size_t)(geometry.n + geometry.m);
  mreq *= sizeof(int);
  mem_realloc((void *)&geometry.card_nums, mreq);
  mem_realloc((void *)&geometry.tag_nums, mreq);

  // Reallocate wire buffers
  mreq = (size_t)geometry.n;
  mreq *= sizeof(double);
  mem_realloc((void *)&geometry.x1, mreq);
  mem_realloc((void *)&geometry.y1, mreq);
  mem_realloc((void *)&geometry.z1, mreq);
  mem_realloc((void *)&geometry.x2, mreq);
  mem_realloc((void *)&geometry.y2, mreq);
  mem_realloc((void *)&geometry.z2, mreq);
  mem_realloc((void *)&geometry.bi, mreq);

  geometry.z1[first_segment_num] = 0.0;
  for(int i = first_segment_num; i < geometry.n; i++ ) {
    // save these out
    geometry.card_nums[i] = card_num;
    geometry.tag_nums[i] = tag_num;
    geometry.bi[i] = rad;

    if(i != first_segment_num)
      geometry.z1[i] = geometry.z1[i-1] + zinc;
    
    geometry.z2[i] = geometry.z1[i] + zinc;
    
    if(a2 == a1) {
      if(b1 == 0.0)
        b1 = a1;
      
      geometry.x1[i]= a1* cos(2.* PI* geometry.z1[i]/ s);
      geometry.y1[i]= b1* sin(2.* PI* geometry.z1[i]/ s);
      geometry.x2[i]= a1* cos(2.* PI* geometry.z2[i]/ s);
      geometry.y2[i]= b1* sin(2.* PI* geometry.z2[i]/ s);
    }
    else
    {
      if(b2 == 0.0)
        b2= a2;
      
      geometry.x1[i]=( a1+( a2- a1)* geometry.z1[i]/ fabs( hl))* cos(2.* PI* geometry.z1[i]/ s);
      geometry.y1[i]=( b1+( b2- b1)* geometry.z1[i]/ fabs( hl))* sin(2.* PI* geometry.z1[i]/ s);
      geometry.x2[i]=( a1+( a2- a1)* geometry.z2[i]/ fabs( hl))* cos(2.* PI* geometry.z2[i]/ s);
      geometry.y2[i]=( b1+( b2- b1)* geometry.z2[i]/ fabs( hl))* sin(2.* PI* geometry.z2[i]/ s);
    } /* if( a2 == a1) */
    
    if( hl > 0.)
      continue;
    
    copy= geometry.x1[i];
    geometry.x1[i]= geometry.y1[i];
    geometry.y1[i]= copy;
    copy= geometry.x2[i];
    geometry.x2[i]= geometry.y2[i];
    geometry.y2[i]= copy;
    
  } /* for( i = ist; i < data.n; i++ ) */

  if(a2 != a1) {
    sangle = atan( a2/( fabs( hl)+( fabs( hl)* a1)/( a2- a1)));
    fprintf( output_fp,
            "\n       THE CONE ANGLE OF THE SPIRAL IS %10.4f", sangle );
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
  if(geometry.n > 0) {
    for (int i = 0; i < geometry.n; i++) {
      geometry.x1[i] = geometry.x1[i] * xw1;
      geometry.y1[i] = geometry.y1[i] * xw1;
      geometry.z1[i] = geometry.z1[i] * xw1;
      geometry.x2[i] = geometry.x2[i] * xw1;
      geometry.y2[i] = geometry.y2[i] * xw1;
      geometry.z2[i] = geometry.z2[i] * xw1;
      geometry.bi[i] = geometry.bi[i] * xw1;
    }
  } /* if( data.n >= n2) */
  
  // and then the patches
  if(geometry.m > 0) {
    yw1 = xw1* xw1;
    for (int i = 0; i < geometry.m; i++) {
      geometry.px[i] = geometry.px[i] * xw1;
      geometry.py[i] = geometry.py[i] * xw1;
      geometry.pz[i] = geometry.pz[i] * xw1;
      geometry.pbi[i] = geometry.pbi[i] * yw1;
    }
  } /* if( data.m >= m2) */
} /* end of scale */

/*-----------------------------------------------------------------------*/
/* duplicate moves the structure with respect to its coordinate system
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
	geometry.ipsym= geometry.ipsym*3;

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
  if( geometry.n > 0) {
	int ir;
	i1= segment_number( its, 1);
	if( i1 < 1)
	  i1= 1;

	ix= i1;
	if( nrpt == 0)
	  k= i1-1;
	else {
	  k= geometry.n;
	  /* Reallocate tags buffer */
	  mreq = (size_t)(geometry.n + geometry.m + (geometry.n + 1 - i1) * nrpt);
	  mreq *= sizeof(int);
	  mem_realloc((void *)&geometry.tag_nums, mreq);

	  /* Reallocate wire buffers */
	  mreq = (size_t)(geometry.n + (geometry.n + 1 - i1) * nrpt);
	  mreq *= sizeof(double);
	  mem_realloc((void *)&geometry.x1, mreq);
	  mem_realloc((void *)&geometry.y1, mreq);
	  mem_realloc((void *)&geometry.z1, mreq);
	  mem_realloc((void *)&geometry.x2, mreq);
	  mem_realloc((void *)&geometry.y2, mreq);
	  mem_realloc((void *)&geometry.z2, mreq);
	  mem_realloc((void *)&geometry.bi, mreq);
	}

	for( ir = 0; ir < nrp; ir++ ) {
	  for( i = i1-1; i < geometry.n; i++ )  {
		xi= geometry.x1[i];
		yi= geometry.y1[i];
		zi= geometry.z1[i];
		geometry.x1[k]= xi* xx+ yi* xy+ zi* xz+ xs;
		geometry.y1[k]= xi* yx+ yi* yy+ zi* yz+ ys;
		geometry.z1[k]= xi* zx+ yi* zy+ zi* zz+ zs;
		xi= geometry.x2[i];
		yi= geometry.y2[i];
		zi= geometry.z2[i];
		geometry.x2[k]= xi* xx+ yi* xy+ zi* xz+ xs;
		geometry.y2[k]= xi* yx+ yi* yy+ zi* yz+ ys;
		geometry.z2[k]= xi* zx+ yi* zy+ zi* zz+ zs;
		geometry.bi[k]= geometry.bi[i];
		geometry.tag_nums[k]= geometry.tag_nums[i];
		if( geometry.tag_nums[i] != 0)
		  geometry.tag_nums[k]= geometry.tag_nums[i]+ itgi;

		k++;

	  } /* for( i = i1; i < data.n; i++ ) */

	  i1= geometry.n+1;
	  geometry.n= k;

	} /* for( ir = 0; ir < nrp; ir++ ) */

  } /* if( data.n >= n2) */

  // repeat the move for any patches
  if( geometry.m > 0) {
	int ii;
	i1 = 0;
	if( nrpt == 0)
	  k= 0;
	else
	  k = geometry.m;

	/* Reallocate patch buffers */
	mreq = (size_t)(geometry.m * (nrpt + 1));
	mreq *= sizeof(double);
	mem_realloc((void *)&geometry.px, mreq);
	mem_realloc((void *)&geometry.py, mreq);
	mem_realloc((void *)&geometry.pz, mreq);
	mem_realloc((void *)&geometry.t1x, mreq);
	mem_realloc((void *)&geometry.t1y, mreq);
	mem_realloc((void *)&geometry.t1z, mreq);
	mem_realloc((void *)&geometry.t2x, mreq);
	mem_realloc((void *)&geometry.t2y, mreq);
	mem_realloc((void *)&geometry.t2z, mreq);
	mem_realloc((void *)&geometry.pbi, mreq);
	mem_realloc((void *)&geometry.psalp, mreq);

	for( ii = 0; ii < nrp; ii++ ) {
	  for( i = i1; i < geometry.m; i++ ) {
		xi= geometry.px[i];
		yi= geometry.py[i];
		zi= geometry.pz[i];
		geometry.px[k]= xi* xx+ yi* xy+ zi* xz+ xs;
		geometry.py[k]= xi* yx+ yi* yy+ zi* yz+ ys;
		geometry.pz[k]= xi* zx+ yi* zy+ zi* zz+ zs;
		xi= geometry.t1x[i];
		yi= geometry.t1y[i];
		zi= geometry.t1z[i];
		geometry.t1x[k]= xi* xx+ yi* xy+ zi* xz;
		geometry.t1y[k]= xi* yx+ yi* yy+ zi* yz;
		geometry.t1z[k]= xi* zx+ yi* zy+ zi* zz;
		xi= geometry.t2x[i];
		yi= geometry.t2y[i];
		zi= geometry.t2z[i];
		geometry.t2x[k]= xi* xx+ yi* xy+ zi* xz;
		geometry.t2y[k]= xi* yx+ yi* yy+ zi* yz;
		geometry.t2z[k]= xi* zx+ yi* zy+ zi* zz;
		geometry.psalp[k]= geometry.psalp[i];
		geometry.pbi[k]= geometry.pbi[i];
		k++;
	  } /* for( i = i1; i < data.m; i++ ) */

	  i1= geometry.m;
	  geometry.m = k;
	} /* for( ii = 0; ii < nrp; ii++ ) */

  } /* if( data.m >= m2) */

  if( (nrpt == 0) && (ix == 1) )
    return;

  geometry.np= geometry.n;
  geometry.mp= geometry.m;
  geometry.ipsym=0;
} /* end of duplicate */

/*-----------------------------------------------------------------------*/

/* reflect() reflects partial structure along x,y, or z axes or rotates */
/* structure to complete a symmetric structure. */
void reflect( int ix, int iy, int iz, int itx, int nop )
{
  int iti, i, nx, itagi, k;
  size_t mreq;
  double e1, e2, fnop, sam, cs, ss, xk, yk;
  
  geometry.np= geometry.n;
  geometry.mp= geometry.m;
  geometry.ipsym=0;
  iti= itx;
  
  if(ix >= 0) {
    if(nop == 0)
      return;
    
    geometry.ipsym=1;
    
    /* reflect along z axis */
    if( iz != 0 ) {
      geometry.ipsym=2;
      
      if( geometry.n > 0 ) {
        /* Reallocate tags buffer */
        mreq = (size_t)(2 * geometry.n + geometry.m);
        mreq *= sizeof(int);
        mem_realloc((void *)&geometry.tag_nums, mreq);
        
        /* Reallocate wire buffers */
        mreq = (size_t)(2 * geometry.n);
        mreq *= sizeof(double);
        mem_realloc((void *)&geometry.x1, mreq);
        mem_realloc((void *)&geometry.y1, mreq);
        mem_realloc((void *)&geometry.z1, mreq);
        mem_realloc((void *)&geometry.x2, mreq);
        mem_realloc((void *)&geometry.y2, mreq);
        mem_realloc((void *)&geometry.z2, mreq);
        mem_realloc((void *)&geometry.bi, mreq);
        
        for(i = 0; i < geometry.n; i++) {
          nx= i+ geometry.n;
          e1= geometry.z1[i];
          e2= geometry.z2[i];
          
          if( (fabs(e1)+fabs(e2) <= 1.0e-5) || (e1*e2 < -1.0e-6) ) {
            fprintf( output_fp,
                    "\n  GEOMETRY DATA ERROR--SEGMENT %d"
                    " LIES IN PLANE OF SYMMETRY", i+1 );
            stop(-1);
          }
          
          geometry.x1[nx]= geometry.x1[i];
          geometry.y1[nx]= geometry.y1[i];
          geometry.z1[nx]= -e1;
          geometry.x2[nx]= geometry.x2[i];
          geometry.y2[nx]= geometry.y2[i];
          geometry.z2[nx]= -e2;
          itagi= geometry.tag_nums[i];
          
          if( itagi == 0)
            geometry.tag_nums[nx]=0;
          if( itagi != 0)
            geometry.tag_nums[nx]= itagi+ iti;
          
          geometry.bi[nx]= geometry.bi[i];
          
        } /* for( i = 0; i < data.n; i++ ) */
        
        geometry.n= geometry.n*2;
        iti= iti*2;
        
      } /* if( data.n > 0) */
      
      if(geometry.m > 0) {
        /* Reallocate patch buffers */
        mreq = (size_t)(2 * geometry.m);
        mreq *= sizeof(double);
        mem_realloc((void *)&geometry.px, mreq);
        mem_realloc((void *)&geometry.py, mreq);
        mem_realloc((void *)&geometry.pz, mreq);
        mem_realloc((void *)&geometry.t1x, mreq);
        mem_realloc((void *)&geometry.t1y, mreq);
        mem_realloc((void *)&geometry.t1z, mreq);
        mem_realloc((void *)&geometry.t2x, mreq);
        mem_realloc((void *)&geometry.t2y, mreq);
        mem_realloc((void *)&geometry.t2z, mreq);
        mem_realloc((void *)&geometry.pbi, mreq);
        mem_realloc((void *)&geometry.psalp, mreq);
        
        for(i = 0; i < geometry.m; i++) {
          nx = i+geometry.m;
          if(fabs(geometry.pz[i]) <= 1.0e-10) {
            fprintf( output_fp,
                    "\n  GEOMETRY DATA ERROR--PATCH %d"
                    " LIES IN PLANE OF SYMMETRY", i+1 );
            stop(-1);
          }
          
          geometry.px[nx]= geometry.px[i];
          geometry.py[nx]= geometry.py[i];
          geometry.pz[nx]= -geometry.pz[i];
          geometry.t1x[nx]= geometry.t1x[i];
          geometry.t1y[nx]= geometry.t1y[i];
          geometry.t1z[nx]= -geometry.t1z[i];
          geometry.t2x[nx]= geometry.t2x[i];
          geometry.t2y[nx]= geometry.t2y[i];
          geometry.t2z[nx]= -geometry.t2z[i];
          geometry.psalp[nx]= -geometry.psalp[i];
          geometry.pbi[nx]= geometry.pbi[i];
        }
        
        geometry.m= geometry.m*2;
      } /* if( data.m >= m2) */
    } /* if( iz != 0) */
    
    /* reflect along y axis */
    if( iy != 0) {
      if( geometry.n > 0) {
        /* Reallocate tags buffer */
        mreq = (size_t)(2 * geometry.n + geometry.m);
        mreq *= sizeof(int);
        mem_realloc((void *)&geometry.tag_nums, mreq);
        
        /* Reallocate wire buffers */
        mreq = (size_t)(2 * geometry.n);
        mreq *= sizeof(double);
        mem_realloc((void *)&geometry.x1, mreq);
        mem_realloc((void *)&geometry.y1, mreq);
        mem_realloc((void *)&geometry.z1, mreq);
        mem_realloc((void *)&geometry.x2, mreq);
        mem_realloc((void *)&geometry.y2, mreq);
        mem_realloc((void *)&geometry.z2, mreq);
        mem_realloc((void *)&geometry.bi, mreq);
        
        for( i = 0; i < geometry.n; i++ ) {
          nx= i+ geometry.n;
          e1= geometry.y1[i];
          e2= geometry.y2[i];
          
          if( (fabs(e1)+fabs(e2) <= 1.0e-5) || (e1*e2 < -1.0e-6) ) {
            fprintf( output_fp,
                    "\n  GEOMETRY DATA ERROR--SEGMENT %d"
                    " LIES IN PLANE OF SYMMETRY", i+1 );
            stop(-1);
          }
          
          geometry.x1[nx]= geometry.x1[i];
          geometry.y1[nx]= -e1;
          geometry.z1[nx]= geometry.z1[i];
          geometry.x2[nx]= geometry.x2[i];
          geometry.y2[nx]= -e2;
          geometry.z2[nx]= geometry.z2[i];
          itagi= geometry.tag_nums[i];
          
          if( itagi == 0)
            geometry.tag_nums[nx]=0;
          if( itagi != 0)
            geometry.tag_nums[nx]= itagi+ iti;
          
          geometry.bi[nx]= geometry.bi[i];
          
        } /* for( i = n2-1; i < data.n; i++ ) */
        
        geometry.n= geometry.n*2;
        iti= iti*2;
        
      } /* if( data.n >= n2) */
      
      // reflect any patches
      if(geometry.m > 0)  {
        // reflection doubles the number of patches, so we start
        // by reallocating the patch list to hold the new ones
        mreq = (size_t)(2 * geometry.m);
        mreq *= sizeof(double);
        mem_realloc((void *)&geometry.px, mreq);
        mem_realloc((void *)&geometry.py, mreq);
        mem_realloc((void *)&geometry.pz, mreq);
        mem_realloc((void *)&geometry.t1x, mreq);
        mem_realloc((void *)&geometry.t1y, mreq);
        mem_realloc((void *)&geometry.t1z, mreq);
        mem_realloc((void *)&geometry.t2x, mreq);
        mem_realloc((void *)&geometry.t2y, mreq);
        mem_realloc((void *)&geometry.t2z, mreq);
        mem_realloc((void *)&geometry.pbi, mreq);
        mem_realloc((void *)&geometry.psalp, mreq);
        
        for( i = 0; i < geometry.m; i++ ) {
          nx= i+geometry.m;
          if( fabs( geometry.py[i]) <= 1.0e-10) {
            fprintf( output_fp,
                    "\n  GEOMETRY DATA ERROR--PATCH %d"
                    " LIES IN PLANE OF SYMMETRY", i+1 );
            stop(-1);
          }
          
          geometry.px[nx]= geometry.px[i];
          geometry.py[nx]= -geometry.py[i];
          geometry.pz[nx]= geometry.pz[i];
          geometry.t1x[nx]= geometry.t1x[i];
          geometry.t1y[nx]= -geometry.t1y[i];
          geometry.t1z[nx]= geometry.t1z[i];
          geometry.t2x[nx]= geometry.t2x[i];
          geometry.t2y[nx]= -geometry.t2y[i];
          geometry.t2z[nx]= geometry.t2z[i];
          geometry.psalp[nx]= -geometry.psalp[i];
          geometry.pbi[nx]= geometry.pbi[i];
          
        } /* for( i = m2; i <= data.m; i++ ) */
        
        geometry.m= geometry.m*2;
        
      } /* if( data.m >= m2) */
      
    } /* if( iy != 0) */
    
    /* reflect along x axis */
    if( ix == 0 )
      return;
    
    if( geometry.n > 0 ) {
      /* Reallocate tags buffer */
      mreq = (size_t)(2 * geometry.n + geometry.m);
      mreq *= sizeof(int);
      mem_realloc((void *)&geometry.tag_nums, mreq);
      
      /* Reallocate wire buffers */
      mreq = (size_t)(2 * geometry.n);
      mreq *= sizeof(double);
      mem_realloc((void *)&geometry.x1, mreq);
      mem_realloc((void *)&geometry.y1, mreq);
      mem_realloc((void *)&geometry.z1, mreq);
      mem_realloc((void *)&geometry.x2, mreq);
      mem_realloc((void *)&geometry.y2, mreq);
      mem_realloc((void *)&geometry.z2, mreq);
      mem_realloc((void *)&geometry.bi, mreq);
      
      for( i = 0; i < geometry.n; i++ ) {
        nx= i+ geometry.n;
        e1= geometry.x1[i];
        e2= geometry.x2[i];
        
        if( (fabs(e1)+fabs(e2) <= 1.0e-5) || (e1*e2 < -1.0e-6) ) {
          fprintf( output_fp,
                  "\n  GEOMETRY DATA ERROR--SEGMENT %d"
                  " LIES IN PLANE OF SYMMETRY", i+1 );
          stop(-1);
        }
        
        geometry.x1[nx]= -e1;
        geometry.y1[nx]= geometry.y1[i];
        geometry.z1[nx]= geometry.z1[i];
        geometry.x2[nx]= -e2;
        geometry.y2[nx]= geometry.y2[i];
        geometry.z2[nx]= geometry.z2[i];
        itagi= geometry.tag_nums[i];
        
        if( itagi == 0)
          geometry.tag_nums[nx]=0;
        if( itagi != 0)
          geometry.tag_nums[nx]= itagi+ iti;
        
        geometry.bi[nx]= geometry.bi[i];
      }
      
      geometry.n= geometry.n*2;
      
    } /* if( data.n > 0) */
    
    if( geometry.m == 0 )
      return;
    
    /* Reallocate patch buffers */
    mreq = (size_t)(2 * geometry.m);
    mreq *= sizeof(double);
    mem_realloc((void *)&geometry.px, mreq);
    mem_realloc((void *)&geometry.py, mreq);
    mem_realloc((void *)&geometry.pz, mreq);
    mem_realloc((void *)&geometry.t1x, mreq);
    mem_realloc((void *)&geometry.t1y, mreq);
    mem_realloc((void *)&geometry.t1z, mreq);
    mem_realloc((void *)&geometry.t2x, mreq);
    mem_realloc((void *)&geometry.t2y, mreq);
    mem_realloc((void *)&geometry.t2z, mreq);
    mem_realloc((void *)&geometry.pbi, mreq);
    mem_realloc((void *)&geometry.psalp, mreq);
    
    for( i = 0; i < geometry.m; i++ ) {
      nx= i+geometry.m;
      if( fabs( geometry.px[i]) <= 1.0e-10) {
        fprintf( output_fp,
                "\n  GEOMETRY DATA ERROR--PATCH %d"
                " LIES IN PLANE OF SYMMETRY", i+1 );
        stop(-1);
      }
      
      geometry.px[nx]= -geometry.px[i];
      geometry.py[nx]= geometry.py[i];
      geometry.pz[nx]= geometry.pz[i];
      geometry.t1x[nx]= -geometry.t1x[i];
      geometry.t1y[nx]= geometry.t1y[i];
      geometry.t1z[nx]= geometry.t1z[i];
      geometry.t2x[nx]= -geometry.t2x[i];
      geometry.t2y[nx]= geometry.t2y[i];
      geometry.t2z[nx]= geometry.t2z[i];
      geometry.psalp[nx]= -geometry.psalp[i];
      geometry.pbi[nx]= geometry.pbi[i];
    }
    
    geometry.m= geometry.m*2;
    return;
    
  } /* if( ix >= 0) */
  
  /* reproduce structure with rotation to form cylindrical structure */
  fnop= (double)nop;
  geometry.ipsym=-1;
  sam=TP/ fnop;
  cs= cos( sam);
  ss= sin( sam);
  
  if( geometry.n > 0) {
    geometry.n *= nop;
    nx= geometry.np;
    
    /* Reallocate tags buffer */
    mreq = (size_t)(geometry.n + geometry.m);
    mreq *= sizeof(int);
    mem_realloc((void *)&geometry.tag_nums, mreq);
    
    /* Reallocate wire buffers */
    mreq = (size_t)geometry.n;
    mreq *= sizeof(double);
    mem_realloc((void *)&geometry.x1, mreq);
    mem_realloc((void *)&geometry.y1, mreq);
    mem_realloc((void *)&geometry.z1, mreq);
    mem_realloc((void *)&geometry.x2, mreq);
    mem_realloc((void *)&geometry.y2, mreq);
    mem_realloc((void *)&geometry.z2, mreq);
    mem_realloc((void *)&geometry.bi, mreq);
    
    for( i = nx; i < geometry.n; i++ ) {
      k= i- geometry.np;
      xk= geometry.x1[k];
      yk= geometry.y1[k];
      geometry.x1[i]= xk* cs- yk* ss;
      geometry.y1[i]= xk* ss+ yk* cs;
      geometry.z1[i]= geometry.z1[k];
      xk= geometry.x2[k];
      yk= geometry.y2[k];
      geometry.x2[i]= xk* cs- yk* ss;
      geometry.y2[i]= xk* ss+ yk* cs;
      geometry.z2[i]= geometry.z2[k];
      geometry.bi[i]= geometry.bi[k];
      itagi= geometry.tag_nums[k];
      
      if( itagi == 0)
        geometry.tag_nums[i]=0;
      if( itagi != 0)
        geometry.tag_nums[i]= itagi+ iti;
    }
    
  } /* if( data.n >= n2) */
  
  if( geometry.m == 0 )
    return;
  
  geometry.m *= nop;
  nx= geometry.mp;
  
  /* Reallocate patch buffers */
  mreq = (size_t)geometry.m;
  mreq *= sizeof(double);
  mem_realloc((void *)&geometry.px, mreq);
  mem_realloc((void *)&geometry.py, mreq);
  mem_realloc((void *)&geometry.pz, mreq);
  mem_realloc((void *)&geometry.t1x, mreq);
  mem_realloc((void *)&geometry.t1y, mreq);
  mem_realloc((void *)&geometry.t1z, mreq);
  mem_realloc((void *)&geometry.t2x, mreq);
  mem_realloc((void *)&geometry.t2y, mreq);
  mem_realloc((void *)&geometry.t2z, mreq);
  mem_realloc((void *)&geometry.pbi, mreq);
  mem_realloc((void *)&geometry.psalp, mreq);
  
  for(i = nx; i < geometry.m; i++) {
    k = i-geometry.mp;
    xk= geometry.px[k];
    yk= geometry.py[k];
    geometry.px[i]= xk* cs- yk* ss;
    geometry.py[i]= xk* ss+ yk* cs;
    geometry.pz[i]= geometry.pz[k];
    xk= geometry.t1x[k];
    yk= geometry.t1y[k];
    geometry.t1x[i]= xk* cs- yk* ss;
    geometry.t1y[i]= xk* ss+ yk* cs;
    geometry.t1z[i]= geometry.t1z[k];
    xk= geometry.t2x[k];
    yk= geometry.t2y[k];
    geometry.t2x[i]= xk* cs- yk* ss;
    geometry.t2y[i]= xk* ss+ yk* cs;
    geometry.t2z[i]= geometry.t2z[k];
    geometry.psalp[i]= geometry.psalp[k];
    geometry.pbi[i]= geometry.pbi[k];
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

  geometry.m++;
  mi= geometry.m-1;

  /* Reallocate patch buffers */
  mreq = (size_t)geometry.m;
  mreq *= sizeof(double);
  mem_realloc((void *)&geometry.px, mreq);
  mem_realloc((void *)&geometry.py, mreq);
  mem_realloc((void *)&geometry.pz, mreq);
  mem_realloc((void *)&geometry.t1x, mreq);
  mem_realloc((void *)&geometry.t1y, mreq);
  mem_realloc((void *)&geometry.t1z, mreq);
  mem_realloc((void *)&geometry.t2x, mreq);
  mem_realloc((void *)&geometry.t2y, mreq);
  mem_realloc((void *)&geometry.t2z, mreq);
  mem_realloc((void *)&geometry.pbi, mreq);
  mem_realloc((void *)&geometry.psalp, mreq);

  if( nx > 0)
	ntp=2;
  else
	ntp= ny;

  if( ntp <= 1) {
    geometry.px[mi]= ax1;
    geometry.py[mi]= ay1;
    geometry.pz[mi]= az1;
    geometry.pbi[mi]= az2;
    znv= cos( ax2);
    xnv= znv* cos( ay2);
    ynv= znv* sin( ay2);
    znv= sin( ax2);
    xa= sqrt( xnv* xnv+ ynv* ynv);

    if( xa >= 1.0e-6) {
      geometry.t1x[mi]= -ynv/ xa;
      geometry.t1y[mi]= xnv/ xa;
      geometry.t1z[mi]=0.;
    } else {
      geometry.t1x[mi]=1.;
      geometry.t1y[mi]=0.;
      geometry.t1z[mi]=0.;
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
    geometry.t1x[mi]= s1x/ xst;
    geometry.t1y[mi]= s1y/ xst;
    geometry.t1z[mi]= s1z/ xst;

    if( ntp <= 2) {
      geometry.px[mi]= ax1+.5*( s1x+ s2x);
      geometry.py[mi]= ay1+.5*( s1y+ s2y);
      geometry.pz[mi]= az1+.5*( s1z+ s2z);
      geometry.pbi[mi]= xa;
    }
    else {
      if( ntp != 4) {
      geometry.px[mi]=( ax1+ ax2+ ax3)/3.;
      geometry.py[mi]=( ay1+ ay2+ ay3)/3.;
      geometry.pz[mi]=( az1+ az2+ az3)/3.;
      geometry.pbi[mi]=.5* xa;
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
        geometry.px[mi]=( xa*( ax1+ ax2+ ax3)+ xst*( ax1+ ax3+ ax4))* salpn;
        geometry.py[mi]=( xa*( ay1+ ay2+ ay3)+ xst*( ay1+ ay3+ ay4))* salpn;
        geometry.pz[mi]=( xa*( az1+ az2+ az3)+ xst*( az1+ az3+ az4))* salpn;
        geometry.pbi[mi]=.5*( xa+ xst);
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

  geometry.t2x[mi]= ynv* geometry.t1z[mi]- znv* geometry.t1y[mi];
  geometry.t2y[mi]= znv* geometry.t1x[mi]- xnv* geometry.t1z[mi];
  geometry.t2z[mi]= xnv* geometry.t1y[mi]- ynv* geometry.t1x[mi];
  geometry.psalp[mi]=1.;

  if( nx != 0) {
	int iy, ix;
	double xs, ys, zs, xt, yt, zt;

	geometry.m += nx*ny-1;
	/* Reallocate patch buffers */
	mreq = (size_t)geometry.m;
	mreq *= sizeof(double);
	mem_realloc((void *)&geometry.px, mreq);
	mem_realloc((void *)&geometry.py, mreq);
	mem_realloc((void *)&geometry.pz, mreq);
	mem_realloc((void *)&geometry.t1x, mreq);
	mem_realloc((void *)&geometry.t1y, mreq);
	mem_realloc((void *)&geometry.t1z, mreq);
	mem_realloc((void *)&geometry.t2x, mreq);
	mem_realloc((void *)&geometry.t2y, mreq);
	mem_realloc((void *)&geometry.t2z, mreq);
	mem_realloc((void *)&geometry.pbi, mreq);
	mem_realloc((void *)&geometry.psalp, mreq);

	xn2= geometry.px[mi]- s1x- s2x;
	yn2= geometry.py[mi]- s1y- s2y;
	zn2= geometry.pz[mi]- s1z- s2z;
	xs= geometry.t1x[mi];
	ys= geometry.t1y[mi];
	zs= geometry.t1z[mi];
	xt= geometry.t2x[mi];
	yt= geometry.t2y[mi];
	zt= geometry.t2z[mi];

	for(iy = 0; iy < ny; iy++) {
	  xn2 += s2x;
	  yn2 += s2y;
	  zn2 += s2z;

	  for(ix = 1; ix <= nx; ix++) {
      xst= (double)ix;
      geometry.px[mi]= xn2+ xst* s1x;
      geometry.py[mi]= yn2+ xst* s1y;
      geometry.pz[mi]= zn2+ xst* s1z;
      geometry.pbi[mi]= xa;
      geometry.psalp[mi]=1.;
      geometry.t1x[mi]= xs;
      geometry.t1y[mi]= ys;
      geometry.t1z[mi]= zs;
      geometry.t2x[mi]= xt;
      geometry.t2y[mi]= yt;
      geometry.t2z[mi]= zt;
      mi++;
	  } /* for( ix = 0; ix < nx; ix++ ) */
	} /* for( iy = 0; iy < ny; iy++ ) */
  } /* if( nx != 0) */

  geometry.ipsym = 0;
  geometry.np = geometry.n;
  geometry.mp = geometry.m;
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
    geometry.m += 3;
  } else {
    geometry.m += 4;
  }

  mreq = (size_t)geometry.m;
  mreq *= sizeof(double);
  mem_realloc((void *)&geometry.px, mreq);
  mem_realloc((void *)&geometry.py, mreq);
  mem_realloc((void *)&geometry.pz, mreq);
  mem_realloc((void *)&geometry.t1x, mreq);
  mem_realloc((void *)&geometry.t1y, mreq);
  mem_realloc((void *)&geometry.t1z, mreq);
  mem_realloc((void *)&geometry.t2x, mreq);
  mem_realloc((void *)&geometry.t2y, mreq);
  mem_realloc((void *)&geometry.t2z, mreq);
  mem_realloc((void *)&geometry.pbi, mreq);
  mem_realloc((void *)&geometry.psalp, mreq);
  mreq = (size_t)(geometry.n + geometry.m);
  mreq *= sizeof(int);
  mem_realloc((void *)&geometry.icon1, mreq);
  mem_realloc((void *)&geometry.icon2, mreq);

  /* Shift patches to make room for new ones */
  if((ny == 0) && (nx != geometry.m))  {
    for(iy = geometry.m-1; iy > nx+2; iy--) {
      ix = iy-3;
      geometry.px[iy]= geometry.px[ix];
      geometry.py[iy]= geometry.py[ix];
      geometry.pz[iy]= geometry.pz[ix];
      geometry.pbi[iy]= geometry.pbi[ix];
      geometry.psalp[iy]= geometry.psalp[ix];
      geometry.t1x[iy]= geometry.t1x[ix];
      geometry.t1y[iy]= geometry.t1y[ix];
      geometry.t1z[iy]= geometry.t1z[ix];
      geometry.t2x[iy]= geometry.t2x[ix];
      geometry.t2y[iy]= geometry.t2y[ix];
      geometry.t2z[iy]= geometry.t2z[ix];
    }
  } /* if( (ny == 0) || (nx != m) ) */

  /* divide patch for connection */
  mi= nx-1;
  xs= geometry.px[mi];
  ys= geometry.py[mi];
  zs= geometry.pz[mi];
  xa= geometry.pbi[mi]/4.;
  xst= sqrt( xa)/2.;
  s1x= geometry.t1x[mi];
  s1y= geometry.t1y[mi];
  s1z= geometry.t1z[mi];
  s2x= geometry.t2x[mi];
  s2y= geometry.t2y[mi];
  s2z= geometry.t2z[mi];
  saln= geometry.psalp[mi];
  xt= xst;
  yt= xst;

  if(ny == 0)
    mia= mi;
  else {
    geometry.mp++;
    mia= geometry.m-1;
  }

  for(ix = 1; ix <= 4; ix++) {
    geometry.px[mia]= xs+ xt* s1x+ yt* s2x;
    geometry.py[mia]= ys+ xt* s1y+ yt* s2y;
    geometry.pz[mia]= zs+ xt* s1z+ yt* s2z;
    geometry.pbi[mia]= xa;
    geometry.t1x[mia]= s1x;
    geometry.t1y[mia]= s1y;
    geometry.t1z[mia]= s1z;
    geometry.t2x[mia]= s2x;
    geometry.t2y[mia]= s2y;
    geometry.t2z[mia]= s2z;
    geometry.psalp[mia]= saln;

    if( ix == 2)
      yt= -yt;

    if( (ix == 1) || (ix == 3) )
      xt= -xt;

    mia++;
  }

  if(nx <= geometry.mp)
    geometry.mp += 3;

  if(ny > 0)
    geometry.pz[mi]=10000.0;
} /* end of subph */
